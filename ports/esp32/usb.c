/*
 * This file is part of the MicroPython project, http://micropython.org/
 *
 * The MIT License (MIT)
 *
 * Copyright (c) 2021 Damien P. George
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#include "py/runtime.h"
#include "py/mphal.h"
#include "usb.h"

#if CONFIG_USB_OTG_SUPPORTED && !CONFIG_ESP_CONSOLE_USB_CDC && !CONFIG_ESP_CONSOLE_USB_SERIAL_JTAG

#include "esp_timer.h"
#include "esp_log.h"
#ifndef NO_QSTR
#include "tinyusb.h"
#include "tusb_cdc_acm.h"
#include "tusb_console.h"
#endif

#if CONFIG_TINYUSB_MSC_ENABLED
#include "tusb_msc_storage.h"
#include "diskio_impl.h"
#include "diskio_sdmmc.h"
#endif

#define CDC_ITF TINYUSB_CDC_ACM_0

static uint8_t usb_rx_buf[CONFIG_TINYUSB_CDC_RX_BUFSIZE];

// This is called from FreeRTOS task "tusb_tsk" in espressif__esp_tinyusb (not an ISR).
static void usb_callback_rx(int itf, cdcacm_event_t *event) {
    // espressif__esp_tinyusb places tinyusb rx data onto freertos ringbuffer which
    // this function forwards onto our stdin_ringbuf.
    for (;;) {
        size_t len = 0;
        esp_err_t ret = tinyusb_cdcacm_read(itf, usb_rx_buf, sizeof(usb_rx_buf), &len);
        if (ret != ESP_OK) {
            break;
        }
        if (len == 0) {
            break;
        }
        for (size_t i = 0; i < len; ++i) {
            if (usb_rx_buf[i] == mp_interrupt_char) {
                mp_sched_keyboard_interrupt();
            } else {
                ringbuf_put(&stdin_ringbuf, usb_rx_buf[i]);
            }
        }
        mp_hal_wake_main_task();
    }
}
#if CONFIG_TINYUSB_MSC_ENABLED
static esp_err_t storage_init_sdmmc(sdmmc_card_t **card)
{
    esp_err_t ret = ESP_OK;
    bool host_init = false;
    sdmmc_card_t *sd_card;

    sdmmc_host_t host = SDMMC_HOST_DEFAULT();
    sdmmc_slot_config_t slot_config = SDMMC_SLOT_CONFIG_DEFAULT();
    slot_config.width = 4;
    slot_config.clk = 7;
    slot_config.cmd = 6;
    slot_config.d0 = 15;
    slot_config.d1 = 16;
    slot_config.d2 = 4;
    slot_config.d3 = 5;

    // Enable internal pullups on enabled pins. The internal pullups
    // are insufficient however, please make sure 10k external pullups are
    // connected on the bus. This is for debug / example purpose only.
    slot_config.flags |= SDMMC_SLOT_FLAG_INTERNAL_PULLUP;

    // not using ff_memalloc here, as allocation in internal RAM is preferred
    sd_card = (sdmmc_card_t *)malloc(sizeof(sdmmc_card_t));
    if (NULL == sd_card) {
        ret = ESP_ERR_NO_MEM;
        goto clean;
    }

    ret = host.init();
    if (ret != ESP_OK) {
        goto clean;
    }

    host_init = true;

    ret = sdmmc_host_init_slot(host.slot, (const sdmmc_slot_config_t *) &slot_config);
    if (ret != ESP_OK) {
        goto clean;
    }

    while (sdmmc_card_init(&host, sd_card)) {
        ESP_LOGE("usb", "The detection pin of the slot is disconnected(Insert uSD card). Retrying...");
        vTaskDelay(pdMS_TO_TICKS(3000));
    }

    // Card has been initialized, print its properties
    //sdmmc_card_print_info(stdout, sd_card);
    *card = sd_card;

    return ESP_OK;
    goto clean;

clean:
    if (host_init) {
        if (host.flags & SDMMC_HOST_FLAG_DEINIT_ARG) {
            host.deinit_p(host.slot);
        } else {
            (*host.deinit)();
        }
    }
    if (sd_card) {
        free(sd_card);
        sd_card = NULL;
    }
    return ret;
}
#endif

void usb_init(void) {
    #if CONFIG_TINYUSB_MSC_ENABLED
    // init SDMMC
    ESP_LOGI("msc", "starting sdmmc");
    sdmmc_card_t * card = NULL;
    ESP_ERROR_CHECK(storage_init_sdmmc(&card));

    const tinyusb_msc_sdmmc_config_t config_sdmmc = {
        .card = card
    };
    ESP_LOGI("msc", "starting msc with sdmmc");
    ESP_ERROR_CHECK(tinyusb_msc_storage_init_sdmmc(&config_sdmmc));
    #endif

    // Initialise the USB with defaults.
    tinyusb_config_t tusb_cfg = {0};
    ESP_ERROR_CHECK(tinyusb_driver_install(&tusb_cfg));

    // Initialise the USB serial interface.
    tinyusb_config_cdcacm_t acm_cfg = {
        .usb_dev = TINYUSB_USBDEV_0,
        .cdc_port = CDC_ITF,
        .rx_unread_buf_sz = 256,
        .callback_rx = &usb_callback_rx,
        #ifdef MICROPY_HW_USB_CUSTOM_RX_WANTED_CHAR_CB
        .callback_rx_wanted_char = &MICROPY_HW_USB_CUSTOM_RX_WANTED_CHAR_CB,
        #endif
        #ifdef MICROPY_HW_USB_CUSTOM_LINE_STATE_CB
        .callback_line_state_changed = &MICROPY_HW_USB_CUSTOM_LINE_STATE_CB,
        #endif
        #ifdef MICROPY_HW_USB_CUSTOM_LINE_CODING_CB
        .callback_line_coding_changed = &MICROPY_HW_USB_CUSTOM_LINE_CODING_CB,
        #endif
    };
    ESP_ERROR_CHECK(tusb_cdc_acm_init(&acm_cfg));

    ESP_ERROR_CHECK(esp_tusb_init_console(CDC_ITF));

    ESP_LOGI("usb", "inited");
}

void usb_tx_strn(const char *str, size_t len) {
    // Write out the data to the CDC interface, but only while the USB host is connected.
    uint64_t timeout = esp_timer_get_time() + (uint64_t)(MICROPY_HW_USB_CDC_TX_TIMEOUT_MS * 1000);
    while (tud_cdc_n_connected(CDC_ITF) && len && esp_timer_get_time() < timeout) {
        size_t l = tinyusb_cdcacm_write_queue(CDC_ITF, (uint8_t *)str, len);
        str += l;
        len -= l;
        tud_cdc_n_write_flush(CDC_ITF);
    }
}

#endif // CONFIG_USB_OTG_SUPPORTED && !CONFIG_ESP_CONSOLE_USB_CDC && !CONFIG_ESP_CONSOLE_USB_SERIAL_JTAG
