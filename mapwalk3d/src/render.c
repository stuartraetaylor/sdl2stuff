#include "render.h"
#include "game.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <math.h>
#include <SDL2/SDL.h>

#define SCREEN_W 640
#define SCREEN_H 480

#define FOV 1.0472f  // 60 degrees in radians

#define COLOUR_CEILING   0xFF333333
#define COLOUR_FLOOR     0xFF666666
#define COLOUR_WALL_N    0xFFCCC6BC   // warm stone, lit (north face)
#define COLOUR_WALL_S    0xFF847E74   // warm stone, shadowed (south face)
#define COLOUR_WALL_E    0xFFB3ADA3   // warm stone, mid (east face)
#define COLOUR_WALL_W    0xFF9C968C   // warm stone, dim (west face)
#define COLOUR_FINISH    0xFFDD2222   // red marker, embedded in a wall
#define COLOUR_MONSTER   0xFFFFFF00
#define COLOUR_WIN       0xFF00FFFF
#define COLOUR_LOSE      0xFFFFFF00

static struct Gpu {
    SDL_Window* window;
    SDL_Renderer* renderer;
    SDL_Texture* texture;
} gpu;

static uint32_t pixels[SCREEN_W * SCREEN_H];

static void render_game_end(void) {
    uint32_t col = (game_result == CHAR_WIN) ? COLOUR_WIN : COLOUR_LOSE;
    for (int i = 0; i < SCREEN_W * SCREEN_H; i++)
        pixels[i] = col;
}

// Result of casting one ray into the grid.
typedef struct {
    char  hit_char;   // cell the ray struck (wall / finish / monster)
    int   hit_y_side; // 0 = x-side hit (E/W face), 1 = y-side hit (N/S face)
    int   step_x;     // ray's x step direction (-1 or +1)
    int   step_y;     // ray's y step direction (-1 or +1)
    float distance;   // perpendicular distance to the hit (fisheye-corrected)
} RayHit;

// Cast a single ray from the player and walk the grid (DDA) until it hits
// a wall, the finish, or the monster.
static RayHit cast_ray(float ray_angle) {
    float ray_dx = cosf(ray_angle);
    float ray_dy = sinf(ray_angle);

    // DDA setup.
    int map_x = (int)player.x;
    int map_y = (int)player.y;

    float delta_x = fabsf(1.0f / ray_dx);
    float delta_y = fabsf(1.0f / ray_dy);

    int step_x, step_y;
    float side_x, side_y;

    if (ray_dx < 0) {
        step_x = -1;
        side_x = (player.x - map_x) * delta_x;
    } else {
        step_x = 1;
        side_x = (map_x + 1.0f - player.x) * delta_x;
    }
    if (ray_dy < 0) {
        step_y = -1;
        side_y = (player.y - map_y) * delta_y;
    } else {
        step_y = 1;
        side_y = (map_y + 1.0f - player.y) * delta_y;
    }

    // DDA: step through grid until we hit something.
    int side = 0;
    char hit = CHAR_EMPTY;
    while (hit == CHAR_EMPTY) {
        if (side_x < side_y) {
            side_x += delta_x;
            map_x += step_x;
            side = 0;
        } else {
            side_y += delta_y;
            map_y += step_y;
            side = 1;
        }
        if (map_x < 0 || map_x >= MAP_COLS || map_y < 0 || map_y >= MAP_ROWS)
            break;
        char cell = grid[map_x][map_y];
        if (cell != CHAR_EMPTY)
            hit = cell;
        else if (map_x == monster.x && map_y == monster.y)
            hit = CHAR_MONSTER;
    }

    // Perpendicular distance (avoids fisheye).
    float wall_dist;
    if (side == 0)
        wall_dist = side_x - delta_x;
    else
        wall_dist = side_y - delta_y;
    if (wall_dist < 0.001f)
        wall_dist = 0.001f;

    return (RayHit){ hit, side, step_x, step_y, wall_dist };
}

typedef enum { FACE_N, FACE_S, FACE_E, FACE_W } Face;

// Which cardinal face of the cell did the ray strike?
static Face hit_face(RayHit h) {
    if (h.hit_y_side)                        // N/S face (ray crossed a y-line)
        return (h.step_y > 0) ? FACE_N : FACE_S;
    else
        return (h.step_x > 0) ? FACE_W : FACE_E;  // E/W face (ray crossed an x-line)
}

// Wall colours, one shade per cardinal face (indexed by Face).
static const uint32_t WALL_FACES[4] = {
    COLOUR_WALL_N, COLOUR_WALL_S, COLOUR_WALL_E, COLOUR_WALL_W,
};

// Pick a colour for the hit. Monster and finish are flat; walls are face-shaded.
static uint32_t wall_colour(RayHit h) {
    if (h.hit_char == CHAR_MONSTER)
        return COLOUR_MONSTER;
    if (h.hit_char == CHAR_FINISH)
        return COLOUR_FINISH;
    return WALL_FACES[hit_face(h)];
}

// Draw one vertical screen column: ceiling above, wall slice, floor below.
static void draw_column(int x, float dist, uint32_t colour) {
    int wall_h = (int)(SCREEN_H / dist);
    int top = (SCREEN_H - wall_h) / 2;
    int bot = top + wall_h;
    if (top < 0) top = 0;
    if (bot > SCREEN_H) bot = SCREEN_H;

    for (int y = 0; y < top; y++)
        pixels[y * SCREEN_W + x] = COLOUR_CEILING;
    for (int y = top; y < bot; y++)
        pixels[y * SCREEN_W + x] = colour;
    for (int y = bot; y < SCREEN_H; y++)
        pixels[y * SCREEN_W + x] = COLOUR_FLOOR;
}

// Push the pixel buffer to the screen.
static void present_frame(void) {
    SDL_UpdateTexture(gpu.texture, NULL, pixels, SCREEN_W * sizeof(uint32_t));
    SDL_RenderCopy(gpu.renderer, gpu.texture, NULL, NULL);
    SDL_RenderPresent(gpu.renderer);
}

void render(void) {
    if (game_end) {
        render_game_end();
    } else {
        for (int x = 0; x < SCREEN_W; x++) {
            // Ray angle for this column, fanned across the field of view.
            float ray_angle = player.angle - FOV / 2.0f
                            + FOV * ((float)x / SCREEN_W);
            RayHit h = cast_ray(ray_angle);
            draw_column(x, h.distance, wall_colour(h));
        }
    }
    present_frame();
}

void render_init(void) {
    printf("Initialising display ...\n");

    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        puts(SDL_GetError());
        exit(1);
    }

    gpu.window = SDL_CreateWindow("Map Walk 3D",
        SDL_WINDOWPOS_UNDEFINED,
        SDL_WINDOWPOS_UNDEFINED,
        SCREEN_W, SCREEN_H,
        SDL_WINDOW_SHOWN);

    gpu.renderer = SDL_CreateRenderer(gpu.window, -1, SDL_RENDERER_ACCELERATED);

    gpu.texture = SDL_CreateTexture(
        gpu.renderer,
        SDL_PIXELFORMAT_ARGB8888,
        SDL_TEXTUREACCESS_STREAMING,
        SCREEN_W, SCREEN_H);

    if (gpu.window == NULL || gpu.renderer == NULL || gpu.texture == NULL) {
        puts(SDL_GetError());
        exit(1);
    }

    SDL_RaiseWindow(gpu.window);
}

void render_shutdown(void) {
    SDL_DestroyRenderer(gpu.renderer);
    SDL_Quit();
}
