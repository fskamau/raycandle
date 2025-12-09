#include "figure.h"

#include <locale.h>
#include <math.h>
#include <stdbool.h>
#include <string.h>
#include <sys/time.h>
#include <time.h>
#include <unistd.h>

#include "axes.h"
#include "fas.h"
#include "locator.h"
#include "mouse_updater.h"
#include "raycandle.h"
#include "ready_signal.h"
#include "utils.h"

static void figure_draw_cursors(Figure *figure);    // draw cursor positions
static bool set_real_span_skel_map(Figure *figure); // map axes skeletons to real pixels
static bool set_borders(Figure *figure);            // returns if borders can be set
static void update_fps(Figure *figure);             // set fps according to figure->fps
static void draw_title(Figure *figure);             // draw title
static void draw_tooltip(Figure *figure);           // draw tooltip data
static void draw_current_time(Figure *figure);      // show current time. might be used if the
                                                    // app is doing nothing
static void load_font(Figure *figure);

static int processId = 0;

static void figure_draw_cursors(Figure *figure) {
  Axes *axes;
  if (!(axes = get_axes_under_mouse(figure)))
    return;
  double xdata, ydata, zdata;
  Vector2 v = GetMousePosition();
  if (axes->artist_len == 0) {
    return;
  } // all axes share same x-axis
  xdata = (double)(v.x - axes->startX) / axes->width;
  ydata = (double)(v.y - axes->startY) / axes->height;
  for (size_t i = 0; i < figure->axes_len; ++i) {
    if ((axes = &figure->axes[i])->artist_len == 0) {
      continue;
    }
    zdata = xdata * axes->width + axes->startX;
    DrawLine(zdata, axes->startY, zdata, axes->startY + axes->height, figure->text_color);
    zdata = ydata * axes->height + axes->startY;
    DrawLine(axes->startX, zdata, axes->startX + axes->width, zdata, figure->text_color);
  }
}

static bool set_real_span_skel_map(Figure *figure) {
  switch (figure->sds) {
  case SCREEN_DIMENSION_STATE_DEFAULT:
    RC_ERROR("condition '%s' is unreachable after InitWindow!\n", RC_ECHO(SCREEN_DIMENSION_STATE_DEFAULT));
    return false; // not reachable
  case SCREEN_DIMENSION_STATE_UNCHANGED: {
    for (size_t axes_index = 0; axes_index < figure->axes_len; axes_index++) {
      size_t index = axes_index * 4;
      figure->axes[axes_index].startX = figure->axes_skels_copy[index];
      figure->axes[axes_index].startY = figure->axes_skels_copy[index + 1];
      figure->axes[axes_index].width = figure->axes_skels_copy[index + 2];
      figure->axes[axes_index].height = figure->axes_skels_copy[index + 3];
    }
    break;
  }
  case SCREEN_DIMENSION_STATE_CHANGED:
    size_t axes_skels_copy[4] = {0};
    long int x = (long int)figure->height - (figure->font_size * (figure->title == NULL ? 1 : 2)); // may underflow if height<font_size
    if (x < 0) {
      return false;
    }
    size_t axes_sizes[] = {figure->width / (figure->cols), x / figure->rows};
    size_t b0;
    for (size_t axes_index = 0; axes_index < figure->axes_len; axes_index++) {
      size_t index = axes_index * 4;
      // set new absolute measurements
      axes_skels_copy[0] = axes_sizes[0] * figure->axes_skels[index];
      axes_skels_copy[1] = axes_sizes[1] * figure->axes_skels[index + 1] + (figure->title == NULL ? 0 : figure->font_size);
      axes_skels_copy[2] = axes_sizes[0] * figure->axes_skels[index + 2];
      axes_skels_copy[3] = axes_sizes[1] * figure->axes_skels[index + 3];
      // ajdust the measurents  by removing border, creating space at the
      // top for title height if title!=null   and creating space at the
      // bottom for tooltip
      b0 = (size_t)(figure->border_dimensions[0] / 2);
      axes_skels_copy[0] += b0;
      axes_skels_copy[2] -= b0 * 2;
      b0 = (size_t)(figure->border_dimensions[1] / 2);
      axes_skels_copy[1] += b0;
      axes_skels_copy[3] -= b0 * 2;
      // then copy these measurements into the axes
      figure->axes[axes_index].startX = figure->axes_skels_copy[index] = axes_skels_copy[0];
      figure->axes[axes_index].startY = figure->axes_skels_copy[index + 1] = axes_skels_copy[1];
      figure->axes[axes_index].width = figure->axes_skels_copy[index + 2] = axes_skels_copy[2];
      figure->axes[axes_index].height = figure->axes_skels_copy[index + 3] = axes_skels_copy[3];
    }
    break;
  }
  return true;
}

