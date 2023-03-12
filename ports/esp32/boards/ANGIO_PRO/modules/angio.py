from machine import Pin, SPI
import network

s = SPI(2, mosi=Pin(11), miso=Pin(13), sck=Pin(12), cs=Pin(10))
l = network.LAN(spi=s, int=Pin(9), power=Pin(8), phy_type=network.PHY_DM9051)
