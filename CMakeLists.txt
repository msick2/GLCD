cmake_minimum_required(VERSION 3.13)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(MCU_TARGET "pico" CACHE STRING "Target MCU port (pico | mkv31)")
set(PICO_BOARD pico CACHE STRING "Pico board type")

if(MCU_TARGET STREQUAL "pico")
    include(pico_sdk_import.cmake)
endif()

project(glcd C CXX ASM)

if(MCU_TARGET STREQUAL "pico")
    pico_sdk_init()
endif()

set(SRC_APP  ${CMAKE_CURRENT_LIST_DIR}/src/app/main.c)
set(SRC_LCD  ${CMAKE_CURRENT_LIST_DIR}/src/lcd/lcd_st75256.c)
set(SRC_UI
    ${CMAKE_CURRENT_LIST_DIR}/src/ui/font5x7.c
    ${CMAKE_CURRENT_LIST_DIR}/src/ui/draw.c
    ${CMAKE_CURRENT_LIST_DIR}/src/ui/sim.c
    ${CMAKE_CURRENT_LIST_DIR}/src/ui/ui.c
)

if(MCU_TARGET STREQUAL "pico")
    set(SRC_PORT
        ${CMAKE_CURRENT_LIST_DIR}/src/port/pico/hal_spi_pico.c
        ${CMAKE_CURRENT_LIST_DIR}/src/port/pico/hal_gpio_pico.c
        ${CMAKE_CURRENT_LIST_DIR}/src/port/pico/hal_delay_pico.c
    )
elseif(MCU_TARGET STREQUAL "mkv31")
    file(GLOB SRC_PORT ${CMAKE_CURRENT_LIST_DIR}/src/port/mkv31/*.c)
else()
    message(FATAL_ERROR "Unknown MCU_TARGET: ${MCU_TARGET}")
endif()

add_executable(glcd
    ${SRC_APP}
    ${SRC_LCD}
    ${SRC_UI}
    ${SRC_PORT}
)

target_include_directories(glcd PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}/src/app
    ${CMAKE_CURRENT_LIST_DIR}/src/hal
    ${CMAKE_CURRENT_LIST_DIR}/src/lcd
    ${CMAKE_CURRENT_LIST_DIR}/src/ui
    ${CMAKE_CURRENT_LIST_DIR}/src/port/${MCU_TARGET}
)

if(MCU_TARGET STREQUAL "pico")
    target_link_libraries(glcd PRIVATE
        pico_stdlib
        hardware_spi
        hardware_gpio
    )
    pico_enable_stdio_usb(glcd 1)
    pico_enable_stdio_uart(glcd 0)
    pico_add_extra_outputs(glcd)
endif()
