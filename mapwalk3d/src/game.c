#include "game.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#define MAP_PATH "./maps/"

static const Move mv_left  = {-1,  0};
static const Move mv_right = { 1,  0};
static const Move mv_up    = { 0, -1};
static const Move mv_down  = { 0,  1};

Player  player;
Monster monster;
char    grid[MAP_COLS][MAP_ROWS];
bool    game_end;
char    game_result;

static char map[MAP_ROWS][MAP_COLS + 1];

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

void init_game(void) {
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
                    grid[c][r] = CHAR_EMPTY;
                    player.x = c + 0.5f;
                    player.y = r + 0.5f;
                    player.angle = 0.0f;
                    break;
                case CHAR_MONSTER:
                    grid[c][r] = CHAR_EMPTY;
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

static const Move* inv_move(const Move* m) {
    if (m == &mv_left)
        return &mv_right;
    else if (m == &mv_right)
        return &mv_left;
    else if (m == &mv_up)
        return &mv_down;
    else if (m == &mv_down)
        return &mv_up;
    return m; // unreachable for the four cardinal moves; silences -Wreturn-type
}

static void fin(char c) {
    game_result = c;
    game_end = true;
}

void player_move(bool fwd, bool back, bool turn_left, bool turn_right) {
    float dx = cosf(player.angle);
    float dy = sinf(player.angle);

    float nx = player.x;
    float ny = player.y;

    if (fwd) {
        nx += dx * MOVE_SPEED;
        ny += dy * MOVE_SPEED;
    }
    if (back) {
        nx -= dx * MOVE_SPEED;
        ny -= dy * MOVE_SPEED;
    }
    if (turn_left)
        player.angle -= ROT_SPEED;
    if (turn_right)
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

static void move_monster(void) {
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

static void check_collision(void) {
    if (monster.x == (int)player.x && monster.y == (int)player.y) {
        fin(CHAR_LOSE);
    }
}

void update_world(void) {
    move_monster();
    check_collision();
}
