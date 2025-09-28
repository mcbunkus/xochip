cmake_minimum_required(VERSION 3.5)
project(xochip C)

set(CMAKE_C_STANDARD 99)

option(BUILD_DESKTOP_EMULATOR "Build the runnable desktop version, uses SDL3" OFF)

if (BUILD_DESKTOP_EMULATOR)
    include(FetchContent)
    FetchContent_Declare(
            sdl3
            GIT_REPOSITORY https://github.com/libsdl-org/SDL.git
            GIT_TAG release-3.2.22
    )

    FetchContent_MakeAvailable(sdl3)

    add_executable(xochip-emulator emulator.c xochip.h)
    target_link_libraries(xochip-emulator PRIVATE SDL3::SDL3)
    if (WIN32)
        add_custom_command(
                TARGET xochip-emulator POST_BUILD
                COMMAND "${CMAKE_COMMAND}" -E copy $<TARGET_FILE:SDL3::SDL3-shared> $<TARGET_FILE_DIR:xochip-emulator>
                VERBATIM
        )
    endif ()
endif ()