static bool set_borders(Figure *figure) {
  figure->border_dimensions[0] = (figure->border_percentage * figure->width) / (figure->cols + 1);
  long int x = (long int)figure->height - (figure->font_size * (figure->title == NULL ? 1 : 2));
  if (x < 0) {
    return false;
  }
  figure->border_dimensions[1] = (figure->border_percentage * x) / (figure->rows + 1);
  return true;
}

void raylib_init(Figure *figure) {
  RC_ASSERT(figure->sds == SCREEN_DIMENSION_STATE_DEFAULT, "show can only be called once\n");
  figure->sds = SCREEN_DIMENSION_STATE_CHANGED;
  SetConfigFlags(FLAG_WINDOW_RESIZABLE);
  SetConfigFlags(FLAG_MSAA_4X_HINT);
  SetTraceLogLevel(LOG_ERROR);
  char *window_title = figure->window_title ? figure->window_title : string_create_from_format(0, NULL, "%s.%d", RAY_WINDOW_TITLE, getpid());
  InitWindow(figure->width, figure->height, window_title);
  string_destroy(window_title);
  load_font(figure);
  update_fps(figure);
  ready_signal_set((ReadySignal *)figure->initialized);
}

static void update_fps(Figure *figure) {
  if (figure->fps < 0) {
    RC_ERROR("fps must be 0 or non negative value\n");
  }
  SetTargetFPS(figure->fps);
}

void raylib_init_loop() {
  // keyboard
  //  F -> Maximize
  if (IsKeyPressed(KEY_F)) {
    if (IsWindowMaximized()) {
      RestoreWindow();
    } else {
      MaximizeWindow();
    }
  }
  // Q -> exit
  SetExitKey(KEY_Q);
}

static void draw_current_time(Figure *figure) {
#define BUFFER_LEN 40
  struct timeval tv;
  struct tm *tm_info;
  char _buffer[BUFFER_LEN] = {0};
  Str buffer = string_create(BUFFER_LEN, _buffer);
  gettimeofday(&tv, NULL);
  tm_info = localtime(&tv.tv_sec);
  string_append(buffer, "%04d-%02d-%02d   %02d:%02d:%02d   %02ld", tm_info->tm_year + 1900, tm_info->tm_mon + 1, tm_info->tm_mday, tm_info->tm_hour,
                tm_info->tm_min, tm_info->tm_sec, tv.tv_usec / 10000);
  float x = align_text(FIGURE_FONT(figure), buffer, figure->width - 10, figure->font_size, figure->font_spacing, RC_ALIGNMENT_CENTER);
  DrawTextEx(FIGURE_FONT(figure), buffer, (Vector2){x, figure->height / 2}, figure->font_size, figure->font_spacing, figure->text_color);
}

static void draw_title(Figure *figure) {
  RC_ASSERT(figure->title != NULL);
  float x = align_text(FIGURE_FONT(figure), figure->title, figure->width, figure->font_size, figure->font_spacing, RC_ALIGNMENT_CENTER);
  DrawTextEx(FIGURE_FONT(figure), figure->title, (Vector2){x, 0}, figure->font_size, figure->font_spacing, figure->text_color);
}

static void draw_tooltip(Figure *figure) {
  /*
    fps  [index_under_mouse/total_length] [current_time-xindex[-1]] timeframe
    'axes_label_under_mouse' x_index_under_mouse y_index_under_mouse
  */
#define buf_size 128
  char _buffer[buf_size];
  char *buffer = string_create(buf_size, _buffer);
  size_t axes_under_mouse = get_axes_index_under_mouse(figure);
  Axes *axes = figure->axes + axes_under_mouse;
  string_append(buffer, "fps=%'d  mem:%'zu ", GetFPS(), cm_malloc_size());
  if (figure->has_dragger) { // the user has called `set_dragger` because
    // otherwise we do not have
    // xdata
    if (figure->axes_len != axes_under_mouse && axes->artist_len != 0) {
      int mousex_iloc = (int)(((float)(GetMouseX() - axes->startX) / (axes->width) * figure->dragger.vlen) + figure->dragger.start);
      string_append(buffer, "%zu [%d/%zu] ", figure->dragger.vlen, mousex_iloc, (figure->dragger._len - 1));
      string_append(buffer, "timeframe=%zu ", figure->dragger.timeframe);
      locator_tooltip_mouse_position(axes, buffer, GetMouseX(), GetMouseY());
    } else {
      string_append(buffer, "Xindex[-1]=");
      formatter_to_str(figure->dragger.locator.ftype, figure->dragger.locator.format, buffer, &figure->dragger.xdata[figure->dragger._len - 1]);
    }
  }
  float x = figure->border_dimensions[0] + AXES_FRAME_THICK +
    align_text(FIGURE_FONT(figure), buffer, (size_t)figure->width - (figure->border_dimensions[0] + AXES_FRAME_THICK), figure->font_size,
               figure->font_spacing, RC_ALIGNMENT_LEFT);
  DrawTextEx(FIGURE_FONT(figure), buffer, (Vector2){x, figure->height - figure->font_size}, figure->font_size, figure->font_spacing, figure->text_color);
}

