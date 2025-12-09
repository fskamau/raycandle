#include "raycandle.h"
#include "raylib.h"

/**
updates the shared xlimit (updating `figure->dragger.xdata_buffer`) using
figure->dragger.xdata from `figure->dragger.start`
*/

#define RAY_WINDOW_TITLE "Rc"
void update_xlim(Figure *figure);
/* void figure_zoom(Figure* figure, int zoom); */
void figure_wait_initialized(Figure *figure);
