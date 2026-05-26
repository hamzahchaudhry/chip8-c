cmake_minimum_required(VERSION 4.3.2)

project(chip8_emulator LANGUAGES C)

set(CMAKE_C_STANDARD 23)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_executable(chip8_emulator)

target_include_directories(chip8_emulator PRIVATE inc)
target_link_libraries(chip8_emulator PRIVATE SDL3)

target_sources(chip8_emulator
  PRIVATE
    src/main.c
    src/chip8.c
)

target_compile_options(chip8_emulator
  PRIVATE
    -Wall
    -Wextra
    -Wpedantic
)
