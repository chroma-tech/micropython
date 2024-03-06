#pragma once

#include "driver/gpio.h"
#include "esp_attr.h"
#include "esp_check.h"
#include "esp_heap_caps.h"
#include "esp_intr_alloc.h"
#include "esp_pm.h"
#include "esp_private/gdma.h"
#include "esp_rom_gpio.h"
#include "hal/dma_types.h"
#include "hal/gpio_hal.h"
#include "hal/lcd_hal.h"
#include "hal/lcd_ll.h"
#include "soc/lcd_periph.h"
#include "soc/rtc.h"
#include "soc/soc_caps.h"
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "esp_heap_caps.h"
#include "esp_lcd_panel_io.h"
#include "esp_lcd_panel_ops.h"
#include "esp_log.h"
#include "esp_memory_utils.h"
#include "esp_timer.h"
#include "soc/gdma_reg.h"

#ifndef NUMSTRIPS
#define NUMSTRIPS 16
#endif

#define AA (0x00AA00AAL)
#define CC (0x0000CCCCL)
#define FF (0xF0F0F0F0L)
#define FF2 (0x0F0F0F0FL)

#define MIN(a, b) (((a) < (b)) ? (a) : (b))
#define __OFFSET (24 * 3 * 2 * 2 * 2)

#ifdef COLOR_ORDER_GRBW
#define _p_r 1
#define _p_g 0
#define _p_b 2
#define _nb_components 4
#elif COLOR_ORDER_RGBW
#define _p_r 0
#define _p_g 1
#define _p_b 2
#define _nb_components 3
#elif COLOR_ORDER_RGBW
#define _p_r 0
#define _p_g 2
#define _p_b 1
#define _nb_components 3
#elif COLOR_ORDER_RGBW
#define _p_r 2
#define _p_g 0
#define _p_b 1
#define _nb_components 3
#elif COLOR_ORDER_RGBW
#define _p_r 2
#define _p_g 1
#define _p_b 0
#define _nb_components 3
#elif COLOR_ORDER_RGBW
#define _p_r 1
#define _p_g 2
#define _p_b 0
#define _nb_components 3
#elif COLOR_ORDER_RGBW
#define _p_r 1
#define _p_g 0
#define _p_b 2
#define _nb_components 3
#else
#define _p_r 1
#define _p_g 0
#define _p_b 2
#define _nb_components 3
#endif

#define LCD_DRIVER_PSRAM_DATA_ALIGNMENT 64
#define CLOCKLESS_PIXEL_CLOCK_HZ (24 * 100 * 1000)

static void IRAM_ATTR transpose16x1_noinline2(unsigned char *A, uint16_t *B) {

  uint32_t x, y, x1, y1, t;

  y = *(unsigned int *)(A);
#if NUMSTRIPS > 4
  x = *(unsigned int *)(A + 4);
#else
  x = 0;
#endif

#if NUMSTRIPS > 8
  y1 = *(unsigned int *)(A + 8);
#else
  y1 = 0;
#endif
#if NUMSTRIPS > 12
  x1 = *(unsigned int *)(A + 12);
#else
  x1 = 0;
#endif

  // pre-transform x
#if NUMSTRIPS > 4
  t = (x ^ (x >> 7)) & AA;
  x = x ^ t ^ (t << 7);
  t = (x ^ (x >> 14)) & CC;
  x = x ^ t ^ (t << 14);
#endif
#if NUMSTRIPS > 12
  t = (x1 ^ (x1 >> 7)) & AA;
  x1 = x1 ^ t ^ (t << 7);
  t = (x1 ^ (x1 >> 14)) & CC;
  x1 = x1 ^ t ^ (t << 14);
#endif
  // pre-transform y
  t = (y ^ (y >> 7)) & AA;
  y = y ^ t ^ (t << 7);
  t = (y ^ (y >> 14)) & CC;
  y = y ^ t ^ (t << 14);
#if NUMSTRIPS > 8
  t = (y1 ^ (y1 >> 7)) & AA;
  y1 = y1 ^ t ^ (t << 7);
  t = (y1 ^ (y1 >> 14)) & CC;
  y1 = y1 ^ t ^ (t << 14);
#endif
  // final transform
  t = (x & FF) | ((y >> 4) & FF2);
  y = ((x << 4) & FF) | (y & FF2);
  x = t;

  t = (x1 & FF) | ((y1 >> 4) & FF2);
  y1 = ((x1 << 4) & FF) | (y1 & FF2);
  x1 = t;

  *((uint16_t *)(B)) =
      (uint16_t)(((x & 0xff000000) >> 8 | ((x1 & 0xff000000))) >> 16);
  *((uint16_t *)(B + 3)) =
      (uint16_t)(((x & 0xff0000) >> 16 | ((x1 & 0xff0000) >> 8)));
  *((uint16_t *)(B + 6)) =
      (uint16_t)(((x & 0xff00) | ((x1 & 0xff00) << 8)) >> 8);
  *((uint16_t *)(B + 9)) = (uint16_t)((x & 0xff) | ((x1 & 0xff) << 8));
  *((uint16_t *)(B + 12)) =
      (uint16_t)(((y & 0xff000000) >> 8 | ((y1 & 0xff000000))) >> 16);
  *((uint16_t *)(B + 15)) =
      (uint16_t)(((y & 0xff0000) | ((y1 & 0xff0000) << 8)) >> 16);
  *((uint16_t *)(B + 18)) =
      (uint16_t)(((y & 0xff00) | ((y1 & 0xff00) << 8)) >> 8);
  *((uint16_t *)(B + 21)) = (uint16_t)((y & 0xff) | ((y1 & 0xff) << 8));
}

