// List of all op codes handled by this emulator.
// I admit, I had claude generate this. In my defense, I didn't want to type this all out.
//
// CHIP-8 Original Instructions
// SYS          // 0nnn - System call (usually ignored)
// CLS          // 00E0 - Clear screen
// RET          // 00EE - Return from subroutine
// JP_ADDR      // 1nnn - Jump to address nnn
// CALL         // 2nnn - Call subroutine at nnn
// SE_VX_BYTE   // 3xkk - Skip next instruction if Vx == kk
// SNE_VX_BYTE  // 4xkk - Skip next instruction if Vx != kk
// SE_VX_VY     // 5xy0 - Skip next instruction if Vx == Vy
// LD_VX_BYTE   // 6xkk - Set Vx = kk
// ADD_VX_BYTE  // 7xkk - Set Vx = Vx + kk
// LD_VX_VY     // 8xy0 - Set Vx = Vy
// OR_VX_VY     // 8xy1 - Set Vx = Vx OR Vy
// AND_VX_VY    // 8xy2 - Set Vx = Vx AND Vy
// XOR_VX_VY    // 8xy3 - Set Vx = Vx XOR Vy
// ADD_VX_VY    // 8xy4 - Set Vx = Vx + Vy, set VF = carry
// SUB_VX_VY    // 8xy5 - Set Vx = Vx - Vy, set VF = NOT borrow
// SHR_VX_VY    // 8xy6 - Set Vx = Vx SHR 1
// SUBN_VX_VY   // 8xy7 - Set Vx = Vy - Vx, set VF = NOT borrow
// SHL_VX_VY    // 8xyE - Set Vx = Vx SHL 1
// SNE_VX_VY    // 9xy0 - Skip next instruction if Vx != Vy
// LD_I_ADDR    // Annn - Set I = nnn
// JP_V0_ADDR   // Bnnn - Jump to location nnn + V0
// RND_VX_BYTE  // Cxkk - Set Vx = random byte AND kk
// DRW_VX_VY_N  // Dxyn - Draw n-byte sprite at (Vx, Vy), set VF = collision
// SKP_VX       // Ex9E - Skip next instruction if key with value Vx is pressed
// SKNP_VX      // ExA1 - Skip next instruction if key with value Vx is not pressed
// LD_VX_DT     // Fx07 - Set Vx = delay timer value
// LD_VX_K      // Fx0A - Wait for key press, store value in Vx
// LD_DT_VX     // Fx15 - Set delay timer = Vx
// LD_ST_VX     // Fx18 - Set sound timer = Vx
// ADD_I_VX     // Fx1E - Set I = I + Vx
// LD_F_VX      // Fx29 - Set I = location of sprite for digit Vx
// LD_B_VX      // Fx33 - Store BCD representation of Vx in memory locations I, I+1, and I+2
// LD_I_VX      // Fx55 - Store registers V0 through Vx in memory starting at location I
// LD_VX_I      // Fx65 - Read registers V0 through Vx from memory starting at location I

// SUPER-CHIP Extensions
// SCD_N        // 00Cn - Scroll display down n pixels
// SCR          // 00FB - Scroll display right 4 pixels
// SCL          // 00FC - Scroll display left 4 pixels
// EXIT         // 00FD - Exit interpreter
// LOW          // 00FE - Enter low resolution (64x32) mode
// HIGH         // 00FF - Enter high resolution (128x64) mode
// DRW_VX_VY_0  // Dxy0 - Draw 16x16 sprite at (Vx, Vy), set VF = collision
// LD_HF_VX     // Fx30 - Set I = location of 10-byte font character in Vx
// LD_R_VX      // Fx75 - Store V0..VX in RPL user flags (X <= 7)
// LD_VX_R      // Fx85 - Read V0..VX from RPL user flags (X <= 7)