static void load_font(Figure *figure) {
  if (string_len(figure->font_path)) {
    figure->font = LoadFont(figure->font_path);
  }
}

bool update_figure(Figure *figure) {
  int sd[] = {GetScreenWidth(), GetScreenHeight()};
  figure->sds =
    (sd[0] == figure->width && sd[1] == figure->height) ? SCREEN_DIMENSION_STATE_UNCHANGED : SCREEN_DIMENSION_STATE_CHANGED; // cannot be
  // SCREEN_DIMENSION_STATE_DEFAULT
  if (figure->force_update) {
    figure->force_update = false;
    figure->sds = SCREEN_DIMENSION_STATE_CHANGED;
    if (figure->has_dragger) {
      update_from_position(figure->dragger.start, figure);
    }
  }
  figure->width = sd[0];
  figure->height = sd[1];
  if (!set_borders(figure)) {
    return false;
  }
  if (!set_real_span_skel_map(figure)) {
    return false;
  }
  if (figure->title != NULL) {
    draw_title(figure);
  }
  if (IsKeyPressed(KEY_LEFT_SHIFT) || IsKeyPressed(KEY_RIGHT_SHIFT)) {
    figure->clear_screen = !figure->clear_screen;
  }
  if (figure->clear_screen) {
    figure->force_update = true;
    draw_current_time(figure);
    return true;
  }
  for (size_t i = 0; i < figure->axes_len; ++i) {
    if (!draw_axes(figure->axes + i)) {
      return false;
    }
  }
  if (figure->dragger.ulen > 0) {
    mouse_updates(figure);
  }
  if (figure->show_cursors == true) {
    figure_draw_cursors(figure);
  }
  draw_tooltip(figure);
  return true;
}

void update_xlim(Figure *figure) {
  size_t y = figure->dragger.start;
  double lmax, lmin;
  lmin = figure->dragger.xdata[y];
  lmin -= figure->dragger.timeframe / 2.f; // if visible_data=1 then
                                           // lmax-lmin=0
  lmax = figure->dragger.xdata[y + figure->dragger.vlen - 1];
  lmax += figure->dragger.timeframe / 2.f; // if visible_data=1 then
                                           // lmax-lmin=0
  figure->dragger.locator.limit = (Limit){.limit_max = lmax, .limit_min = lmin, .diff = lmax - lmin, .is_static = figure->dragger.locator.limit.is_static};
  for (size_t i = 0; i < figure->dragger.vlen; ++i, ++y)
    figure->dragger.xdata_shared[i] = (double)i / figure->dragger.vlen;
  figure->label_length = snprintf(NULL, 0, "%.5f", lmax);
}

Figure *create_figure(char *figskel, int *fig_size, char *window_title, Color background_color, float border_percentage, int fps, size_t font_size,
                      int font_spacing, char *font_path) {
  Fas fas=fas_parse(figskel);
  if (border_percentage < 0.f || border_percentage >= 1.f) {
    RC_ERROR("border_percentage is %f\n", border_percentage);
  }
  if (font_size == 0) {
    RC_ERROR("font_size is 0\n");
  }
  setlocale(LC_NUMERIC, "");
  size_t *CM_MALLOC(border_dimensions,sizeof(size_t) * 2);
  size_t *CM_MALLOC(axes_skels_dyn,sizeof(size_t) * fas.len * 4);
  memcpy(axes_skels_dyn, fas.skel, sizeof(fas.skel[0]) * fas.len * 4);
  Figure *CM_MALLOC(figure,sizeof(Figure));
  memset(figure, 0, sizeof(*figure));
  *figure = (Figure){
    .width = fig_size[0],
    .height = fig_size[1],
    .fps = fps,
    .font_size = font_size,
    .font_spacing = font_spacing,
    .zoomx_padding = 0.f,
    .vertical_limit_drag = 0.f,
    .border_percentage = border_percentage,
    // .dragger=set by calling set_dragger
    .title = NULL,
    .window_title = window_title ? string_create_from_format(0, NULL, "%s", window_title) : NULL,
    .axes_len = fas.len,
    .rows = fas.rows,
    .cols = fas.cols,
    .axes_skels = axes_skels_dyn,
    .axes_skels_copy = fas.skel,
    .label_length = 0,
    .axes = NULL,
    .border_dimensions = border_dimensions,
    .font = GetFontDefault(),
    .font_path = string_create_from_format(0, NULL, "%s", font_path),
    .initialized = ready_signal_create(),
    .mouse_drag = {0},
    .axes_frame_color = BLACK,
    .background_color = background_color,
    .text_color = BLACK,
    .sds = SCREEN_DIMENSION_STATE_DEFAULT,
    .show_cursors = true,
    .force_update = true,
    .has_dragger = false,
    .clear_screen = false,
    .show_xlabels = true,
    .show_ylabels = true,
  };
  create_axes(figure, fas.labels);
  CM_FREE(fas.labels);
  return figure;
}

