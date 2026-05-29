#ifndef RENDER_H
#define RENDER_H

// Create the window/renderer/texture (also initialises SDL video).
void render_init(void);

// Render the current world state to the screen.
void render(void);

// Tear down the renderer and SDL.
void render_shutdown(void);

#endif // RENDER_H
