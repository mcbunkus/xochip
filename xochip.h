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

// TODO figure out what to do when we don't have stdlib.h. The only reason they're here is for random
// numbers. Replace this with a better solution
#include <stdlib.h>

// =====================================================================================================================
//    DEFINES
// =====================================================================================================================

// XO-CHIP address space is 64kb vs the original CHIP-8's 4kb. However, like the original CHIP-8, the first 512 bytes is
// traditionally reserved for the interpreter. We don't need that, and since this is meant for resource constrained
// systems, we don't want to waste those bytes. 0xFE00 is 0x10000 - 0x200, or 64kb without the 512 wasted bytes.
#define XOCHIP_ADDRESS_SPACE_SIZE 0x10000

// Where the address space would traditionally start. This value is applied to address operations as needed, to account
// for the "missing" 512 bytes.
#define XOCHIP_ADDRESS_SPACE_START 0x200

// The display's width and height. There's a xochip_display_t struct that the emulator uses that assumes this is always
// the case.
#define XOCHIP_DISPLAY_WIDTH 128
#define XOCHIP_DISPLAY_HEIGHT 64
#define XOCHIP_DISPLAY_PIXELS (XOCHIP_DISPLAY_WIDTH * XOCHIP_DISPLAY_HEIGHT)

// XO-CHIP fonts are 5 rows tall, therefore 5 bytes
#define XOCHIP_FONT_SIZE 5

// XO-CHIP instruction size
#define XOCHIP_OPCODE_SIZE 2

// Creates the mask needed for doing bitwise operations on the pressed/released key integers acting as bit fields for
// the keys
#define XOCHIP_KEY(key) (((uint16_t)0x01) << (key))

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

// XO-CHIPs are 8-bit machines == 8-bit registers
typedef uint8_t xochip_register_t;

// XO-CHIP address space takes full advantage of 16-bit address space
typedef uint16_t xochip_address_t;

typedef enum xochip_result
{
    XOCHIP_SUCCESS,                 // saul goodman
    XOCHIP_ERR_ROM_TOO_LARGE,       // too much game
    XOCHIP_ERR_INVALID_INSTRUCTION, // not an instruction
    XOCHIP_ERR_ADDRESS_OVERFLOW,    // attempted to access memory outside address space
    XOCHIP_ERR_ADDRESS_UNDERFLOW,   // attempted to access memory outside address space
    XOCHIP_ERR_STACK_OVERFLOW,      // tried to push to many subroutines onto the stack
    XOCHIP_ERR_NULL_POINTER,        // whatever pointer you passed to something was null
} xochip_result_t;

/**
 * V1-VF registers, these are used for indexing into the registers array in the xochip_t struct. You don't need to use
 * these directly.
 */
typedef enum xochip_registers
{
    XOCHIP_V0, // 0-E are general purpose registers
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
    XOCHIP_VF,     // used for flags, context dependent upon previous instruction:wa
    XOCHIP_VSOUND, // sound timer (>0 means play sound, usually ticked down at 60 Hz)
    XOCHIP_VPITCH, // pitch of audio playback (4000 * 2 ^ ((vx - 64) / 48))
    XOCHIP_VDELAY, // delay timer (usually ticked down at 60 Hz)
    XOCHIP_VCOUNT  // just a sentinel value, not a register
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
    XOCHIP_KEYCOUNT,
} xochip_keys_t;

/**
 * The XO-CHIP's address stack. I think the original spec called for 16 addresses?
 */
typedef struct xochip_stack
{
    uint16_t addresses[16];
    xochip_register_t counter;
} xochip_stack_t;

/**
 * 128x64 pixel display buffer. Each pixel is packed into 1024 uint8_t's, each bit corresponding to 1 pixel.
 */
typedef struct xochip_display
{
    uint8_t back_plane[XOCHIP_DISPLAY_PIXELS / 8]; // 8192 bits representing pixels
    uint8_t fore_plane[XOCHIP_DISPLAY_PIXELS / 8]; // 8192 bits representing pixels
    uint8_t selected_plane;
    bool updated;
} xochip_display_t;

