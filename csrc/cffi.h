/* This is the FFI ABI contract: no preprocessor, raylib, pthread, or platform structs. */
typedef struct Figure Figure;
typedef struct Axes Axes;
typedef struct Artist Artist;
typedef struct {
  unsigned char r,g,b,a;
} RColor;
typedef enum {
  ARTIST_TYPE_LINE=0,
  ARTIST_TYPE_CANDLE=1,
  ARTIST_TYPE_STUDY=2
} ArtistType;
typedef enum {
  LEGEND_POSITION_NO_LEGEND=0,
  LEGEND_POSITION_TOP_LEFT=1,
  LEGEND_POSITION_TOP_RIGHT=2,
  LEGEND_POSITION_BOTTOM_LEFT=3,
  LEGEND_POSITION_BOTTOM_RIGHT=4
} LegendPosition;
typedef struct {
  int is_splined;
  float* data;
} LineData;
Figure* figure_create(size_t rows, size_t cols);
void figure_destroy(Figure* figure);
Axes* figure_get_axes(Figure* figure, size_t index);
size_t figure_get_view_count(Figure* figure);
size_t figure_get_active_view(Figure* figure);
size_t figure_get_view_len(Figure* figure, size_t view_id);
const double* figure_view_get_epochs(Figure* figure, size_t view_id);
size_t figure_add_view(Figure* figure);
void figure_move_view_on_top(Figure* figure, size_t view_id);
void figure_view_set_data(Figure* figure, size_t view_id, size_t len, const double* epochs, size_t timeframe, const char* title);
void figure_view_update_xdata(Figure* figure, size_t view_id, const double* epochs, size_t start, size_t count);
void figure_set_xformat(Figure* figure, const char* xformat);
void figure_set_font_path(Figure* figure, const char* path);
void figure_invalidate(Figure* figure);
void figure_lock(Figure* figure);
void figure_unlock(Figure* figure);
void figure_show(Figure* figure, char* window_title, int width, int height, int fps);
void figure_wait_for_InitWindow(Figure* figure);
void figure_show_cursors(Figure* figure);
void figure_show_legends(Figure* figure, LegendPosition lp);
Artist* artist_create(Axes* axes, ArtistType atype, float thick, RColor* colors, const char* label, void* data);
float* artist_get_data_ptr(Artist* artist);
size_t artist_get_data_len(Artist* artist);
void artist_line_shift(Artist* artist, size_t count);
void artist_invalidate(Artist* artist);
void artist_set_limitable(Artist* artist, int enabled);
void artist_set_alpha(Artist* artist, unsigned char alpha);
void artist_line_set_data(Artist* artist, float* data, size_t start, size_t len);
void artist_candle_set_data(Artist* artist, float* data, size_t start, size_t len);
int artist_candle_update_tick(Artist* artist, double epoch, float price);
void axes_show_legend(Axes* axes, LegendPosition lp);
void axes_set_yformatter(Axes* axes, char* format);
void axes_set_ylim(Axes* axes, double min, double max);
