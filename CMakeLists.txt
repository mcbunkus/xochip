cmake_minimum_required(VERSION 4.0)
project(xochip C)

set(CMAKE_C_STANDARD 99)

add_executable(xochip_load_rom test/load_rom.c xochip.h)
