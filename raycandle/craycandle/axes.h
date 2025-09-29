#include "raycandle.h"

void create_axes(Figure *figure, char *labels); // allocate memory and create axes structs in it
bool draw_axes(Axes *axes); // plot the axes and all artists for the axes
/*
return index of axes under mouse. if none return `figure->axes_len`
*/
uint8_t get_axes_index_under_mouse(Figure *figure);
void axes_set_legend(Figure* figure);
