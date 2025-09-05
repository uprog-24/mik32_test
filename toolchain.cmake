set(CMAKE_SYSTEM_NAME Generic) # Определяем систему как Generic (не конкретная ОС)
set(CMAKE_SYSTEM_PROCESSOR riscv) # Определяем процессор 

# Настройка префикса компилятора
set(TOOLCHAIN_PREFIX "T:/Downloads/xpack-riscv-none-elf-gcc-14.2.0-3-win32-x64/xpack-riscv-none-elf-gcc-14.2.0-3/bin/riscv-none-elf-")

# Назначение компиляторов (флаги компиляции отдельно)
set(CMAKE_C_COMPILER "${TOOLCHAIN_PREFIX}gcc.exe")
set(CMAKE_ASM_COMPILER ${CMAKE_C_COMPILER})
set(CMAKE_CXX_COMPILER "${TOOLCHAIN_PREFIX}g++.exe")
# set(CMAKE_LINKER "${TOOLCHAIN_PREFIX}g++.exe")

# Утилиты
set(CMAKE_OBJCOPY "${TOOLCHAIN_PREFIX}objcopy.exe") # Нужен для создания .bin и .hex файлов
set(CMAKE_SIZE "${TOOLCHAIN_PREFIX}size.exe") # Показывает размер секций ELF-файла

# Отключаем попытку запускать исполняемые файлы при проверке компиляции
set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)