void show(Figure *figure) {
  if (processId) {
    RC_ERROR("OpenGL context cannot be re-initialized correctly. only one "
             "show() per process\n");
  }
  processId = getpid();
  RC_ASSERT(figure != NULL);
  raylib_init(figure);
  axes_set_legend(figure);
  // trigger updates in the first loop
  figure->force_update = true;
  while (!WindowShouldClose()) {
    raylib_init_loop();
    BeginDrawing();
    ClearBackground(figure->background_color);
    if (!update_figure(figure) && (RAYCANDLE_DEBUG)) {
      RC_INFO("figure size is too small, some data will not be visible\n");
    }
    EndDrawing();
  }
  CloseWindow();
}

void figure_set_title(Figure *figure, char *title) {
  RC_ASSERT(title != NULL);
  if (figure->title != NULL) {
    string_destroy(figure->title);
  } else
    figure->force_update = true;
  figure->title = string_create_from_format(0, NULL, "%s", title);
}

void set_dragger(Figure *figure, size_t len, size_t timeframe, double *xdata, FormatterType ftype, char *format) {
  if (figure->has_dragger) {
    RC_ERROR("'%s' may only be called once before InitWindow\n", RC_ECHO(set_dragger));
  }
  Dragger dragger = {0};
  if ((dragger._len = len) == 0) {
    RC_ERROR("dragger.len is 0, no xdata?\n");
  }
  dragger.vlen = RC_INITIAL_VISIBLE_DATA < len ? RC_INITIAL_VISIBLE_DATA : len;
  RC_ASSERT(dragger.vlen <= RC_MAX_PLOTTABLE_LEN);
  dragger.ulen = RC_UPDATE_LEN > RC_INITIAL_VISIBLE_DATA ? RC_INITIAL_VISIBLE_DATA - RC_UPDATE_LEN : RC_UPDATE_LEN;
  if ((dragger.timeframe = timeframe) == 0) {
    RC_ERROR("spacing is  zero\n");
  }
  dragger.start = 0;
  if (format == NULL && ftype != FORMATTER_NULL_FORMATTER) {
    RC_ERROR("'%s' is NULL but formatter '%s' is not '%s'\n", RC_ECHO(format), RC_ECHO(FormatterType), RC_ECHO(FORMATTER_NULL_FORMATTER));
  }
  dragger.locator.ftype = ftype;
  dragger.locator.format = string_create_from_format(0, NULL, "%s", format ? format : "[NULL]");
  CM_MALLOC(dragger.xdata_shared,RC_MAX_PLOTTABLE_LEN * sizeof(double));
  for (size_t i = 0; i < figure->axes_len; ++i) {
    CM_MALLOC(figure->axes[i].xdata_buffer,RC_MAX_PLOTTABLE_LEN * sizeof(double));
  }
  RC_ASSERT((dragger.xdata = xdata) != NULL);
  figure->dragger = dragger;
  figure->has_dragger = true;
}

void update_timeframe(Figure *figure, size_t timeframe) {
  if (figure->has_dragger == false) {
    RC_ERROR("a dragger need to be set first; call `set_dragger`\n");
  }
  if (timeframe < 0.f) {
    RC_ERROR("timeframe must not be NaN or zero\n");
  }
  figure->dragger.timeframe = timeframe;
}

void figure_wait_initialized(Figure *figure) { ready_signal_wait((ReadySignal *)figure->initialized); }

void lib_free(void) { CM_FREE_ALL(); }
