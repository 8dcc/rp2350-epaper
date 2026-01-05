cmake_minimum_required(VERSION 3.13)

# Must be included before the project.
include(pico_sdk_import.cmake)

project(rp2350_epaper C CXX ASM)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

# Initialize the Pico SDK
pico_sdk_init()

# ------------------------------------------------------------------------------

# Build the E-Paper driver as a static library
add_library(epaper_2in9 STATIC
    src/epaper_2in9.c
)

# Set include directories for the library
target_include_directories(epaper_2in9 PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

# Link required Pico SDK libraries to the static library
target_link_libraries(epaper_2in9 PUBLIC
    pico_stdlib
    hardware_spi
)

# ------------------------------------------------------------------------------

# Build the example executable
add_executable(rp2350_epaper_example
    examples/main.c
)

# Link the example against the library
target_link_libraries(rp2350_epaper_example
    epaper_2in9
)

# Enable USB output, disable UART output
pico_enable_stdio_usb(rp2350_epaper_example 1)
pico_enable_stdio_uart(rp2350_epaper_example 0)

# Create map/bin/hex/uf2 files
pico_add_extra_outputs(rp2350_epaper_example)
