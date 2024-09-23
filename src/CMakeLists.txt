add_executable(${ProjectName}
    main.cpp
)

add_library(led led.c)
target_link_libraries(led pico_stdlib hardware_pwm)

target_include_directories(${ProjectName} PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}
)

target_link_libraries(${ProjectName} 
    pico_stdlib 
    FreeRTOS-Kernel-Heap4
    led
    )

pico_add_extra_outputs(${ProjectName})

# Disable usb output, enable uart output
pico_enable_stdio_usb(${PROJECT_NAME} 0)
pico_enable_stdio_uart(${PROJECT_NAME} 1)