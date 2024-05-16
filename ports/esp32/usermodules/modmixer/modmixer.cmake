# Create an INTERFACE library for our CPP module.
add_library(usermod_mixer INTERFACE)

target_sources(usermod_mixer INTERFACE
  ${CMAKE_CURRENT_LIST_DIR}/modmixer.c
  ${CMAKE_CURRENT_LIST_DIR}/modmixer-bindings.c
)

# Add the current directory as an include directory.
target_include_directories(usermod_mixer INTERFACE
  ${CMAKE_CURRENT_LIST_DIR}
)

# Link our INTERFACE library to the usermod target.
target_link_libraries(usermod INTERFACE usermod_mixer)
