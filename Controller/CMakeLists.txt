# Set minimum required version of CMake
cmake_minimum_required(VERSION 3.13)

# Include build functions from Pico SDK
include($ENV{PICO_SDK_PATH}/external/pico_sdk_import.cmake)

set(CMAKE_C_COMPILER "/usr/bin/clang")
set(CMAKE_CXX_COMPILER "/usr/bin/clang++")

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

set(PICO_CXX_ENABLE_EXCEPTIONS 1)


add_compile_options(-Wall -Wno-unused-function)

# Creates a pico-sdk subdirectory in our project for the libraries
pico_sdk_init()

include(lib/RF24/CMakeLists.txt)

# Set name of project (as PROJECT_NAME) and C/C++ standards
project(NCPNavSystem)

# Tell CMake where to find the executable source file
add_executable(${PROJECT_NAME})
target_sources(${PROJECT_NAME} PRIVATE main.cc transmission.cc)

# Link to pico_stdlib (gpio, time, etc. functions)
target_link_libraries(${PROJECT_NAME} 
	PRIVATE
		pico_stdlib
		hardware_spi
		hardware_pwm
		RF24
)

# Create map/bin/hex/uf2 files
pico_add_extra_outputs(${PROJECT_NAME})

# Enable usb output, disable uart output
pico_enable_stdio_usb(${PROJECT_NAME} 1)
pico_enable_stdio_uart(${PROJECT_NAME} 0)
