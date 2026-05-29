#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <SDL2/SDL.h>

#define SCREEN_W 640
#define SCREEN_H 480

#define MAP_COLS 32
#define MAP_ROWS 24

#define FOV 1.0472f  // 60 degrees in radians

#define MOVE_SPEED 0.05f
#define ROT_SPEED  0.03f

#define CHAR_EMPTY    ' '
#define CHAR_WALL     'W'
#define CHAR_PLAYER   'P'
#define CHAR_MONSTER  'M'
#define CHAR_FINISH   'F'

#define CHAR_WIN      CHAR_PLAYER
#define CHAR_LOSE     CHAR_MONSTER

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

typedef struct {
    int dx,dy;
} Move;

const Move mv_left  = {-1,  0};
const Move mv_right = { 1,  0};
const Move mv_up    = { 0, -1};
const Move mv_down  = { 0,  1};

struct Player {
    float x, y;
    float angle;
} player;

struct Monster {
    int x,y;
    const Move *dir;
} monster;

char grid[MAP_COLS][MAP_ROWS];
bool game_end;
char game_result;

#define MAP_PATH "./maps/"
#define DEFAULT_MAP "map.txt"

char map[MAP_ROWS][MAP_COLS + 1];

struct Gpu {
    SDL_Window* window;
    SDL_Renderer* renderer;
    SDL_Texture* texture;
} gpu;

uint32_t pixels[SCREEN_W * SCREEN_H];

void load_map(const char* filename) {
    char path[256];
    snprintf(path, sizeof path, "%s%s", MAP_PATH, filename);
    FILE* f = fopen(path, "r");
    if (!f) {
        printf("Error: cannot open map file: %s (%s)\n", filename, path);
        exit(1);
    }

    int max_len = 0, total_rows = 0;
    char line[256];
    for (int r = 0; r < MAP_ROWS; r++) {
        if (!fgets(line, sizeof line, f))
            break; // EOF

        int len = strlen(line);
        max_len = (len > max_len) ? len : max_len;
        if (len > 0 && line[len - 1] == '\n')
            line[--len] = '\0';

        memcpy(map[r], line, len + 1);
        total_rows += 1;
    }

    fclose(f);
    printf("Loaded map '%s' (%dx%d)\n", filename, max_len, total_rows);
}

void init_game() {
    printf("Initialising game ...\n");

    memset(&grid, 0, sizeof grid);

    for (int r = 0; r < MAP_ROWS; r++) {
        for (int c = 0; c < MAP_COLS; c++) {
            char cell = map[r][c];
            if (!cell) {
                cell = CHAR_EMPTY;
                continue;
            }
            switch (cell) {
                case CHAR_EMPTY:
                case CHAR_WALL:
                case CHAR_FINISH:
                    grid[c][r] = cell;
                    break;
                case CHAR_PLAYER:
                    player.x = c + 0.5f;
                    player.y = r + 0.5f;
                    player.angle = 0.0f;
                    break;
                case CHAR_MONSTER:
                    monster.x = c;
                    monster.y = r;
                    monster.dir = NULL;
                    break;
                default:
                    printf("Invalid map data!\n\n");
                    printf("Error: invalid char: '%c' at %d,%d\n\n", cell, r, c);
                    exit(1);
            }
        }
    }

    game_end = false;
    game_result = 0;
}

const Move* inv_move(const Move* m) {
    if (m == &mv_left)
        return &mv_right;
    else if (m == &mv_right)
        return &mv_left;
    else if (m == &mv_up)
        return &mv_down;
    else if (m == &mv_down)
        return &mv_up;
}

void fin(char c) {
    game_result = c;
    game_end = true;
}

void move(const uint8_t* keys) {
    float dx = cosf(player.angle);
    float dy = sinf(player.angle);

    float nx = player.x;
    float ny = player.y;

    if (keys[SDL_SCANCODE_UP]) {
        nx += dx * MOVE_SPEED;
        ny += dy * MOVE_SPEED;
    }
    if (keys[SDL_SCANCODE_DOWN]) {
        nx -= dx * MOVE_SPEED;
        ny -= dy * MOVE_SPEED;
    }
    if (keys[SDL_SCANCODE_LEFT])
        player.angle -= ROT_SPEED;
    if (keys[SDL_SCANCODE_RIGHT])
        player.angle += ROT_SPEED;

    // Collision: check each axis independently for wall sliding.
    if (grid[(int)nx][(int)player.y] != CHAR_WALL)
        player.x = nx;
    if (grid[(int)player.x][(int)ny] != CHAR_WALL)
        player.y = ny;

    // Check for finish.
    if (grid[(int)player.x][(int)player.y] == CHAR_FINISH)
        fin(CHAR_WIN);
}

void render_game_end() {
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
RayHit cast_ray(float ray_angle) {
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
Face hit_face(RayHit h) {
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
uint32_t wall_colour(RayHit h) {
    if (h.hit_char == CHAR_MONSTER)
        return COLOUR_MONSTER;
    if (h.hit_char == CHAR_FINISH)
        return COLOUR_FINISH;
    return WALL_FACES[hit_face(h)];
}

// Draw one vertical screen column: ceiling above, wall slice, floor below.
void draw_column(int x, float dist, uint32_t colour) {
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
void present_frame() {
    SDL_UpdateTexture(gpu.texture, NULL, pixels, SCREEN_W * sizeof(uint32_t));
    SDL_RenderCopy(gpu.renderer, gpu.texture, NULL, NULL);
    SDL_RenderPresent(gpu.renderer);
}

void render() {
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

void move_monster() {
    if (monster.dir == NULL)
        monster.dir = &mv_left;

    int x = monster.x + monster.dir->dx;
    int y = monster.y + monster.dir->dy;

    if (grid[x][y] == CHAR_WALL) {
        monster.dir = inv_move(monster.dir);
    } else {
        monster.x = x;
        monster.y = y;
    }
}

void check_collision() {
    if (monster.x == (int)player.x && monster.y == (int)player.y) {
        fin(CHAR_LOSE);
    }
}

void update_world() {
    move_monster();
    check_collision();
}

void init_gpu() {
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

static bool done() {
    SDL_Event event;
    SDL_PollEvent(&event);
    return event.type == SDL_QUIT
        || event.key.keysym.sym == SDLK_END
        || event.key.keysym.sym == SDLK_ESCAPE;
}

int main(int argc, char *argv[]) {
    const char* mapfile = (argc > 1) ? argv[1] : DEFAULT_MAP;
    load_map(mapfile);
    init_gpu();
    init_game();

    int last_world = SDL_GetTicks();

    while (!done()) {
        const int t0 = SDL_GetTicks();

        const uint8_t* keys = SDL_GetKeyboardState(NULL);
        move(keys);

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

    SDL_DestroyRenderer(gpu.renderer);
    SDL_Quit();
}
