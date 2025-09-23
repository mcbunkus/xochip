#define XOCHIP_IMPLEMENTATION
#include "../xochip.h"

#include <stdio.h>

int main(int argc, char *argv[])
{

    if (argc != 2)
    {
        fprintf(stderr, "Usage: %s <rom file>\n", argv[0]);
        return 1;
    }

    xochip_t emulator;
    xochip_init(&emulator);

    uint8_t rom[XOCHIP_ADDRESS_SPACE_SIZE];

    FILE *rom_file = fopen(argv[1], "rb");
    if (!rom_file)
    {
        perror("Failed to open rom file");
        return 1;
    }

    const size_t nread = fread(rom, sizeof(rom[0]), XOCHIP_ADDRESS_SPACE_SIZE, rom_file);
    if (nread != XOCHIP_ADDRESS_SPACE_SIZE && !feof(rom_file))
    {
        perror("Failed to read rom file");
        return 2;
    }

    fclose(rom_file);
    xochip_result_t result = xochip_load_rom(&emulator, rom, nread);

    if (result != XOCHIP_SUCCESS)
    {
        perror("Failed to load rom into emulator");
    }

    result = xochip_cycle(&emulator);

    if (result != XOCHIP_SUCCESS)
    {
        perror("Failed to cycle emulator");
    }

    return 0;
}