// XO-CHIP Extensions
// SAVE_VX_VY   // 5xy2 - Save an inclusive range of registers to memory starting at I
// LOAD_VX_VY   // 5xy3 - Load an inclusive range of registers from memory starting at I
// LD_I_LONG    // F000 nnnn - Set I = nnnn (16-bit immediate)
// PLANE        // Fn01 - Select drawing planes n (n = 1, 2, or 3)
// AUDIO        // F002 - Store 16 bytes starting at I in the audio pattern buffer
// LD_PITCH_VX  // Fx3A - Set audio pitch = Vx

// Additional XO-CHIP opcodes for completeness
// XOCHIP_OP_INVALID // For invalid/unknown opcodes

#ifndef XOCHIP_H
#define XOCHIP_H

#include <stdbool.h>
#include <stdint.h>
#include <string.h>

// =====================================================================================================================
//    DEFINES
// =====================================================================================================================

// XO-CHIP address space is 64kb vs the original CHIP-8's 4kb. However, like the original CHIP-8, the first 512 bytes is
// traditionally reserved for the interpreter. We don't need that, and since this is meant for resource constrained
// systems, we don't want to waste those bytes. 0xFE00 is 0x10000 - 0x200, or 64kb without the 512 wasted bytes.
#define XOCHIP_ADDRESS_SPACE_SIZE 0xFE00

// Where the address space would traditionally start. This value is applied to address operations as needed, to account
// for the "missing" 512 bytes.
#define XOCHIP_ADDRESS_SPACE_START 0x200

// The display's width and height. There's a xochip_display_t struct that the emulator uses that assumes this is always
// the case.
#define XOCHIP_DISPLAY_WIDTH 128
#define XOCHIP_DISPLAY_HEIGHT 64
#define XOCHIP_DISPLAY_PIXELS (XOCHIP_DISPLAY_WIDTH * XOCHIP_DISPLAY_HEIGHT)

// Extract individual nibbles (4-bit values)
// For opcode format: [N1][N2][N3][N4] where each N is a nibble
#define OPCODE_N1(opcode) (((opcode) & 0xF000) >> 12) // First nibble (bits 15-12)
#define OPCODE_N2(opcode) (((opcode) & 0x0F00) >> 8)  // Second nibble (bits 11-8)
#define OPCODE_N3(opcode) (((opcode) & 0x00F0) >> 4)  // Third nibble (bits 7-4)
#define OPCODE_N4(opcode) ((opcode) & 0x000F)         // Fourth nibble (bits 3-0)

// Extract bytes
#define OPCODE_HIGH_BYTE(opcode) (((opcode) & 0xFF00) >> 8) // High byte (bits 15-8)
#define OPCODE_LOW_BYTE(opcode) ((opcode) & 0x00FF)         // Low byte (bits 7-0)

// Common opcode patterns for CHIP-8/XO-CHIP
#define OPCODE_X(opcode) OPCODE_N2(opcode)        // Register X (second nibble)
#define OPCODE_Y(opcode) OPCODE_N3(opcode)        // Register Y (third nibble)
#define OPCODE_N(opcode) OPCODE_N4(opcode)        // Single nibble value
#define OPCODE_KK(opcode) OPCODE_LOW_BYTE(opcode) // Byte immediate value
#define OPCODE_NNN(opcode) ((opcode) & 0x0FFF)    // 12-bit address/value

// =====================================================================================================================
//    TYPES
// =====================================================================================================================

typedef enum xochip_result
{
    XOCHIP_SUCCESS,
    XOCHIP_ERR_ROM_TOO_LARGE,
    XOCHIP_ERR_INVALID_INSTRUCTION,
    XOCHIP_ERR_ADDRESS_OVERFLOW,
    XOCHIP_ERR_ADDRESS_UNDERFLOW,
    XOCHIP_ERR_STACK_OVERFLOW,
    XOCHIP_ERR_NULL_POINTER,
} xochip_result_t;

