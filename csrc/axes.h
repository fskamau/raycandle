#include "raylib.h"

typedef struct Axes Axes;
typedef struct Figure Figure;

void axes_draw(Axes* axes);
Color axes_get_next_tableau_t10_color(Axes *axes);//returns next color pointer from tableau color array
void axes_update_ylim(Axes* axes);
void axes_set_yformatter(Axes* axes,char* format);
Axes* get_axes_under_mouse(Figure* figure);
