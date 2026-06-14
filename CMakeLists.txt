cmake_minimum_required(VERSION 3.20)

include($ENV{PICO_SDK_PATH}/external/pico_sdk_import.cmake)

project(joybus-cli C CXX ASM)

pico_sdk_init()

# Pull in libjoybus as a dependency
set(JOYBUS_BACKEND_RP2XXX TRUE)
add_subdirectory(lib/libjoybus ${CMAKE_BINARY_DIR}/_deps/libjoybus-build)

# Create the executable
add_executable(joybus-cli src/main.c)

# Link with the standard library, PWM (for the external clock), and libjoybus
target_link_libraries(joybus-cli pico_stdlib hardware_pwm joybus)

# Use USB for the machine protocol, no UART
pico_enable_stdio_usb(joybus-cli 1)
pico_enable_stdio_uart(joybus-cli 0)

# Generate additional binary outputs (.uf2, etc.)
pico_add_extra_outputs(joybus-cli)