/**
 * V1-VF registers, these are used for indexing into the registers array in the xochip_t struct. You don't need to use
 * these directly.
 */
typedef enum xochip_registers
{
    XOCHIP_V0,
    XOCHIP_V1,
    XOCHIP_V2,
    XOCHIP_V3,
    XOCHIP_V4,
    XOCHIP_V5,
    XOCHIP_V6,
    XOCHIP_V7,
    XOCHIP_V8,
    XOCHIP_V9,
    XOCHIP_VA,
    XOCHIP_VB,
    XOCHIP_VC,
    XOCHIP_VD,
    XOCHIP_VE,
    XOCHIP_VF,
    XOCHIP_VDELAY,
    XOCHIP_VCOUNT
} xochip_registers_t;

/**
 * Used for representing XO-CHIP's hexadecimal input keys.
 */
typedef enum xochip_keys
{
    XOCHIP_KEY0,
    XOCHIP_KEY1,
    XOCHIP_KEY2,
    XOCHIP_KEY3,
    XOCHIP_KEY4,
    XOCHIP_KEY5,
    XOCHIP_KEY6,
    XOCHIP_KEY7,
    XOCHIP_KEY8,
    XOCHIP_KEY9,
    XOCHIP_KEYA,
    XOCHIP_KEYB,
    XOCHIP_KEYC,
    XOCHIP_KEYD,
    XOCHIP_KEYE,
    XOCHIP_KEYF,
} xochip_keys_t;

/**
 * The XO-CHIP's address stack
 */
typedef struct xochip_stack
{
    uint16_t addresses[16];
    uint8_t counter;
} xochip_stack_t;

/**
 * 128x64 pixel display buffer. Each pixel is packed into 1024 uint8_t's, each bit corresponding to 1 pixel.
 */
typedef struct xochip_display
{
    uint8_t pixels[XOCHIP_DISPLAY_PIXELS / 8]; // 8192 bits representing pixels
    bool updated;
} xochip_display_t;

/**
 * This is the main struct, which holds all the ROM, registers, counters, pressed keys, etc. All fields in here are
 * "private", just don't mess around in here unless you have a good reason to. The API below provides access and
 * instructions.
 */
typedef struct xochip
{
    uint16_t counter; // program counter
    uint16_t address; // address index (VI)
    uint16_t keys;    // pressed keys packed into an uint16_t for space

    uint8_t registers[XOCHIP_VCOUNT];
    uint8_t memory[XOCHIP_ADDRESS_SPACE_SIZE];
    xochip_stack_t stack;

    xochip_display_t display; // the pixel display buffer
} xochip_t;

// =====================================================================================================================
//    API
// =====================================================================================================================

/**
 * @brief Initializes an emulator.
 * @param emulator A non-null pointer to an emulator
 * @return Success or error:
 * - XOCHIP_SUCCESS when ok
 * - XOCHIP_ERR_NULL_POINTER when emulator is null
 */
xochip_result_t xochip_init(xochip_t *emulator);

/**
 * @brief Resets the internal state of an emulator, like you just booted it up for the first time. ROM will be cleared.
 *
 * @param emulator A non-null pointer to an emulator
 * @return Success or error:
 * - XOCHIP_SUCCESS when ok
 * - XOCHIP_ERR_NULL_POINTER when emulator is null
 */
xochip_result_t xochip_reset(xochip_t *emulator);

/**
 * @brief Load the full contents of a ROM into the emulator's address space. The emulator will completely clear the
 * memory and load in the new ROM. This is more convenient if you're able to allocate enough memory for a complete ROM.
 * @param emulator A non-null pointer to an emulator
 * @param data Beginning of the ROM data buffer
 * @param size The number of bytes to copy into the emulator's address space
 * @return Success or error:
 * - XOCHIP_SUCCESS when ok
 * - XOCHIP_ERR_NULL_POINTER when emulator is null
 * - XOCHIP_ERR_ROM_TOO_LARGE when ROM is too large to fit in the address space
 */
