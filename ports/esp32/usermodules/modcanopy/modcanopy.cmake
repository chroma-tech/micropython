# Create an INTERFACE library for our CPP module.
add_library(usermod_canopy INTERFACE)

target_sources(usermod_canopy INTERFACE
    ${CMAKE_CURRENT_LIST_DIR}/modcanopy.c
    ${CMAKE_CURRENT_LIST_DIR}/modcanopy.cpp)

# Add the current directory as an include directory.
target_include_directories(usermod_canopy INTERFACE
    ${CMAKE_CURRENT_LIST_DIR}
)

# Link our INTERFACE library to the usermod target.
target_link_libraries(usermod INTERFACE usermod_canopy)
