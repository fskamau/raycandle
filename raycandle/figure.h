#include "raycandle.h"

/**
updates the shared xlimit (updating `figure->dragger.xdata_buffer`) using figure->dragger.xdata from `figure->dragger.start`
*/
#define RAY_WINDOW_TITLE "Rc"
void update_xlim_shared(Figure* figure);
void figure_zoom(Figure* figure, int zoom);
