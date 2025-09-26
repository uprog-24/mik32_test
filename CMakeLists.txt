cmake_minimum_required(VERSION 3.22)
project(mik_build C)

# Указываем toolchain перед вызовом project()
set(CMAKE_TOOLCHAIN_FILE "${CMAKE_CURRENT_SOURCE_DIR}/toolchain.cmake" CACHE STRING "Toolchain file" FORCE)

# set(CMAKE_C_COMPILER
#     ) # Укажите ваш кросс-компилятор

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(EXECUTABLE ${CMAKE_PROJECT_NAME})

set(CPU_PARAMETERS
    -mcmodel=medlow
    -march=rv32imc_zicsr_zifencei
    -mabi=ilp32
    -msmall-data-limit=8
)


set(MCU_LINKER_SCRIPT "${CMAKE_CURRENT_SOURCE_DIR}/shared/ldscripts/spifi.ld")
set(STARTUP_SCRIPT "${CMAKE_CURRENT_SOURCE_DIR}/shared/runtime/crt0.S")


# Пути к HAL и SDK
set(HAL_INC_DIR
    mik32_hal_main/core/Include
    mik32_hal_main/peripherals/Include
    mik32_hal_main/utilities/Include

    shared/include
    shared/periphery
    shared/libs
)
# set(HAL_SRC_DIR
#     mik32_hal_main/core/Source
#     mik32_hal_main/peripherals/Source
#     mik32_hal_main/utilities/Source

#     shared/periphery
# )

set(PROJECT_INCLUDE_DIRECTORIES
)

set(PROJECT_SOURCES
    # src/main.c
    # src/timer_16_blink_irq.c
    src/timer_16_pwm_led_irq.c
    # src/usart.c
)

file(GLOB_RECURSE HAL_SRC
    mik32_hal_main/core/Source/*.c
    mik32_hal_main/peripherals/Source/*.c
    mik32_hal_main/utilities/Source/*.c
    shared/periphery/*.c
    shared/libs/*.c
)



# Добавить все .c файлы проекта и HAL
# file(
#     # GLOB
#     GLOB_RECURSE
#     SRC_FILES
#     src/*.c
#     ${HAL_SRC_DIR}/*.c
# )

# set(SRC_FILES src/main.c ${CORE_SRC} ${PERIPH_SRC} ${UTIL_SRC} ${SHARED_SRC})

# message(STATUS "SRC_FILES: ${SRC_FILES}")

add_executable(${EXECUTABLE}
    ${STARTUP_SCRIPT}
    ${PROJECT_SOURCES}
    ${HAL_SRC}
)

# Указываем, что у таргета расширение .elf
set_target_properties(${EXECUTABLE} PROPERTIES SUFFIX ".elf")


# Макросы (defines)
target_compile_definitions(${EXECUTABLE} PRIVATE
    HAS_RV32IMC HAS_RISC_V)


include_directories(${HAL_INC_DIR})
# include_directories(${PROJECT_SOURCE_DIR}/include) # ваши заголовочные файлы

# Задать linker script
# set(LD_SCRIPT
#     "${CMAKE_CURRENT_SOURCE_DIR}/shared/ldscripts/ram.ld"
# )
# set(CMAKE_EXE_LINKER_FLAGS "-T${LD_SCRIPT}")
# set(CMAKE_EXE_LINKER_FLAGS "-T${LD_SCRIPT} -L${CMAKE_CURRENT_SOURCE_DIR}/shared/ldscripts/sections.lds")


# Флаги компиляции и линковки - настройте в соответствии с SDK и вашим устройством

target_compile_options(${EXECUTABLE} PRIVATE
    ${CPU_PARAMETERS}
    -fdata-sections -ffunction-sections # Common flags

    $<$<COMPILE_LANGUAGE:CXX>:
    -fno-rtti -fno-exceptions -fno-threadsafe-statics
    -Wsuggest-override> # C++
    $<$<CONFIG:Debug>:-O0 -g3 -ggdb> # Debug
    $<$<CONFIG:Release>:-Os -g0> # Release

)

# target_link_options(${EXECUTABLE} PRIVATE
#     -nostartfiles
#     -nostdlib
#     -Wl,--gc-sections
# )

# Флаги для линкера
target_link_options(${EXECUTABLE} PRIVATE
    ${CPU_PARAMETERS}
    -T${MCU_LINKER_SCRIPT}
    --specs=nano.specs
    -Wl,-Map=${CMAKE_PROJECT_NAME}.map
    -Wl,--gc-sections
    -Wl,--start-group
    -lc -lm -lstdc++ -lsupc++
    -Wl,--end-group
    -Wl,--print-memory-usage
)

add_custom_command(TARGET ${EXECUTABLE} POST_BUILD
    COMMAND ${CMAKE_SIZE} $<TARGET_FILE:${EXECUTABLE}>)

# Convert output to hex and binary
add_custom_command(TARGET ${EXECUTABLE} POST_BUILD
    COMMAND ${CMAKE_OBJCOPY} -O ihex $<TARGET_FILE:${EXECUTABLE}>
    ${EXECUTABLE}.hex
    COMMAND ${CMAKE_OBJCOPY} -O binary $<TARGET_FILE:${EXECUTABLE}>
    ${EXECUTABLE}.bin)

# Таргет для копирования файлов режима
add_custom_target(copy_outputs ALL
    # Копируем HEX/BIN из папки текущего режима и compile_commands.json
    COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_FILE_DIR:${EXECUTABLE}>/${EXECUTABLE}.hex ${CMAKE_CURRENT_SOURCE_DIR}/build
    COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_FILE_DIR:${EXECUTABLE}>/${EXECUTABLE}.bin ${CMAKE_CURRENT_SOURCE_DIR}/build
    COMMAND ${CMAKE_COMMAND} -E copy_if_different ${CMAKE_BINARY_DIR}/compile_commands.json ${CMAKE_CURRENT_SOURCE_DIR}/build

    COMMENT "Copying HEX, BIN and compile_commands.json to top-level build"
)

# Зависимость от основной сборки
add_dependencies(copy_outputs ${EXECUTABLE})

