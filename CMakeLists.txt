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
add_library(epaper_display STATIC
    src/epaper_display.c
    src/epaper_display_2in9.c
    src/epaper_display_utils.c
)

# Link required Pico SDK libraries to the static library
target_link_libraries(epaper_display PUBLIC
    pico_stdlib
    hardware_spi
)

# ------------------------------------------------------------------------------

# Build the example executable
add_executable(rp2350_epaper_example
    examples/main.c
)

# Set include directories, so the example can access the library's headers.
target_include_directories(rp2350_epaper_example PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

# Link the example against the library
target_link_libraries(rp2350_epaper_example
    epaper_display
)

# Enable USB output, disable UART output
pico_enable_stdio_usb(rp2350_epaper_example 1)
pico_enable_stdio_uart(rp2350_epaper_example 0)

# Create map/bin/hex/uf2 files
pico_add_extra_outputs(rp2350_epaper_example)
