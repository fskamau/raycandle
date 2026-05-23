#include "raylib.h"

typedef struct Axes Axes;

void axes_draw(Axes* axes);
Color axes_get_next_tableau_t10_color(Axes *axes);//returns next color pointer from tableau color array
