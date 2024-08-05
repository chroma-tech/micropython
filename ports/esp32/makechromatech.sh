# add ESP_IDF to path if it isn't there already
# if [ -z "$ESP_IDF" ]; then
#   source ~/esp/esp-idf-5/export.sh
# fi
make BOARD=CHROMATECH_FERN USER_C_MODULES=`pwd`/usermodules/usermodules.cmake PORT=/dev/cu.usbmodem4201 deploy
# make BOARD=CHROMATECH_FERN USER_C_MODULES=`pwd`/usermodules/usermodules.cmake 