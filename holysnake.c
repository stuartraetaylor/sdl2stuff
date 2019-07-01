#include <stdbool.h>
#include <time.h>

#include <SDL2/SDL.h>

// Game params.
#define DEBUG 1
#define SLOW 0

// Snake.
#define MIN_LEN 1
#define MAX_LEN 2048
#define STEP 3

// Screen.
#define SCREEN_W 640
#define SCREEN_H 480

#define SQ_W 16

#define COLS (SCREEN_W / SQ_W)
#define ROWS (SCREEN_H / SQ_W)

#define CHAR_EMPTY 0
#define CHAR_SNAKE 1
#define CHAR_APPLE 2

// Movement.
typedef struct {
    int dx,dy;
} Move;

const Move mv_left  = {-1,  0};
const Move mv_right = { 1,  0};
const Move mv_up    = { 0, -1};
const Move mv_down  = { 0,  1};

// Colour.
typedef struct {
    Uint8 r,g,b;
} Colour;

const Colour red  = { 0xFF, 0x00, 0x00 };
const Colour cyan = { 0x00, 0xFF, 0xFF }; 

// Snake.
struct Snake {
    int x,y,len,
        tail_x[MAX_LEN],
        tail_y[MAX_LEN];
    const Move *dir;
} snake;

// Apple.
struct Apple {
    int x,y;
} apple;

// Game.
bool game_end;
char grid[COLS][ROWS] = {};

struct Gpu {
    SDL_Window* window;
    SDL_Renderer* renderer;
    SDL_Texture* texture;
} gpu;

int score() { return snake.len - MIN_LEN; }

void init_game() {
    memset(&grid, 0, sizeof grid);

    snake.x = 10;
    snake.y = 10;
    snake.dir = &mv_right;
    snake.len = MIN_LEN;  

    memset(&snake.tail_x, 0, sizeof snake.tail_x);
    memset(&snake.tail_y, 0, sizeof snake.tail_y);

    apple.x = 15;
    apple.y = ROWS / 2;

    srand(time(NULL)); 

    game_end = false;
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

void plot_snake() {
    // Move tail.
    int px = snake.tail_x[0],
        py = snake.tail_y[0];

    snake.tail_x[0] = snake.x;
    snake.tail_y[0] = snake.y;
  
    for (int i=1; i < snake.len; i++) {
        int tx = snake.tail_x[i],
            ty = snake.tail_y[i];

        snake.tail_x[i] = px;
        snake.tail_y[i] = py;

        px = tx;
        py = ty;
    }

    // Move head.
    snake.x += snake.dir->dx;
    snake.y += snake.dir->dy;

    // Detect tail collision.
    for (int i=0; i < snake.len; i++) {
        if (snake.x == snake.tail_x[i] && snake.y == snake.tail_y[i]) {
            game_end = true; // Lose!
            memset(grid, CHAR_APPLE, sizeof grid);
            return;
        }
    }

    // Wrap screen.
    if (snake.x >= COLS)
        snake.x = 0;
    if (snake.y >= ROWS)
        snake.y = 0;

    if (snake.x < 0)
        snake.x = COLS-1;
    if (snake.y < 0)
        snake.y = ROWS-1;

    // Plot snake.
    grid[snake.x][snake.y] = CHAR_SNAKE;
  
    for (int i=0; i < snake.len; i++)
        grid[snake.tail_x[i]][snake.tail_y[i]] = CHAR_SNAKE;

}

void plot_apple() {
    // Detect collision.
    if (snake.x == apple.x && snake.y == apple.y) {
        grid[apple.x][apple.y] = CHAR_EMPTY;
        snake.len += STEP + (snake.len/5.0);

        apple.x = rand() % COLS;
        apple.y = rand() % ROWS;
    }

    if (snake.len == MAX_LEN) {
        game_end = true; // Win!
        memset(&grid, CHAR_SNAKE, sizeof grid);
        return;
    }

    // Plot.
    grid[apple.x][apple.y] = CHAR_APPLE;
}

void move(const uint8_t* key) {
    const Move *d = NULL;

    if (key[SDL_SCANCODE_UP])
        d = &mv_up;
    if (key[SDL_SCANCODE_DOWN])
        d = &mv_down;
    if (key[SDL_SCANCODE_LEFT])
        d = &mv_left;
    if (key[SDL_SCANCODE_RIGHT])
        d = &mv_right;
        
    if (d == NULL)
        return;

    if (d == inv_move(snake.dir))
        return;

    // Change direction.
    snake.dir = d;
}

void draw_rect(int col, int row, char item) {
    SDL_Rect r;
    r.x = SQ_W *col;
    r.y = SQ_W *row;
    r.w = SQ_W;
    r.h = SQ_W;

    const Colour *c;
    switch (item) {
        case CHAR_SNAKE:
            c = &cyan; break;
        case CHAR_APPLE:
            c = &red; break;
    }

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
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        puts(SDL_GetError());
        exit(1);
    }

    gpu.window = SDL_CreateWindow("Holy Snake (heretic edition)",
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
        memset(&grid, 0, sizeof grid);

        const uint8_t* key = SDL_GetKeyboardState(NULL);
        move(key);

        plot_apple();
        plot_snake();
        render();

        float delay = 100*(1+SLOW)-score();
        SDL_Delay(delay);

        // Restart.
        if (game_end) {
            SDL_Delay(700);
            init_game();
        }
    }

    SDL_DestroyRenderer(gpu.renderer);
    SDL_Quit();
}
