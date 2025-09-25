//
// Created by Austen on 9/23/2025.
//

#define XOCHIP_IMPLEMENTATION
#include "../xochip.h"

#include "rom.h"
#include "unity.h"

xochip_t emulator;

void setUp(void)
{
    xochip_result_t result = xochip_init(&emulator);
    TEST_ASSERT_EQUAL_MESSAGE(XOCHIP_SUCCESS, result, "xochip_init failed");

    result = xochip_load_rom(&emulator, xochip_test_rom, xochip_test_rom_len);
    TEST_ASSERT_EQUAL_MESSAGE(XOCHIP_SUCCESS, result, "xochip_load_rom failed");
}

void tearDown(void)
{
    const xochip_result_t result = xochip_reset(&emulator);
    TEST_ASSERT_EQUAL_MESSAGE(XOCHIP_SUCCESS, result, "xochip_reset failed");
}

void test_op_cls(void)
{
    emulator.display.back_plane[3] = 0x23; // something random
    emulator.display.back_plane[9] = 0xaf; // something random
    emulator.display.updated = false;      // this should be true after
    emulator.counter = 0;

    xochip_cycle(&emulator);

    TEST_ASSERT_EACH_EQUAL_UINT8(0x0, emulator.display.fore_plane, sizeof(emulator.display.fore_plane));
    TEST_ASSERT_EACH_EQUAL_UINT8(0x0, emulator.display.back_plane, sizeof(emulator.display.back_plane));
    TEST_ASSERT_TRUE(emulator.display.updated);

    TEST_ASSERT_EQUAL(emulator.counter, 2);
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_op_cls);
    return UNITY_END();
}