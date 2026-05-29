#include <stdbool.h>
#include <stdint.h>
#include <SDL2/SDL.h>

#include "game.h"
#include "render.h"

static bool done(void) {
    SDL_Event event;
    SDL_PollEvent(&event);
    return event.type == SDL_QUIT
        || event.key.keysym.sym == SDLK_END
        || event.key.keysym.sym == SDLK_ESCAPE;
}

int main(int argc, char *argv[]) {
    const char* mapfile = (argc > 1) ? argv[1] : DEFAULT_MAP;
    load_map(mapfile);
    render_init();
    init_game();

    int last_world = SDL_GetTicks();

    while (!done()) {
        const int t0 = SDL_GetTicks();

        const uint8_t* keys = SDL_GetKeyboardState(NULL);
        player_move(keys[SDL_SCANCODE_UP],   keys[SDL_SCANCODE_DOWN],
                    keys[SDL_SCANCODE_LEFT],  keys[SDL_SCANCODE_RIGHT]);

        // Update monster at ~70ms intervals (matching mapwalk2 tick rate).
        if (!game_end && t0 - last_world >= 70) {
            update_world();
            last_world = t0;
        }

        render();

        // Restart.
        if (game_end) {
            SDL_Delay(700);
            init_game();
            last_world = SDL_GetTicks();
        }

        // min 30ms loop (~33 fps).
        const int t1 = SDL_GetTicks();
        const int ms = 30 - (t1 - t0);
        SDL_Delay(ms < 0 ? 0 : ms);
    }

    render_shutdown();
}
