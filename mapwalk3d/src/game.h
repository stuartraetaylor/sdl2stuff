#ifndef GAME_H
#define GAME_H

#include <stdbool.h>

// Grid dimensions (cells).
#define MAP_COLS 32
#define MAP_ROWS 24

// Player movement per frame.
#define MOVE_SPEED 0.05f
#define ROT_SPEED  0.03f

// Map cell characters.
#define CHAR_EMPTY    ' '
#define CHAR_WALL     'W'
#define CHAR_PLAYER   'P'
#define CHAR_MONSTER  'M'
#define CHAR_FINISH   'F'

// Win/lose markers (reuse the cell chars).
#define CHAR_WIN      CHAR_PLAYER
#define CHAR_LOSE     CHAR_MONSTER

// Default map filename (resolved under ./maps/ by load_map).
#define DEFAULT_MAP "map.txt"

typedef struct {
    int dx, dy;
} Move;

typedef struct {
    float x, y;
    float angle;
} Player;

typedef struct {
    int x, y;
    const Move *dir;
} Monster;

// World state, read by the renderer.
extern Player  player;
extern Monster monster;
extern char    grid[MAP_COLS][MAP_ROWS];
extern bool    game_end;
extern char    game_result;

// Load a map file (under ./maps/) into the internal map buffer.
void load_map(const char* filename);

// (Re)initialise the world from the loaded map.
void init_game(void);

// Advance the player from input intent: forward/back move, left/right turn.
void player_move(bool fwd, bool back, bool turn_left, bool turn_right);

// Advance non-player world state (monster + collision) by one tick.
void update_world(void);

#endif // GAME_H
