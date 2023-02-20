set(IDF_TARGET esp32c3)

set(CHROMATECH_BOARD_MOSS 1)

set(SDKCONFIG_DEFAULTS
    boards/sdkconfig.base
    boards/sdkconfig.ble
    boards/MOSS/sdkconfig.board
)
