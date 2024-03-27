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

D1 = const(1)
D2 = const(2)
D3 = const(3)
D4 = const(46)
D5 = const(9)
D6 = const(10)
D7 = const(11)
D8 = const(12)

PatternRainbow = "CTP-eyJpZCI6IjYwMzY4MDllLTliNWItNDJhOS04MzE5LTFkNDA4OTU5M2JhMyIsInZlcnNpb24iOjEsIm5hbWUiOiJ0ZXN0IiwicGFsZXR0ZXMiOnsicmFpbmJvdyI6W1swLFsxLDAsMF1dLFswLjE1LFsxLDEsMF1dLFswLjMsWzAsMSwwXV0sWzAuNSxbMCwxLDFdXSxbMC43LFswLDAsMV1dLFswLjg1LFsxLDAsMV1dLFsxLFsxLDAsMF1dXSwiX2JsYWNrLXdoaXRlIjpbWzAsWzAsMCwwXV0sWzEsWzEsMSwxXV1dfSwicGFyYW1zIjp7ImNvbnN0MSI6MC4xLCJwb2ludGVyMSI6ImNvbnN0MSIsInNjYWxlIjp7InR5cGUiOiJub2lzZSIsImlucHV0cyI6eyJ2YWx1ZSI6MCwibWluIjoiY29uc3QxIiwibWF4IjoxfX0sInNwZWVkMSI6eyJ0eXBlIjoic2F3IiwiaW5wdXRzIjp7InZhbHVlIjowLjUsIm1pbiI6MCwibWF4IjoxfX19LCJsYXllcnMiOlt7ImVmZmVjdCI6InNvbGlkIiwib3BhY2l0eSI6MSwiYmxlbmQiOiJub3JtYWwiLCJwYWxldHRlIjoicmFpbmJvdyIsImlucHV0cyI6eyJvZmZzZXQiOiJzcGVlZDEifX0seyJlZmZlY3QiOiJncmFkaWVudCIsIm9wYWNpdHkiOjAuNSwiYmxlbmQiOiJub3JtYWwiLCJwYWxldHRlIjoicmFpbmJvdyIsImlucHV0cyI6eyJvZmZzZXQiOiJzcGVlZDEiLCJzaXplIjowLjUsInJvdGF0aW9uIjowfX1dfQ"
PatternPurpProg = "CTP-eyJpZCI6ImFjZDgxZGZlLTFmMjMtNGMzMS05NDAxLTg3NzdjZWM4MDQ2YSIsInZlcnNpb24iOjEsIm5hbWUiOiJOZXcgUGF0dGVybiIsInBhbGV0dGVzIjp7IlBhbGV0dGUxIjpbWzAsWzAuNzE3NjQ3MDU4ODIzNTI5NCwwLDFdXSxbMC41MyxbMC4xODgyMzUyOTQxMTc2NDcwNiwwLDAuMjcwNTg4MjM1Mjk0MTE3NjNdXSxbMSxbMC40NTA5ODAzOTIxNTY4NjI3NSwwLDAuNzgwMzkyMTU2ODYyNzQ1MV1dXSwiUGFsZXR0ZTIiOltbMSxbMSwxLDFdXV19LCJwYXJhbXMiOnsicHJvZ3Jlc3MiOjB9LCJsYXllcnMiOlt7ImVmZmVjdCI6ImdyYWRpZW50Iiwib3BhY2l0eSI6MSwiYmxlbmQiOiJub3JtYWwiLCJwYWxldHRlIjoiUGFsZXR0ZTEiLCJpbnB1dHMiOnsib2Zmc2V0Ijp7InR5cGUiOiJyc2F3IiwiaW5wdXRzIjp7InZhbHVlIjowLjUsIm1pbiI6MCwibWF4IjoxfX0sInNpemUiOjAuMTEsInJvdGF0aW9uIjowfX0seyJlZmZlY3QiOiJjaGFzZXIiLCJvcGFjaXR5IjoxLCJibGVuZCI6Im1hc2siLCJwYWxldHRlIjoiUGFsZXR0ZTIiLCJpbnB1dHMiOnsib2Zmc2V0IjoicHJvZ3Jlc3MiLCJzaXplIjowLjV9fV19"


def sdcard():
    return SDCard(
        width=4, clk=SD_CLK, cmd=SD_CMD, d0=SD_D0, d1=SD_D1, d2=SD_D2, d3=SD_D3
    )


def mount_sdcard(path="/sd"):
    sd = sdcard()
    os.mount(sd, path)
