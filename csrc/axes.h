#include "raylib.h"

#define LEGEND_GAP     5
#define LEGEND_ICON_LEN 15


typedef struct Axes Axes;
typedef struct Figure Figure;
typedef struct AxesView AxesView;

void axes_draw(Axes* axes);
Color axes_get_next_tableau_t10_color(Axes *axes);//returns next color pointer from tableau color array
void axes_update_ylim(Axes* axes);
Axes* get_axes_under_mouse(Figure* figure);
AxesView* get_axes_view_under_mouse(Figure* figure);
