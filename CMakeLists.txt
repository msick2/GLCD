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

# ----- FreeRTOS-Kernel (Raspberry Pi fork) -----
# Fetched on first configure. Single-core scheduler runs on Core 1 only;
# Core 0 stays bare-metal and must not call any FreeRTOS APIs.
if(MCU_TARGET STREQUAL "pico")
    include(FetchContent)
    FetchContent_Declare(
        freertos_kernel
        GIT_REPOSITORY https://github.com/raspberrypi/FreeRTOS-Kernel.git
        GIT_TAG        main
        GIT_SHALLOW    TRUE
    )
    FetchContent_GetProperties(freertos_kernel)
    if(NOT freertos_kernel_POPULATED)
        message(STATUS "Fetching FreeRTOS-Kernel (raspberrypi fork)...")
        FetchContent_Populate(freertos_kernel)
    endif()
    set(FREERTOS_KERNEL_PATH ${freertos_kernel_SOURCE_DIR})
    set(FREERTOS_CONFIG_FILE_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}/src/freertos_config)
    include(${FREERTOS_KERNEL_PATH}/portable/ThirdParty/GCC/RP2040/FreeRTOS_Kernel_import.cmake)
endif()

set(SRC_APP  ${CMAKE_CURRENT_LIST_DIR}/src/app/main.c)
set(SRC_LCD  ${CMAKE_CURRENT_LIST_DIR}/src/lcd/lcd_st75256.c)
set(SRC_UI
    ${CMAKE_CURRENT_LIST_DIR}/src/ui/font5x7.c
    ${CMAKE_CURRENT_LIST_DIR}/src/ui/draw.c
    ${CMAKE_CURRENT_LIST_DIR}/src/ui/sim.c
    ${CMAKE_CURRENT_LIST_DIR}/src/ui/ui.c
)
set(SRC_PWM
    ${CMAKE_CURRENT_LIST_DIR}/src/pwm/buck_pwm.c
)
set(SRC_TASKS
    ${CMAKE_CURRENT_LIST_DIR}/src/tasks/core1_entry.c
    ${CMAKE_CURRENT_LIST_DIR}/src/tasks/lcd_task.c
    ${CMAKE_CURRENT_LIST_DIR}/src/tasks/pwm_task.c
)

if(MCU_TARGET STREQUAL "pico")
    set(SRC_PORT
        ${CMAKE_CURRENT_LIST_DIR}/src/port/pico/hal_spi_pico.c
        ${CMAKE_CURRENT_LIST_DIR}/src/port/pico/hal_gpio_pico.c
        ${CMAKE_CURRENT_LIST_DIR}/src/port/pico/hal_delay_pico.c
        ${CMAKE_CURRENT_LIST_DIR}/src/port/pico/hal_pwm_pico.c
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
    ${SRC_PWM}
    ${SRC_TASKS}
    ${SRC_PORT}
)

target_include_directories(glcd PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}/src/app
    ${CMAKE_CURRENT_LIST_DIR}/src/hal
    ${CMAKE_CURRENT_LIST_DIR}/src/lcd
    ${CMAKE_CURRENT_LIST_DIR}/src/ui
    ${CMAKE_CURRENT_LIST_DIR}/src/pwm
    ${CMAKE_CURRENT_LIST_DIR}/src/tasks
    ${CMAKE_CURRENT_LIST_DIR}/src/freertos_config
    ${CMAKE_CURRENT_LIST_DIR}/src/port/${MCU_TARGET}
)

if(MCU_TARGET STREQUAL "pico")
    target_link_libraries(glcd PRIVATE
        pico_stdlib
        pico_multicore
        hardware_spi
        hardware_gpio
        hardware_pwm
        hardware_clocks
        FreeRTOS-Kernel-Heap4
    )
    pico_enable_stdio_usb(glcd 1)
    pico_enable_stdio_uart(glcd 0)
    pico_add_extra_outputs(glcd)
endif()
