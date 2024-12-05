freeze("$(PORT_DIR)/modules")
include("$(MPY_DIR)/extmod/asyncio")

# Useful networking-related packages.
require("bundle-networking")

# Require some micropython-lib modules.
require("aioespnow")
require("onewire")
require("upysh")
require("aiohttp")
require("stat")
# require("dht")
# require("ds18x20")
# require("neopixel")
# require("umqtt.simple")
# require("umqtt.robust")
