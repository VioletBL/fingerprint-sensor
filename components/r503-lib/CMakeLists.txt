set(COMPONENT_SRCS "src/r503-lib.cpp")

file(GLOB_RECURSE ARDUINO_SOURCES "arduino_lib/src/*.cpp")
list(APPEND COMPONENT_SRCS ${ARDUINO_SOURCES})

idf_component_register(SRCS ${COMPONENT_SRCS}
                       INCLUDE_DIRS "include" "arduino_lib/src"
                       REQUIRES arduino)