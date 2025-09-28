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

    uint64_t last_tick_time;  // for emulator timers that tick down at 60 Hz
    uint64_t last_cycle_time; // for emulator instruction execution, ~500 Hz but configurable for specific games

    // Maps SDL scancodes to emulator keys (I know, I was lazy here okay)
    xochip_keys_t keymap[SDL_SCANCODE_COUNT];
} emulator_app_t;

SDL_AppResult SDL_AppInit(void **app_state, int argc, char **argv)
{
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

    app->last_tick_time = 0;
    app->last_cycle_time = 0;

    *app_state = app;

    if (!SDL_CreateWindowAndRenderer("XOCHIP", WINDOW_WIDTH, WINDOW_HEIGHT, 0, &app->window, &app->renderer))
    {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Failed to create window and renderer: %s", SDL_GetError());
        return SDL_APP_FAILURE;
    }

    return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppIterate(void *app_state)
{
    uint64_t now = SDL_GetTicksNS();

    return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppEvent(void *app_state, const SDL_Event *event)
{
    emulator_app_t *app = app_state;
    switch (event->type)
    {
    case SDL_EVENT_QUIT:
        return SDL_APP_SUCCESS;
    case SDL_EVENT_KEY_DOWN:
        // xochip_key_down(app->emulator);
        break;
    case SDL_EVENT_KEY_UP:
        break;
    default:
        return SDL_APP_CONTINUE;
    }
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
