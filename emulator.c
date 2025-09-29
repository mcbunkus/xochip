//
// This is an example implementation targeting the desktop, using SDL3. For your own project, you can simply copy and
// paste xochip.h into a header file. This file is just for demonstration.
//

#define SDL_MAIN_USE_CALLBACKS
#include "SDL3/SDL.h"
#include "SDL3/SDL_main.h"

#define XOCHIP_IMPLEMENTATION
#include "xochip.h"

// XOCHIP 128x64 display times 10
#define WINDOW_WIDTH 1280
#define WINDOW_HEIGHT 640

// Timing stuff
#define TICK_TIME 16666667ULL
#define CYCLE_TIME 2000000ULL

typedef struct emulator_app
{
    SDL_Window *window;
    SDL_Renderer *renderer;
    xochip_t *emulator;

    uint64_t next_tick;  // for emulator timers that tick down at 60 Hz
    uint64_t next_cycle; // for emulator instruction execution, ~500 Hz but configurable for specific games

    // Maps SDL scancodes to emulator keys (I know, I was lazy here okay)
    xochip_keys_t keymap[SDL_SCANCODE_COUNT];
} emulator_app_t;

SDL_AppResult SDL_AppInit(void **app_state, int argc, char **argv)
{

    if (argc < 2)
    {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "I require a path to a ROM");
        return SDL_APP_FAILURE;
    }

    if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO))
    {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Failed to initialize SDL: %s", SDL_GetError());
        return SDL_APP_FAILURE;
    }

    emulator_app_t *app = SDL_malloc(sizeof(emulator_app_t));

    if (!app)
    {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Failed to initialize memory for application context: %s",
                     SDL_GetError());
        return SDL_APP_FAILURE;
    }

    app->emulator = SDL_malloc(sizeof(xochip_t));

    if (!app->emulator)
    {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Failed to allocate emulator context: %s", SDL_GetError());
        return SDL_APP_FAILURE;
    }

    const xochip_result_t init_result = xochip_init(app->emulator);

    if (init_result != XOCHIP_SUCCESS)
    {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Failed to initialize xochip context: %s",
                     xochip_strerror(init_result));
        return SDL_APP_FAILURE;
    }

    // setting all unhandled keymaps in keymap to something that exists outside the range of valid keys, which
    // xochip_key_down and xochip_key_up check for
    SDL_memset(app->keymap, XOCHIP_KEYCOUNT, sizeof(app->keymap));

    // Mapping physical keys to XOCHIP keys
    app->keymap[SDL_SCANCODE_1] = XOCHIP_KEY1;
    app->keymap[SDL_SCANCODE_2] = XOCHIP_KEY2;
    app->keymap[SDL_SCANCODE_3] = XOCHIP_KEY3;
    app->keymap[SDL_SCANCODE_4] = XOCHIP_KEYC;
    app->keymap[SDL_SCANCODE_Q] = XOCHIP_KEY4;
    app->keymap[SDL_SCANCODE_W] = XOCHIP_KEY5;
    app->keymap[SDL_SCANCODE_E] = XOCHIP_KEY6;
    app->keymap[SDL_SCANCODE_R] = XOCHIP_KEYD;
    app->keymap[SDL_SCANCODE_A] = XOCHIP_KEY7;
    app->keymap[SDL_SCANCODE_S] = XOCHIP_KEY8;
    app->keymap[SDL_SCANCODE_D] = XOCHIP_KEY9;
    app->keymap[SDL_SCANCODE_F] = XOCHIP_KEYE;
    app->keymap[SDL_SCANCODE_Z] = XOCHIP_KEYA;
    app->keymap[SDL_SCANCODE_X] = XOCHIP_KEY0;
    app->keymap[SDL_SCANCODE_C] = XOCHIP_KEYB;
    app->keymap[SDL_SCANCODE_V] = XOCHIP_KEYF;

    app->next_tick = 0;
    app->next_cycle = 0;

    *app_state = app;

    if (!SDL_CreateWindowAndRenderer("XOCHIP", WINDOW_WIDTH, WINDOW_HEIGHT, 0, &app->window, &app->renderer))
    {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Failed to create window and renderer: %s", SDL_GetError());
        return SDL_APP_FAILURE;
    }

    // load the ROM
    size_t romlen = 0;
    uint8_t *rom = SDL_LoadFile(argv[1], &romlen);

    if (!rom)
    {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Failed to load ROM: %s", SDL_GetError());
        return SDL_APP_FAILURE;
    }

    const xochip_result_t load_result = xochip_load_rom(app->emulator, rom, romlen);
    if (load_result != XOCHIP_SUCCESS)
    {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Failed to load ROM: %s", xochip_strerror(load_result));
        return SDL_APP_FAILURE;
    }

    return SDL_APP_CONTINUE;
}

// Calls xochip_cycle and xochip_tick when it's time. Afterwards, it will calculate the time needed until the next
// upcoming action (tick or cycle), and will sleep until it's time.
SDL_AppResult SDL_AppIterate(void *app_state)
{
    emulator_app_t *app = app_state;

    uint64_t now = SDL_GetTicksNS();

    if (app->next_cycle <= now)
    {
        xochip_cycle(app->emulator);
        app->next_cycle += CYCLE_TIME;
    }

    if (app->next_tick <= now)
    {
        xochip_tick(app->emulator);
        app->next_tick += TICK_TIME;
    }

    // determine when the next operation is, and wait until it's time
    const uint64_t next_op_time = SDL_min(app->next_cycle, app->next_tick);
    now = SDL_GetTicksNS(); // where are we after running previous instructions

    if (next_op_time > now)
    {
        SDL_DelayNS(next_op_time - now);
    }

    return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppEvent(void *app_state, SDL_Event *event)
{
    emulator_app_t *app = app_state;
    switch (event->type)
    {
    case SDL_EVENT_QUIT:
        return SDL_APP_SUCCESS;
    case SDL_EVENT_KEY_DOWN:
        // This is safe, xochip_key_down will ignore anything that's not a valid xochip key
        xochip_key_down(app->emulator, app->keymap[event->key.scancode]);
        break;
    case SDL_EVENT_KEY_UP:
        xochip_key_up(app->emulator, app->keymap[event->key.scancode]);
        break;
    default:
        break;
    }
    return SDL_APP_CONTINUE;
}

void SDL_AppQuit(void *app_state, SDL_AppResult result)
{
    emulator_app_t *app = app_state;
    if (app)
    {
        SDL_DestroyWindow(app->window);
        SDL_DestroyRenderer(app->renderer);
        SDL_free(app);
    }
}
