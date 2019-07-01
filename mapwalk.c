#include <stdbool.h>
#include <SDL2/SDL.h>

#define SCREEN_W 640
#define SCREEN_H 480

#define SQ_W 20

#define COLS (SCREEN_W / SQ_W)
#define ROWS (SCREEN_H / SQ_W)

#define CHAR_EMPTY  ' '
#define CHAR_WALL   'W'
#define CHAR_PLAYER 'P'
#define CHAR_FINISH 'F'

struct Player {
    int x,y;
} player;

typedef struct {
    int dx,dy;
} Move;

const Move mv_left  = {-1,  0};
const Move mv_right = { 1,  0};
const Move mv_up    = { 0, -1};
const Move mv_down  = { 0,  1};

typedef struct {
    Uint8 r,g,b;
} Colour;

const Colour grey = { 0xCC, 0xCC, 0xCC };
const Colour cyan = { 0x00, 0xFF, 0xFF };
const Colour red  = { 0xFF, 0x00, 0x00 };

char grid[COLS][ROWS] = {};
bool game_end;

const char* map[] = { // 32x24
    "WWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWW",
    "W                              W",
    "WWWW WWWWWWWWWWWWW WWWW WWW  WWW",
    "W WW      WWW   WW W WWWWWWW  WW",
    "W  WWWWWW                     WW",
    "WW         WWWWWW  WWWWWWWWWW  W",
    "WWWWWWWWWW WWWWWW      WWWWWWW W",
    "WWWWWWWWWW WWWWWWWWWWWWWWWW    W",
    "W                         W  WWW",
    "WW  WWWWWWWWWWWWW  WWW   WW WWWW",
    "WW  WWWW W WWWWWWW WWWWW  W WWWW",
    "WW              WW    WW    WWWW",
    "WWWWWWWWWWWWWW  WWWWWWWWWWWWWWWW",
    "WWW  WWWWWWWWW                 W",
    "WWW          WWWWWWWWWW  WWWWW W",
    "WWW WWWW  WWWWWWWWWWWWW  WW  W W",
    "WWW    W                 WW W  W",
    "WWWWWWWWWWWWWWWWWWWWWWW  W  W WW",
    "W       WWWWWWWWWW       W WW WW",
    "W  WWW         WWW WWWWWWW WW WW",
    "WW WWWWWWWWWW  WWW WWWWWWW WW WW",
    "WW          W                 WW",
    "WWWWWWWW    WWWWWWWWW W WWW WWWW",
    "WWWWWWWWFFFFWWWWWWWWWWWWWWWWWWWW",
};

struct Gpu {
    SDL_Window* window;
    SDL_Renderer* renderer;
    SDL_Texture* texture;
} gpu;

void init_map() {
    for (int r = 0; r < ROWS; r++) {
        for (int c = 0; c < COLS; c++) {
            char cell = map[r][c];
            switch (cell) {
                case CHAR_EMPTY:
                case CHAR_WALL:
                case CHAR_FINISH:
                    grid[c][r] = cell;
                    break;
                default:
                    printf("Invalid map data!\n\n");
                    printf("Error: invalid char: '%c' at %d,%d\n\n", cell, r, c);
                    exit(1);
            }
        }
    }
}

void init_player() {
    for (int c = 0; c < COLS; c++) {
        for (int r = 0; r < ROWS; r++) {
            if (grid[c][r] == CHAR_EMPTY) {
                grid[c][r] = CHAR_PLAYER;
                player.x = c;
                player.y = r;
                return;
            }
        }
    }

    printf("Error: no empty map cells\n");
    exit(1);
}

void init_game() {
    printf("Initialising game ...\n");

    memset(&grid, 0, sizeof grid);

    init_map();
    init_player();

    game_end = false;
}

