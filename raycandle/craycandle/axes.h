#include "raycandle.h"
#include "utils.h"

void create_axes(Figure *figure, char *labels); // allocate memory and create axes structs in it
bool draw_axes(Axes *axes); // plot the axes and all artists for the axes
/*
return index of axes under mouse. if none return `figure->axes_len`
*/
size_t get_axes_index_under_mouse(Figure *figure);
RC_ONCE_AFTER_INIT_WINDOW void axes_set_legend(Figure* figure);
