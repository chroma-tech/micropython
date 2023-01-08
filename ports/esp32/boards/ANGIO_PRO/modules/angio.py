from machine import Pin, SPI
import network
import random

s = SPI(2, mosi=Pin(11), miso=Pin(13), sck=Pin(12), cs=Pin(10))
l = network.LAN(spi=s, int=Pin(9), power=Pin(8), phy_type=network.PHY_DM9051)

# configure a default mac address if we don't have one yet
if l.config('mac') == b'\xff\xff\xff\xff\xff\xff':
    mac_addr = "02:00:00:%02x:%02x:%02x" % (random.randint(0, 255),
                             random.randint(0, 255),
                             random.randint(0, 255))
    l.config(mac=mac_addr)