/**
 * This is the main struct, which holds all the ROM, registers, counters, pressed keys, etc. All fields in here are
 * "private", just don't mess around in here unless you have a good reason to. The API below provides access and
 * instructions.
 */
typedef struct xochip
{
    uint16_t counter;       // program counter
    uint16_t address;       // address index (VI)
    uint16_t pressed_keys;  // pressed keys packed into an uint16_t for space
    uint16_t released_keys; // pressed keys packed into an uint16_t for space

    xochip_register_t registers[XOCHIP_VCOUNT];
    uint8_t memory[XOCHIP_ADDRESS_SPACE_SIZE];
    xochip_stack_t stack;

    xochip_display_t display; // the pixel display buffer
    uint8_t audio[16];        // audio buffer, 16 bytes per spec
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

/**
 * @brief Tick the emulators various timers down. It's recommended you call this function at 60 Hz, since that is what
 * the original CHIP-8s did. This function will always succeed.
 * @param emulator A non-null pointer to an emulator
 */
void xochip_tick(xochip_t *emulator);

/**
 * @brief Instruct the emulator that a key has been released
 * @param emulator A non-null pointer to an emulator
 * @param key The key that was just released. If the key is not in the range defined by xochip_keys_t, it is ignored
 */
void xochip_key_up(xochip_t *emulator, xochip_keys_t key);

/**
 * @brief Instruct the emulator that a key has been pressed
 * @param emulator A non-null pointer to an emulator
 * @param key The key that was just pressed. If the key is not in the range defined by xochip_keys_t, it is ignored
 */
void xochip_key_down(xochip_t *emulator, xochip_keys_t key);

/**
 *
 * @param err A result returned from a function, assuming you're calling this after an error
 * @return A string description of the error
 */
const char *xochip_strerror(xochip_result_t err);

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

// Used for checking released keys in emulator->released_keys
static int8_t xochip_find_first_set_bit(uint16_t value)
{
    int8_t pos = 0;

    // Check lower byte first (usually faster on 8-bit MCUs)
    if (!(value & 0xFF))
    {
        value >>= 8;
        pos += 8;
    }

    // Binary search through remaining bits
    if (!(value & 0x0F))
    {
        value >>= 4;
        pos += 4;
    }
    if (!(value & 0x03))
    {
        value >>= 2;
        pos += 2;
    }
    if (!(value & 0x01))
    {
        pos += 1;
    }

    return pos;
}

// =====================================================================================================================
//    OP CODE HANDLERS
// =====================================================================================================================

// clear the screen
inline xochip_result_t xochip_op_cls(xochip_t *emulator)
{
    memset(emulator->display.back_plane, 0, sizeof(emulator->display.back_plane));
    memset(emulator->display.fore_plane, 0, sizeof(emulator->display.fore_plane));
    emulator->display.updated = true;
    return XOCHIP_SUCCESS;
}

// return from a subroutine
inline xochip_result_t xochip_op_ret(xochip_t *emulator)
{
    return xochip_stack_pop(&emulator->stack, &emulator->counter);
}

// jump to an address
inline xochip_result_t xochip_op_jp_addr(xochip_t *emulator, uint16_t address)
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

static xochip_result_t xochip_op_se_vx_b(xochip_t *emulator, const xochip_register_t reg, const uint8_t byte)
{
    if (emulator->registers[reg] == byte)
    {
        emulator->counter += XOCHIP_OPCODE_SIZE;
    }
    return XOCHIP_SUCCESS;
}

static xochip_result_t xochip_op_sne_vx_b(xochip_t *emulator, const xochip_register_t reg, const uint8_t byte)
{
    if (emulator->registers[reg] != byte)
    {
        emulator->counter += XOCHIP_OPCODE_SIZE;
    }
    return XOCHIP_SUCCESS;
}

static xochip_result_t xochip_op_se_vx_vy(xochip_t *emulator, const xochip_register_t vx, const xochip_register_t vy)
{
    if (emulator->registers[vx] == emulator->registers[vy])
    {
        emulator->counter += XOCHIP_OPCODE_SIZE;
    }
    return XOCHIP_SUCCESS;
}

static xochip_result_t xochip_op_ld_vx_b(xochip_t *emulator, const xochip_register_t vx, const uint8_t byte)
{
    emulator->registers[vx] = byte;
    return XOCHIP_SUCCESS;
}

static xochip_result_t xochip_op_add_vx_b(xochip_t *emulator, const xochip_register_t vx, const uint8_t byte)
{
    emulator->registers[vx] += byte;
    return XOCHIP_SUCCESS;
}

static xochip_result_t xochip_op_ld_vx_vy(xochip_t *emulator, const xochip_register_t vx, const xochip_register_t vy)
{
    emulator->registers[vx] = emulator->registers[vy];
    return XOCHIP_SUCCESS;
}

static xochip_result_t xochip_op_or_xv_vy(xochip_t *emulator, const xochip_register_t vx, const xochip_register_t vy)
{
    emulator->registers[vx] |= emulator->registers[vy];
    emulator->registers[XOCHIP_VF] = 0;
    return XOCHIP_SUCCESS;
}

static xochip_result_t xochip_op_and_xv_vy(xochip_t *emulator, const xochip_register_t vx, const xochip_register_t vy)
{
    emulator->registers[vx] &= emulator->registers[vy];
    emulator->registers[XOCHIP_VF] = 0;
    return XOCHIP_SUCCESS;
}

static xochip_result_t xochip_op_xor_xv_vy(xochip_t *emulator, const xochip_register_t vx, const xochip_register_t vy)
{
    emulator->registers[vx] ^= emulator->registers[vy];
    emulator->registers[XOCHIP_VF] = 0;
    return XOCHIP_SUCCESS;
}

static xochip_result_t xochip_op_add_xv_vy(xochip_t *emulator, const xochip_register_t vx, const xochip_register_t vy)
{
    emulator->registers[vx] += emulator->registers[vy];
    emulator->registers[XOCHIP_VF] = emulator->registers[vx] <= emulator->registers[vy] ? 1 : 0;
    return XOCHIP_SUCCESS;
}

static xochip_result_t xochip_op_sub_xv_vy(xochip_t *emulator, const xochip_register_t vx, const xochip_register_t vy)
{
    // This one took more finesse to make the test ROMs happy
    const uint8_t _vx = emulator->registers[vx];
    const uint8_t _vy = emulator->registers[vy];
    emulator->registers[vx] = _vx - _vy;
    emulator->registers[XOCHIP_VF] = _vx >= _vy ? 1 : 0;
    return XOCHIP_SUCCESS;
}

// There's some confusion on this one, SHR 1 or SHR VY?
static xochip_result_t xochip_op_shr_xv_vy(xochip_t *emulator, const xochip_register_t vx, const xochip_register_t vy)
{
    const uint8_t carry = emulator->registers[vx] & 0x1;
    emulator->registers[vx] >>= emulator->registers[vy];
    emulator->registers[XOCHIP_VF] = carry;
    return XOCHIP_SUCCESS;
}

static xochip_result_t xochip_op_subn_xv_vy(xochip_t *emulator, const xochip_register_t vx, const xochip_register_t vy)
{
    emulator->registers[vx] = emulator->registers[vy] - emulator->registers[vx];
    emulator->registers[XOCHIP_VF] = emulator->registers[vy] >= emulator->registers[vx] ? 1 : 0;
    return XOCHIP_SUCCESS;
}

static xochip_result_t xochip_op_shl_xv_vy(xochip_t *emulator, const xochip_register_t vx, const xochip_register_t vy)
{
    const uint8_t carry = (emulator->registers[vx] & 0x80) >> 7;
    emulator->registers[vx] <<= emulator->registers[vy];
    emulator->registers[XOCHIP_VF] = carry;
    return XOCHIP_SUCCESS;
}

static xochip_result_t xochip_op_sne_xv_vy(xochip_t *emulator, const xochip_register_t vx, const xochip_register_t vy)
{
    if (emulator->registers[vx] != emulator->registers[vy])
    {
        emulator->counter += XOCHIP_OPCODE_SIZE;
    }
    return XOCHIP_SUCCESS;
}

static xochip_result_t xochip_op_ld_i(xochip_t *emulator, const xochip_address_t address)
{
    emulator->address = address;
    return XOCHIP_SUCCESS;
}

static xochip_result_t xochip_op_jp_v0_addr(xochip_t *emulator, const xochip_address_t address)
{
    emulator->address = emulator->registers[XOCHIP_V0] + address;
    return XOCHIP_SUCCESS;
}

static xochip_result_t xochip_op_rnd_vx_b(xochip_t *emulator, const xochip_register_t vx, const uint8_t byte)
{
    const uint8_t rnd = (uint8_t)(rand() % 0xFF);
    emulator->registers[vx] = rnd & byte;
    return XOCHIP_SUCCESS;
}

static xochip_result_t xochip_op_drw_vx_vy_n(xochip_t *emulator, const xochip_register_t vx, const xochip_register_t vy,
                                             const uint8_t height)
{

    return XOCHIP_SUCCESS;
}

static xochip_result_t xochip_op_skp_vx(xochip_t *emulator, const xochip_register_t vx)
{
    const uint16_t key = (uint16_t)0x1 << (emulator->registers[vx]);
    if (emulator->pressed_keys & key)
    {
        emulator->counter += XOCHIP_OPCODE_SIZE;
    }
    return XOCHIP_SUCCESS;
}

static xochip_result_t xochip_op_skpn_vx(xochip_t *emulator, const xochip_register_t vx)
{
    const uint16_t key = (uint16_t)0x1 << (emulator->registers[vx]);
    if (!(emulator->pressed_keys & key))
    {
        emulator->counter += XOCHIP_OPCODE_SIZE;
    }
    return XOCHIP_SUCCESS;
}

static xochip_result_t xochip_op_ld_vx_dt(xochip_t *emulator, const xochip_register_t vx)
{
    emulator->registers[vx] = emulator->registers[XOCHIP_VDELAY];
    return XOCHIP_SUCCESS;
}

static xochip_result_t xochip_op_ld_vx_k(xochip_t *emulator, const xochip_register_t vx)
{
    if (!(emulator->released_keys))
    {
        emulator->counter -= XOCHIP_OPCODE_SIZE;
        return XOCHIP_SUCCESS;
    }

    // we wouldn't get here if emulator->released_keys wasn't 0, therefore, there should always be a set bit
    emulator->registers[vx] = xochip_find_first_set_bit(emulator->released_keys);
    return XOCHIP_SUCCESS;
}

static xochip_result_t xochip_op_ld_dt_vx(xochip_t *emulator, const xochip_register_t vx)
{
    emulator->registers[XOCHIP_VDELAY] = emulator->registers[vx];
    return XOCHIP_SUCCESS;
}

static xochip_result_t xochip_op_ld_st_vx(xochip_t *emulator, const xochip_register_t vx)
{
    emulator->registers[XOCHIP_VSOUND] = emulator->registers[vx];
    return XOCHIP_SUCCESS;
}

static xochip_result_t xochip_op_add_i_vx(xochip_t *emulator, const xochip_register_t vx)
{
    emulator->address += emulator->registers[vx];
    return XOCHIP_SUCCESS;
}

static xochip_result_t xochip_op_ld_f_vx(xochip_t *emulator, const xochip_register_t vx)
{
    const uint8_t digit = emulator->registers[vx];
    emulator->address = digit * XOCHIP_FONT_SIZE;
    return XOCHIP_SUCCESS;
}

static xochip_result_t xochip_op_ld_b_vx(xochip_t *emulator, const xochip_register_t vx)
{
    const xochip_address_t VI = emulator->address;
    uint8_t value = emulator->registers[vx];
    emulator->memory[VI + 2] = value % 10;
    value /= 10;
    emulator->memory[VI + 1] = value % 10;
    value /= 10;
    emulator->memory[VI] = value % 10;
    return XOCHIP_SUCCESS;
}

static xochip_result_t xochip_op_ld_i_vx(xochip_t *emulator, const xochip_register_t vx)
{
    for (xochip_register_t reg = 0; reg <= emulator->registers[vx]; ++reg)
    {
        emulator->memory[emulator->address] = emulator->registers[reg];
        emulator->address++;
    }
    return XOCHIP_SUCCESS;
}

static xochip_result_t xochip_op_ld_vx_i(xochip_t *emulator, const xochip_register_t vx)
{
    for (xochip_register_t reg = 0; reg <= emulator->registers[vx]; ++reg)
    {
        emulator->registers[reg] = emulator->memory[emulator->address];
        emulator->address++;
    }
    return XOCHIP_SUCCESS;
}

// =====================================================================================================================
//    XO-CHIP EXTENSION OP-CODES
// =====================================================================================================================

static xochip_result_t xochip_op_save_vx_vy(xochip_t *emulator, const xochip_register_t vx, const xochip_register_t vy)
{
    const xochip_register_t start = min(vx, vy);
    const xochip_register_t end = max(vx, vy);

    for (xochip_register_t reg = start; reg <= end; ++reg)
    {
        emulator->memory[emulator->address] = emulator->registers[reg];
        emulator->address++;
    }
    return XOCHIP_SUCCESS;
}

static xochip_result_t xochip_op_load_vx_vy(xochip_t *emulator, const xochip_register_t vx, const xochip_register_t vy)
{
    const xochip_register_t start = min(vx, vy);
    const xochip_register_t end = max(vx, vy);

    for (xochip_register_t reg = start; reg <= end; ++reg)
    {
        emulator->registers[reg] = emulator->memory[emulator->address];
        emulator->address++;
    }
    return XOCHIP_SUCCESS;
}

static xochip_result_t xochip_op_ld_i_long(xochip_t *emulator, const xochip_address_t addr)
{
    emulator->address = addr;
    return XOCHIP_SUCCESS;
}

static xochip_result_t xochip_op_plane(xochip_t *emulator, const uint8_t plane)
{
    emulator->display.selected_plane = plane;
    return XOCHIP_SUCCESS;
}

static xochip_result_t xochip_op_audio(xochip_t *emulator)
{
    memcpy(emulator->audio, &emulator->memory[emulator->address], sizeof(emulator->audio));
    return XOCHIP_SUCCESS;
}

static xochip_result_t xochip_op_pitch(xochip_t *emulator, const xochip_register_t vx)
{
    emulator->registers[XOCHIP_VPITCH] = emulator->registers[vx];
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
    emulator->pressed_keys = 0;
    emulator->released_keys = 0;
    emulator->stack.counter = 0;

    memset(emulator->memory, 0, sizeof(emulator->memory));
    memset(emulator->registers, 0, sizeof(emulator->registers));
    memset(emulator->stack.addresses, 0, sizeof(emulator->stack.addresses));
    memset(emulator->display.back_plane, 0, sizeof(emulator->display.back_plane));
    memset(emulator->display.fore_plane, 0, sizeof(emulator->display.fore_plane));

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

// This trusts your emulator pointer is not null
xochip_result_t xochip_cycle(xochip_t *emulator)
{

    const uint16_t next_instruction =
        (emulator->memory[emulator->counter] << 8) | (emulator->memory[emulator->counter + 1]);

    emulator->counter += XOCHIP_OPCODE_SIZE;

    xochip_result_t result = XOCHIP_SUCCESS;

    switch (OPCODE_N1(next_instruction))
    {
    case 0x0:
    {
        switch (next_instruction)
        {
        case 0x00E0:
            result = xochip_op_cls(emulator);
            break;
        case 0x00EE:
            result = xochip_op_ret(emulator);
            break;
        default:
            // else this is a SYS command which we don't handle
            break;
        }
    }
    case 0x1:
    {
        const uint16_t address = OPCODE_NNN(next_instruction);
        result = xochip_op_jp_addr(emulator, address);
        break;
    }
    case 0x2:
    {
        const uint16_t address = OPCODE_NNN(next_instruction);
        result = xochip_op_call(emulator, address);
        break;
    }
    case 0x3:
    {
        const xochip_register_t vx = OPCODE_X(next_instruction);
        const uint8_t byte = OPCODE_KK(next_instruction);
        result = xochip_op_se_vx_b(emulator, vx, byte);
        break;
    }
    case 0x4:
    {
        const xochip_register_t vx = OPCODE_X(next_instruction);
        const uint8_t byte = OPCODE_KK(next_instruction);
        result = xochip_op_sne_vx_b(emulator, vx, byte);
        break;
    }
    case 0x5:
    {
        const xochip_register_t vx = OPCODE_X(next_instruction);
        const xochip_register_t vy = OPCODE_Y(next_instruction);

        switch (OPCODE_N(next_instruction))
        {
        case 0x0:
            result = xochip_op_se_vx_vy(emulator, vx, vy);
            break;
        case 0x2:
            result = xochip_op_save_vx_vy(emulator, vx, vy);
            break;
        case 0x3:
            result = xochip_op_load_vx_vy(emulator, vx, vy);
            break;
        default:
            result = XOCHIP_ERR_INVALID_INSTRUCTION;
            break;
        }
        break;
    }
    case 0x6:
    {
        const xochip_register_t vx = OPCODE_X(next_instruction);
        const uint8_t byte = OPCODE_KK(next_instruction);
        result = xochip_op_ld_vx_b(emulator, vx, byte);
        break;
    }
    case 0x7:
    {
        const xochip_register_t vx = OPCODE_X(next_instruction);
        const uint8_t byte = OPCODE_KK(next_instruction);
        result = xochip_op_add_vx_b(emulator, vx, byte);
        break;
    }
    case 0x8:
    {
        const xochip_register_t vx = OPCODE_X(next_instruction);
        const xochip_register_t vy = OPCODE_Y(next_instruction);

        switch (OPCODE_N(next_instruction))
        {
        case 0x0:
            result = xochip_op_ld_vx_vy(emulator, vx, vy);
            break;
        case 0x1:
            result = xochip_op_or_xv_vy(emulator, vx, vy);
            break;
        case 0x2:
            result = xochip_op_and_xv_vy(emulator, vx, vy);
            break;
        case 0x3:
            result = xochip_op_xor_xv_vy(emulator, vx, vy);
            break;
        case 0x4:
            result = xochip_op_add_xv_vy(emulator, vx, vy);
            break;
        case 0x5:
            result = xochip_op_sub_xv_vy(emulator, vx, vy);
            break;
        case 0x6:
            result = xochip_op_shr_xv_vy(emulator, vx, vy);
            break;
        case 0x7:
            result = xochip_op_subn_xv_vy(emulator, vx, vy);
            break;
        case 0xE:
            result = xochip_op_shl_xv_vy(emulator, vx, vy);
            break;
        default:
            result = XOCHIP_ERR_INVALID_INSTRUCTION;
            break;
        }
        break;
    }
    case 0x9:
    {
        const xochip_register_t vx = OPCODE_X(next_instruction);
        const xochip_register_t vy = OPCODE_Y(next_instruction);
        result = xochip_op_sne_xv_vy(emulator, vx, vy);
        break;
    }
    case 0xA:
    {
        result = xochip_op_ld_i(emulator, OPCODE_NNN(next_instruction));
        break;
    }
    case 0xB:
    {
        result = xochip_op_jp_v0_addr(emulator, OPCODE_NNN(next_instruction));
        break;
    }
    case 0xC:
    {
        const xochip_register_t vx = OPCODE_X(next_instruction);
        const uint8_t byte = OPCODE_KK(next_instruction);
        result = xochip_op_rnd_vx_b(emulator, vx, byte);
        break;
    }
    case 0xD:
    {
        const xochip_register_t vx = OPCODE_X(next_instruction);
        const xochip_register_t vy = OPCODE_X(next_instruction);
        const uint8_t height = OPCODE_N4(next_instruction);
        result = xochip_op_drw_vx_vy_n(emulator, vx, vy, height);
        break;
    }
    case 0xE:
    {
        const xochip_register_t vx = OPCODE_X(next_instruction);

        switch (OPCODE_KK(next_instruction))
        {
        case 0x9E:
            result = xochip_op_skp_vx(emulator, vx);
            break;
        case 0xA1:
            result = xochip_op_skpn_vx(emulator, vx);
            break;
        default:
            result = XOCHIP_ERR_INVALID_INSTRUCTION;
            break;
        }
        break;
    }
    case 0xF:
    {
        if (next_instruction == 0xF000)
        {
            emulator->counter += XOCHIP_OPCODE_SIZE;
            const uint16_t new_addr =
                (emulator->memory[emulator->counter] << 8) | (emulator->memory[emulator->counter + 1]);
            result = xochip_op_ld_i_long(emulator, new_addr);
            break;
        }

        const xochip_register_t vx = OPCODE_X(next_instruction);

        switch (OPCODE_KK(next_instruction))
        {
        case 0x01:
            result = xochip_op_plane(emulator, OPCODE_X(next_instruction));
            break;
        case 0x02:
            result = xochip_op_audio(emulator);
            break;
        case 0x07:
            result = xochip_op_ld_vx_dt(emulator, vx);
            break;
        case 0x0A:
            result = xochip_op_ld_vx_k(emulator, vx);
            break;
        case 0x15:
            result = xochip_op_ld_dt_vx(emulator, vx);
            break;
        case 0x18:
            result = xochip_op_ld_st_vx(emulator, vx);
            break;
        case 0x1E:
            result = xochip_op_add_i_vx(emulator, vx);
            break;
        case 0x29:
            result = xochip_op_ld_f_vx(emulator, vx);
            break;
        case 0x3a:
            result = xochip_op_pitch(emulator, vx);
            break;
        case 0x33:
            result = xochip_op_ld_b_vx(emulator, vx);
            break;
        case 0x55:
            result = xochip_op_ld_i_vx(emulator, vx);
            break;
        case 0x65:
            result = xochip_op_ld_vx_i(emulator, vx);
            break;
        default:
            result = XOCHIP_ERR_INVALID_INSTRUCTION;
            break;
        }
        break;
    }
    default:
        result = XOCHIP_ERR_INVALID_INSTRUCTION;
        break;
    }

    emulator->released_keys = 0;
    return result;
}

void xochip_tick(xochip_t *emulator)
{
    if (emulator->registers[XOCHIP_VSOUND] > 0)
    {
        emulator->registers[XOCHIP_VSOUND]--;
    }

    if (emulator->registers[XOCHIP_VDELAY] > 0)
    {
        emulator->registers[XOCHIP_VDELAY]--;
    }
}

void xochip_key_up(xochip_t *emulator, xochip_keys_t key)
{
    if (key < XOCHIP_KEYCOUNT)
    {
        emulator->released_keys |= XOCHIP_KEY(key);
        emulator->pressed_keys &= ~XOCHIP_KEY(key);
    }
}

void xochip_key_down(xochip_t *emulator, xochip_keys_t key)
{
    if (key < XOCHIP_KEYCOUNT)
    {
        emulator->pressed_keys |= XOCHIP_KEY(key);
        emulator->released_keys &= ~XOCHIP_KEY(key);
    }
}

const char *xochip_strerror(const xochip_result_t err)
{
    switch (err)
    {
    case XOCHIP_SUCCESS:
        return "SUCCESS";
    case XOCHIP_ERR_ROM_TOO_LARGE:
        return "ROM TOO LARGE";
    case XOCHIP_ERR_INVALID_INSTRUCTION:
        return "INVALID INSTRUCTION";
    case XOCHIP_ERR_ADDRESS_OVERFLOW:
        return "ADDRESS OVERFLOW";
    case XOCHIP_ERR_ADDRESS_UNDERFLOW:
        return "ADDRESS UNDERFLOW";
    case XOCHIP_ERR_STACK_OVERFLOW:
        return "STACK OVERFLOW";
    case XOCHIP_ERR_NULL_POINTER:
        return "NULL POINTER";
    }
    return "UNKNOWN";
}

#endif

#endif // XOCHIP_H