#ifndef traceISR_EXIT_TO_SCHEDULER
        #define traceISR_EXIT_TO_SCHEDULER()
    #endif

class I2SClocklessLedDriveresp32S3 {
  typedef union {
    uint8_t bytes[16];
    uint32_t shorts[8];
    uint32_t raw[2];
  } Lines;

  enum class ColorArrangement {
    ORDER_GRBW,
    ORDER_RGB,
    ORDER_RBG,
    ORDER_GRB,
    ORDER_GBR,
    ORDER_BRG,
    ORDER_BGR,
  };

  esp_lcd_panel_io_handle_t io_handle = nullptr;
  esp_lcd_i80_bus_handle_t i80_bus = nullptr;
  SemaphoreHandle_t xsemi = nullptr;
  volatile bool isDisplaying = false;
  volatile bool iswaiting = false;

  static bool IRAM_ATTR flush_ready(esp_lcd_panel_io_handle_t panel_io,
                                    esp_lcd_panel_io_event_data_t *edata,
                                    void *user_ctx) {
    // printf("we're here");
    auto driver = (I2SClocklessLedDriveresp32S3 *)user_ctx;
    driver->isDisplaying = false;
    if (driver->iswaiting) {
      portBASE_TYPE HPTaskAwoken = 0;
      driver->iswaiting = false;
      xSemaphoreGiveFromISR(driver->xsemi, &HPTaskAwoken);
      if (HPTaskAwoken == pdTRUE)
        portYIELD_FROM_ISR(HPTaskAwoken);
    }
    return false;
  }

  // #ifdef __cplusplus
  // extern "C" {
  // #endif
  void _initled(uint8_t *leds, int *pins, int numstrip, int NUM_LED_PER_STRIP) {

    esp_lcd_i80_bus_config_t bus_config;

    bus_config.clk_src = LCD_CLK_SRC_PLL160M;
    bus_config.dc_gpio_num = 0;
    bus_config.wr_gpio_num = 0;
    for (int i = 0; i < numstrip; i++) {
      bus_config.data_gpio_nums[i] = pins[i];
    }
    if (numstrip < 16) {
      for (int i = numstrip; i < 16; i++) {
        bus_config.data_gpio_nums[i] = 0;
      }
    }
    bus_config.bus_width = 16;
    bus_config.max_transfer_bytes =
        _nb_components * NUM_LED_PER_STRIP * 8 * 3 * 2 + __OFFSET;
    bus_config.psram_trans_align = LCD_DRIVER_PSRAM_DATA_ALIGNMENT;
    bus_config.sram_trans_align = 4;

    ESP_ERROR_CHECK(esp_lcd_new_i80_bus(&bus_config, &i80_bus));

    esp_lcd_panel_io_i80_config_t io_config;

    io_config.cs_gpio_num = -1;
    io_config.pclk_hz = CLOCKLESS_PIXEL_CLOCK_HZ;
    io_config.trans_queue_depth = 1;
    io_config.dc_levels = {
        .dc_idle_level = 0,
        .dc_cmd_level = 0,
        .dc_dummy_level = 0,
        .dc_data_level = 1,
    };
    //.on_color_trans_done = flush_ready,
    // .user_ctx = nullptr,
    io_config.lcd_cmd_bits = 0;
    io_config.lcd_param_bits = 0;

    io_config.on_color_trans_done = flush_ready;
    io_config.user_ctx = this;
    ESP_ERROR_CHECK(esp_lcd_new_panel_io_i80(i80_bus, &io_config, &io_handle));

    // reclaim GPIO0 back from the LCD peripheral. esp_lcd_new_i80_bus requires
    // wr and dc to be valid gpio and we used 0, however we don't need those
    // signals for our LED stuff so we can reclaim it here.
    esp_rom_gpio_connect_out_signal(0, SIG_GPIO_OUT_IDX, false, true);
  }
  // #ifdef __cplusplus
  // }
  // #endif

public:
  uint16_t *led_output = nullptr;
  uint8_t *ledsbuff = nullptr;
  int num_leds_per_strip;
  int _numstrips;

