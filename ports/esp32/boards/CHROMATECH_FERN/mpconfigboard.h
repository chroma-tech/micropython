#ifndef MICROPY_HW_BOARD_NAME
// Can be set by mpconfigboard.cmake.
#define MICROPY_HW_BOARD_NAME "Chroma.tech Fern"
#endif
#define MICROPY_HW_MCU_NAME "ESP32S3"

#define MICROPY_PY_MACHINE_DAC (0)
#define MICROPY_PY_MACHINE_I2S_MCK (1)

#define MICROPY_HW_ENABLE_UART_REPL (0)

#define MICROPY_HW_I2C0_SCL (43)
#define MICROPY_HW_I2C0_SDA (44)
