cmake_minimum_required(VERSION 3.10)

# Name project
set(ProjectName Keijos-Retirement-Plan-Product)

# Set any variables required for importing libraries
set(FREERTOS_KERNEL_PATH ${CMAKE_CURRENT_SOURCE_DIR}/FreeRTOS-KernelV10.6.2)

set(CMAKE_CXX_STANDARD 20)

# Import those libraries
include($ENV{PICO_SDK_PATH}/external/pico_sdk_import.cmake)
include(${FREERTOS_KERNEL_PATH}/portable/ThirdParty/GCC/RP2040/FreeRTOS_Kernel_import.cmake)

# Set include dir variable
set(INCLUDE_DIR ${CMAKE_CURRENT_LIST_DIR}/inc)

set(PICO_BOARD pico_w)

set(CXX_STANDARD 20)

#add_compile_options(-Wall -Wextra -Wpedantic)

# Define project
project(${ProjectName})

# Initialize the Raspberry Pi Pico SDK
pico_sdk_init()

# Add subdirectories
add_subdirectory(src)

# if you have anything in "lib" folder then uncomment below - remember to add a CMakeLists.txt
# file to the "lib" directory
#add_subdirectory(lib)

