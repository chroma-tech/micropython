from micropython import const
from machine import Pin, SDCard
import os

LED1_DATA = const(8)
LED1_CLOCK = const(18)
LED2_DATA = const(17)
# led2 clock depends on solder bridge
LED2_CLOCK_DEFAULT = LED1_CLOCK
LED2_CLOCK_ALT = const(0)
LED2_CLOCK = LED2_CLOCK_DEFAULT

I2S_BCK = const(47)
I2S_MCK = const(48)
I2S_WS = const(14)
I2S_SDOUT = const(21)
I2S_SDIN = const(13)

SD_CMD = const(6)
SD_CLK = const(7)
SD_D0 = const(15)
SD_D1 = const(16)
SD_D2 = const(4)
SD_D3 = const(5)

NFC_SCK = const(38)
NFC_MISO = const(39)
NFC_MOSI = const(40)
NFC_NSS = const(41)
NFC_RST = const(42)
NFC_BUSY = const(45)

I2C_SDA = const(44)
I2C_SCL = const(43)


def sdcard():
    return SDCard(
        width=4, clk=SD_CLK, cmd=SD_CMD, d0=SD_D0, d1=SD_D1, d2=SD_D2, d3=SD_D3
    )


def mount_sdcard(path="/sd"):
    sd = sdcard()
    os.mount(sd, path)
