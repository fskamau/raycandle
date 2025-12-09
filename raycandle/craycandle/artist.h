#include "locator.h"

Artist *get_artist(Axes *axes, size_t index);
void artist_update_data_buffer(Artist *artist, LimitChanged lim);
void draw_artist(Artist *artist);
void artist_draw_icon(
    Artist *artist,
    Vector2 startPos); // draw a small shape of len  RC_LEGEND_ICON_WIDTH