void plot_player(const Move* m) {
    int x = player.x + m->dx;
    int y = player.y + m->dy;

    switch (grid[x][y]) {
        case CHAR_WALL:
            break; // do nothing.
        case CHAR_FINISH:
            game_end = true; // Win!
            memset(grid, CHAR_PLAYER, sizeof grid);
            break;
        case CHAR_EMPTY:
            grid[player.x][player.y] = CHAR_EMPTY;
            player.x = x;
            player.y = y;
            grid[player.x][player.y] = CHAR_PLAYER;
            break;
    }
}

void move(const uint8_t* event) {
    const Move *m = NULL;

    if (event[SDL_SCANCODE_UP])
        m = &mv_up;
    if (event[SDL_SCANCODE_DOWN])
        m = &mv_down;
    if (event[SDL_SCANCODE_LEFT])
        m = &mv_left;
    if (event[SDL_SCANCODE_RIGHT])
        m = &mv_right;

    if (m == NULL)
        return;

    // Move player.
    plot_player(m);
}

void draw_rect(int col, int row, char item) {
    SDL_Rect r;
    r.x = SQ_W *col;
    r.y = SQ_W *row;
    r.w = SQ_W;
    r.h = SQ_W;

    const Colour *c = NULL;
    switch (item) {
        case CHAR_WALL:
            c = &grey; break;
        case CHAR_PLAYER:
            c = &cyan; break;
        case CHAR_FINISH:
            c = &red; break;
    }

    if (c == NULL)
        return;

    SDL_RenderDrawRect(gpu.renderer, &r);
    SDL_SetRenderDrawColor(gpu.renderer, c->r, c->g, c->b, 0x00);
    SDL_RenderFillRect(gpu.renderer, &r);
}

void render() {
    SDL_SetRenderTarget(gpu.renderer, gpu.texture);
    SDL_SetRenderDrawColor(gpu.renderer, 0x00, 0x00, 0x00, 0x00);
    SDL_RenderClear(gpu.renderer);

    for (int i=0; i < COLS; i++) {
        for (int j=0; j < ROWS; j++) {
            if (grid[i][j] > 0)
                draw_rect(i, j, grid[i][j]);
        }
    }

    SDL_SetRenderTarget(gpu.renderer, NULL);
    SDL_RenderCopy(gpu.renderer, gpu.texture, NULL, NULL);

    SDL_RenderPresent(gpu.renderer);
}

void init_gpu() {
    printf("Initialising GPU ...\n");

    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        puts(SDL_GetError());
        exit(1);
    }

    gpu.window = SDL_CreateWindow("Map Walk",
        SDL_WINDOWPOS_UNDEFINED,
        SDL_WINDOWPOS_UNDEFINED,
        SCREEN_W, SCREEN_H,
        SDL_WINDOW_SHOWN);

    gpu.renderer = SDL_CreateRenderer(gpu.window, -1, SDL_RENDERER_ACCELERATED);

    gpu.texture = SDL_CreateTexture(
        gpu.renderer,
        SDL_PIXELFORMAT_ARGB8888,
        SDL_TEXTUREACCESS_TARGET,
        SCREEN_W, SCREEN_H);

    if (gpu.window == NULL || gpu.renderer == NULL || gpu.texture == NULL) {
        puts(SDL_GetError());
        exit(1);
    }
}

static bool done() {
    SDL_Event event;
    SDL_PollEvent(&event);
    return event.type == SDL_QUIT
        || event.key.keysym.sym == SDLK_END
        || event.key.keysym.sym == SDLK_ESCAPE;
}

int main(int argc, char *argv[]) {
    init_gpu();
    init_game();

    while (!done()) {
        const int t0 = SDL_GetTicks();

        const uint8_t* event = SDL_GetKeyboardState(NULL);
        move(event);

        render();

        // Restart.
        if (game_end) {
            SDL_Delay(700);
            init_game();
        }

        // min 70ms loop.
        const int t1 = SDL_GetTicks();
        const int ms = 70 - (t1 - t0);
        SDL_Delay(ms < 0 ? 0 : ms);
    }

    SDL_DestroyRenderer(gpu.renderer);
    SDL_Quit();
}