xochip_result_t xochip_load_rom(xochip_t *emulator, const uint8_t *data, uint16_t size);

/**
 * @brief Write a chunk of memory into the address space. Useful if you can't copy the full ROM in one go for whatever
 * reason.
 * @param emulator A non-null pointer to an emulator
 * @param data Beginning of the ROM data buffer
 * @param size The number of bytes to copy into the emulator's address space
 * @param address Where in the emulator's address space you want to write this memory to (start from 0, not from 0x200!)
 * @return Success or error:
 * - XOCHIP_SUCCESS when ok
 * - XOCHIP_ERR_NULL_POINTER when emulator is null
 * - XOCHIP_ERR_ROM_TOO_LARGE when ROM is too large to fit in the address space
 * - XOCHIP_ERR_OVERFLOW when write would overflow the address space
 */
xochip_result_t xochip_write_rom(xochip_t *emulator, const uint8_t *data, uint16_t size, uint16_t address);

/**
 * @brief Perform the next operation. This doesn't handle timing or anything like that. That is left to you because it
 * depends on your circumstances.
 * @param emulator A non-null pointer to an emulator
 * @return Success or error:
 * - XOCHIP_SUCCESS when ok
 * - XOCHIP_ERR_NULL_POINTER when emulator is null
 * - XOCHIP_ERR_INVALID_INSTRUCTION when an invalid instruction is executed
 */
xochip_result_t xochip_cycle(xochip_t *emulator);

// =====================================================================================================================
//    IMPLEMENTATION
// =====================================================================================================================

#ifdef XOCHIP_IMPLEMENTATION

// =====================================================================================================================
//    HELPERS
// =====================================================================================================================

static xochip_result_t xochip_stack_push(xochip_stack_t *stack, uint16_t address)
{
    if (!stack)
    {
        return XOCHIP_ERR_NULL_POINTER;
    }

    if (stack->counter >= (sizeof(stack->addresses) - 1))
    {
        return XOCHIP_ERR_STACK_OVERFLOW; // I'm finally a real programmer now
    }

    stack->addresses[stack->counter] = address;
    stack->counter++;

    return XOCHIP_SUCCESS;
}

static xochip_result_t xochip_stack_pop(xochip_stack_t *stack, uint16_t *address)
{
    if (!stack || !address)
    {
        return XOCHIP_ERR_NULL_POINTER;
    }

    *address = stack->addresses[stack->counter];

    if (stack->counter > 0)
    {
        stack->addresses[stack->counter] = 0;
        stack->counter--;
    }

    return XOCHIP_SUCCESS;
}

// =====================================================================================================================
//    OP CODE HANDLERS
// =====================================================================================================================

// clear the screen
static xochip_result_t xochip_op_cls(xochip_t *emulator)
{
    memset(emulator->display.pixels, 0, sizeof(emulator->display.pixels));
    emulator->display.updated = true;
    return XOCHIP_SUCCESS;
}

// return from a subroutine
static xochip_result_t xochip_op_ret(xochip_t *emulator)
{
    return xochip_stack_pop(&emulator->stack, &emulator->counter);
}

// jump to an address
static xochip_result_t xochip_op_jp_addr(xochip_t *emulator, uint16_t address)
{
    if (address < XOCHIP_ADDRESS_SPACE_START)
    {
        return XOCHIP_ERR_ADDRESS_UNDERFLOW;
    }

    address -= XOCHIP_ADDRESS_SPACE_START;
    if (address > XOCHIP_ADDRESS_SPACE_SIZE)
    {
        return XOCHIP_ERR_ADDRESS_OVERFLOW;
    }

    emulator->counter = address;
    return XOCHIP_SUCCESS;
}