  void initled(uint8_t *leds, int *pins, int numstrip, int NUM_LED_PER_STRIP) {
    auto buffer_size =
        8 * _nb_components * NUM_LED_PER_STRIP * 3 * 2 + __OFFSET;

    led_output = (uint16_t *)heap_caps_aligned_alloc(
        4, buffer_size, MALLOC_CAP_DMA | MALLOC_CAP_8BIT);
    memset(led_output, 0, buffer_size);

    for (int i = 0; i < NUM_LED_PER_STRIP * _nb_components * 8; i++) {
      led_output[3 * i + 1] =
          0xFFFF; // the +1 because it's like the first value doesnt get pushed
                  // do not ask me why for now
    }

    ledsbuff = leds;
    _numstrips = numstrip;
    num_leds_per_strip = NUM_LED_PER_STRIP;
    _initled(leds, pins, numstrip, NUM_LED_PER_STRIP);
  }

  void deinit() {
    if (io_handle) {
      ESP_ERROR_CHECK(esp_lcd_panel_io_del(io_handle));
      ESP_ERROR_CHECK(esp_lcd_del_i80_bus(i80_bus));
      io_handle = nullptr;
    }
    if (led_output) {
      free(led_output);
      led_output = nullptr;
    }
  }

  I2SClocklessLedDriveresp32S3() { xsemi = xSemaphoreCreateBinary(); }
  ~I2SClocklessLedDriveresp32S3() { deinit(); }

  void transposeAll(uint16_t *ledoutput) {
    uint16_t ledToDisplay = 0;
    Lines secondPixel[_nb_components];
    uint16_t *buff =
        ledoutput + 2; //+1 pour le premier empty +1 pour le 1 systématique
    uint16_t jump = num_leds_per_strip * _nb_components;
    for (int j = 0; j < num_leds_per_strip; j++) {
      uint8_t *poli = ledsbuff + ledToDisplay * _nb_components;
      for (int i = 0; i < _numstrips; i++) {
        secondPixel[_p_g].bytes[i] = *(poli + 1);
        secondPixel[_p_r].bytes[i] = *(poli + 0);
        secondPixel[_p_b].bytes[i] = *(poli + 2);
        if (_nb_components > 3)
          secondPixel[3].bytes[i] = *(poli + 3);
        poli += jump;
      }
      ledToDisplay++;
      transpose16x1_noinline2(secondPixel[0].bytes, buff);
      buff += 24;
      transpose16x1_noinline2(secondPixel[1].bytes, buff);
      buff += 24;
      transpose16x1_noinline2(secondPixel[2].bytes, buff);
      buff += 24;
      if (_nb_components > 3) {
        transpose16x1_noinline2(secondPixel[3].bytes, buff);
        buff += 24;
      }
    }
  }

  void show() {
    if (isDisplaying) {
      iswaiting = true;
      xSemaphoreTake(xsemi, portMAX_DELAY);
    }
    isDisplaying = true;
    transposeAll(led_output);
    esp_lcd_panel_io_tx_color(io_handle, 0x2C, led_output,
                        _nb_components * num_leds_per_strip * 8 * 3 * 2 +
                            __OFFSET);
  }
};