# Create an INTERFACE library for our CPP module.
add_library(usermod_canopy INTERFACE)

target_sources(usermod_canopy INTERFACE
    ${CMAKE_CURRENT_LIST_DIR}/modcanopy.cpp
    ${CMAKE_CURRENT_LIST_DIR}/modcanopy.c
    ${CMAKE_CURRENT_LIST_DIR}/fastled_shims.cpp)

# Add the current directory as an include directory.
target_include_directories(usermod_canopy INTERFACE
    ${CMAKE_CURRENT_LIST_DIR}
)

target_compile_options(usermod INTERFACE
    -DFASTLED_NO_MCU
    -DFASTLED_NO_PINMAP
    -DHAS_HARDWARE_PIN_SUPPORT
    -DFASTLED_FORCE_SOFTWARE_SPI)

# Link our INTERFACE library to the usermod target.
target_link_libraries(usermod INTERFACE usermod_canopy)