// call a subroutine at address
static xochip_result_t xochip_op_call(xochip_t *emulator, uint16_t address)
{
    if (address < XOCHIP_ADDRESS_SPACE_START)
    {
        return XOCHIP_ERR_ADDRESS_UNDERFLOW;
    }

    address -= XOCHIP_ADDRESS_SPACE_START;
    if (address > XOCHIP_ADDRESS_SPACE_SIZE)
    {
        return XOCHIP_ERR_ADDRESS_OVERFLOW;
    }

    const xochip_result_t res = xochip_stack_push(&emulator->stack, emulator->counter);
    if (!res)
    {
        return res;
    }

    emulator->counter = address;
    return XOCHIP_SUCCESS;
}

// =====================================================================================================================
//    API IMPLEMENTATIONS
// =====================================================================================================================
xochip_result_t xochip_init(xochip_t *emulator) { return xochip_reset(emulator); }

xochip_result_t xochip_reset(xochip_t *emulator)
{
    if (!emulator)
    {
        return XOCHIP_ERR_NULL_POINTER;
    }

    emulator->counter = 0;
    emulator->address = 0;
    emulator->keys = 0;
    emulator->stack.counter = 0;

    memset(emulator->memory, 0, sizeof(emulator->memory));
    memset(emulator->registers, 0, sizeof(emulator->registers));
    memset(emulator->stack.addresses, 0, sizeof(emulator->stack.addresses));
    memset(emulator->display.pixels, 0, sizeof(emulator->display.pixels));

    return XOCHIP_SUCCESS;
}

xochip_result_t xochip_load_rom(xochip_t *emulator, const uint8_t *data, uint16_t size)
{
    if (!emulator)
    {
        return XOCHIP_ERR_NULL_POINTER;
    }

    if (size > XOCHIP_ADDRESS_SPACE_SIZE)
    {
        return XOCHIP_ERR_ROM_TOO_LARGE;
    }

    memset(emulator->memory, 0, sizeof(emulator->memory));
    memcpy(emulator->memory, data, size);
    return XOCHIP_SUCCESS;
}

xochip_result_t xochip_write_rom(xochip_t *emulator, const uint8_t *data, uint16_t size, uint16_t address)
{
    if (!emulator)
    {
        return XOCHIP_ERR_NULL_POINTER;
    }

    if (size > XOCHIP_ADDRESS_SPACE_SIZE)
    {
        return XOCHIP_ERR_ROM_TOO_LARGE;
    }

    if ((address + size) > XOCHIP_ADDRESS_SPACE_SIZE)
    {
        return XOCHIP_ERR_ADDRESS_OVERFLOW;
    }

    memcpy(emulator->memory + address, data, size);
    return XOCHIP_SUCCESS;
}

xochip_result_t xochip_cycle(xochip_t *emulator)
{
    if (!emulator)
    {
        return XOCHIP_ERR_NULL_POINTER;
    }

    if ((emulator->counter + 1) >= XOCHIP_ADDRESS_SPACE_SIZE)
    {
        return XOCHIP_ERR_ADDRESS_OVERFLOW;
    }

    const uint16_t next_instruction =
        (emulator->memory[emulator->counter] << 8) | (emulator->memory[emulator->counter + 1]);

    emulator->counter += 2;

    switch (OPCODE_N1(next_instruction))
    {
    case 0x0:
    {
        if (next_instruction == 0x00E0)
        {
            return xochip_op_cls(emulator);
        }

        if (next_instruction == 0x00EE)
        {
            return xochip_op_ret(emulator);
        }

        // else this is a SYS command which we don't handle
        return XOCHIP_SUCCESS;
    }
    case 0x1:
    {
        const uint16_t address = OPCODE_NNN(next_instruction);
        return xochip_op_jp_addr(emulator, address);
    }
    case 0x2:
    {
        const uint16_t address = OPCODE_NNN(next_instruction);
        return xochip_op_call(emulator, address);
    }
    default:
        return XOCHIP_ERR_INVALID_INSTRUCTION;
    }
}

#endif

#endif // XOCHIP_H
