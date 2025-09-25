cmake_minimum_required(VERSION 4.0)
project(xochip C)

set(CMAKE_C_STANDARD 99)

include(FetchContent)

FetchContent_Declare(
        unity
        GIT_REPOSITORY https://github.com/ThrowTheSwitch/Unity.git
        GIT_TAG v2.6.1
)

FetchContent_MakeAvailable(unity)


function(unit_test TEST_NAME)
    cmake_parse_arguments(
            TEST                          # Prefix
            ""                           # Options (boolean flags)
            ""                           # One-value keywords
            "SOURCES;DEPENDENCIES"       # Multi-value keywords
            ${ARGN}                      # Arguments to parse
    )
    add_executable(xochip_${TEST_NAME} ${TEST_SOURCES} xochip.h)
    target_link_libraries(xochip_${TEST_NAME} PRIVATE unity)
    add_test(NAME ${TEST_NAME} COMMAND ${TEST_NAME})
endfunction()

unit_test(load_rom SOURCES test/load_rom.c)
unit_test(opcodes SOURCES test/opcodes.c)


