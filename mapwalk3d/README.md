# mapwalk3d

A first-person raycasting maze (Wolfenstein-style), built on the mapwalk grid
and wandering monster. Reach the red finish without being caught by the yellow
monster.

## Build & run

Needs `libsdl2-dev` (`sudo apt install libsdl2-dev`). Run from this directory
so the relative `./maps/` path resolves:

```sh
make                  # incremental build -> bin/mapwalk3d
make run              # build and run (uses maps/short.txt)
make run MAP=map.txt  # run a specific map from maps/
make clean            # remove bin/
```

## Controls

| Key          | Action            |
|--------------|-------------------|
| Up / Down    | Move forward/back |
| Left / Right | Turn              |
| Esc / End    | Quit              |

## Maps

Pass a map file (relative to `./maps/`); defaults to `map.txt`:

```sh
./bin/mapwalk3d short.txt
```

Legend — `W` wall, `P` player start, `M` monster start, `F` finish,
space = floor. Up to 32×24 cells.

## Layout

- `src/game.c` — the world: map, player, monster, win/lose rules (no SDL).
- `src/render.c` — the raycaster and SDL presentation (reads game state).
- `src/main.c` — entry point, input, timing, the main loop.
- `maps/` — map data files. `bin/` — build output.
