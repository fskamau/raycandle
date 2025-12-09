#include "artist.h"

#include <assert.h>
#include <math.h>
#include <string.h>

#include "axes.h"
#include "raycandle.h"
#include "utils.h"

typedef struct {
  double *d0, *d1;
  double *p0, *p1, *p2, *p3;
  uint8_t *color_indexes;
  double width;
} CandleData;

// init
static void artist_init(Artist *artist, void *config);
static void artist_line_init(Artist *artist, void *config);
static void artist_candle_init(Artist *artist, void *config);
// update
static void artist_line_update_data_buffer(Artist *artist, LimitChanged lim);
static void artist_candle_update_data_buffer(Artist *artist, LimitChanged lim);
// draw
static void artist_line_plot(Artist *artist);
static void artist_candle_plot(Artist *artist);
static void artist_line_draw_icon(Artist *artist, Vector2 startPos);
static void artist_candle_draw_icon(Artist *artist, Vector2 startPos);

static void artist_init(Artist *artist, void *config) {
  switch (artist->artist_type) {
  case ARTIST_TYPE_LINE:
    artist_line_init(artist, config);
    break;
  case ARTIST_TYPE_CANDLE:
    RC_ASSERT(artist->gdata.cols == 4);
    artist_candle_init(artist, config);
    break;
  default:
    RC_ERROR("artist type %d has not been implemented on '%s'\n",
             artist->artist_type, __func__);
  }
}

static void artist_line_init(Artist *artist, void *config) {
  LineData *line_data = (LineData *)config;
  RC_ASSERT(line_data && !line_data->data);
  CM_MALLOC(LineData *line_data_ , (sizeof(LineData)));
  switch (line_data->line_type) {
  case LINE_TYPE_S_LINE:    
    CM_MALLOC(line_data_->data,RC_MAX_PLOTTABLE_LEN * sizeof(*line_data_->data));
    break;
  case LINE_TYPE_H_LINE:
  case LINE_TYPE_V_LINE: {
    RC_ASSERT(artist->gdata.ydata && artist->gdata.ydata[0] >= 0 &&
              artist->gdata.ydata[0] <= 1);
    CM_MALLOC(line_data_->data,sizeof(*line_data->data) * 2);
    line_data_->data[0] = artist->gdata.ydata[0];
    artist->gdata.ydata = NULL;
    artist->ylim_consider = false; // these two are not used in finding ylims
    break;
  }
  default:
    RC_ERROR("unknown LINE_TYPE %d\n", line_data->line_type);
    break;
  }
  line_data_->line_type = line_data->line_type;
  artist->data = line_data_;
  Color *CM_MALLOC(color,sizeof(Color));
  if (!artist->color) {
    *color = axes_get_next_tableau_t10_color(artist->parent);
  } else {
    memcpy(color, artist->color, sizeof(Color));
  }
  artist->color = color;
}

static void artist_candle_init(Artist *artist, void *config) {
  /*


     d0   d1
  -------------------
        | <------- p0
        |
      ----- <----- p1
      |   |
      |   |
      |   |
      ----- <----- p2
        |
        |<---------p3

  */
  (void)config;
  size_t vdata;
  CandleData *candledata;
  Color *color;
  vdata = RC_MAX_PLOTTABLE_LEN;
  CM_MALLOC(candledata,sizeof(CandleData));
  candledata->d0 = artist->parent->xdata_buffer;
  CM_MALLOC(candledata->d1,sizeof(double) * vdata * 5);
  candledata->p0 = candledata->d1 + vdata * 1;
  candledata->p1 = candledata->d1 + vdata * 2;
  candledata->p2 = candledata->d1 + vdata * 3;
  candledata->p3 = candledata->d1 + vdata * 4;
  CM_MALLOC(candledata->color_indexes ,sizeof(*candledata->color_indexes) * vdata);
  artist->data = (void *)candledata;
  CM_MALLOC(color,sizeof(Color) * 2);
  if (artist->color == NULL) {
    color[0] = axes_get_next_tableau_t10_color(artist->parent);
    color[1] = axes_get_next_tableau_t10_color(artist->parent);
  } else {
    memcpy(color, artist->color, sizeof(Color) * 2);
  }
  artist->color = color;
}

static void artist_line_update_data_buffer(Artist *artist, LimitChanged lim) {
  if (lim == LIMIT_CHANGED_XLIM) {
    return;
  } // user changing xlim does no change ylim
  double *ydata = ((LineData *)artist->data)->data;
  switch (((LineData *)artist->data)->line_type) {
  case LINE_TYPE_S_LINE: {
    double *pixel_data = artist->gdata.ydata;
    size_t yindex = artist->parent->parent->dragger.start;
    for (size_t i = 0; i < artist->parent->parent->dragger.vlen;
         ++i, ++yindex) {
      /* ydata[i]=RC_DATA_IN_LIMIT(pixel_data[yindex],artist->parent->ylocator.limit)?RC_DATA_Y_2_PIXEL(pixel_data[yindex],
       * artist->parent):NAN; */
      ydata[i] = RC_DATA_Y_2_PIXEL(pixel_data[yindex], artist->parent);
    }
    return;
  }
  case LINE_TYPE_H_LINE: {
    ydata[1] = RC_DATA_Y_2_PIXEL(ydata[0], artist->parent);
    return;
  }
  case LINE_TYPE_V_LINE: {
    /* ydata[1]=RC_DATA_X_2_PIXEL(ydata[0],artist->parent); */
    return;
  }
  default:
    RC_ERROR("unknown linetype \n");
  }
}

static void artist_candle_update_data_buffer(Artist *artist, LimitChanged lim) {
  size_t vdata = artist->parent->parent->dragger.vlen;
  CandleData *candledata = (CandleData *)artist->data;
  candledata->width =
      ((double)artist->parent->width / artist->parent->parent->dragger.vlen) /
      2.f;
  size_t ldata = artist->parent->parent->dragger._len;
  size_t yindex = artist->parent->parent->dragger.start;
  bool ogtc;
  double *cdata = artist->gdata.ydata;
  for (size_t i = 0; i < vdata; ++i, ++yindex) {
    /* if( */
    /*   !RC_DATA_IN_LIMIT(cdata[yindex],artist->parent->ylocator.limit)|| */
    /*   !RC_DATA_IN_LIMIT(cdata[ldata+yindex],artist->parent->ylocator.limit)||
     */
    /*   !RC_DATA_IN_LIMIT(cdata[ldata*2+yindex],artist->parent->ylocator.limit)||
     */
    /*   !RC_DATA_IN_LIMIT(cdata[ldata*3+yindex],artist->parent->ylocator.limit)
     */
    /* ){ */
    /*   candledata->p0[i]=candledata->p1[i]=candledata->p2[i]=candledata->p3[i]=NAN;
     */
    /*   continue; */
    /* } */
    if (lim == LIMIT_CHANGED_XLIM || lim == LIMIT_CHANGED_ALL_LIM) {
      candledata->d1[i] = candledata->d0[i] + (candledata->width / 2.f);
    }
    if (lim == LIMIT_CHANGED_YLIM || lim == LIMIT_CHANGED_ALL_LIM) {
      ogtc = cdata[yindex] > cdata[ldata * 3 + yindex];
      candledata->p0[i] =
          RC_DATA_Y_2_PIXEL(cdata[ldata + yindex], artist->parent);
      candledata->p1[i] = RC_DATA_Y_2_PIXEL(
          ogtc ? cdata[yindex] : cdata[ldata * 3 + yindex], artist->parent);
      candledata->p2[i] = RC_DATA_Y_2_PIXEL(
          !ogtc ? cdata[yindex] : cdata[ldata * 3 + yindex], artist->parent);
      candledata->p3[i] =
          RC_DATA_Y_2_PIXEL(cdata[ldata * 2 + yindex], artist->parent);
      candledata->color_indexes[i] = (uint8_t)!ogtc;
    }
  }
}

static void artist_line_plot(Artist *artist) {
  LineData line_data = *(LineData *)artist->data;
  double *xdata = artist->parent->xdata_buffer;
  switch (line_data.line_type) {
  case LINE_TYPE_S_LINE: {
    size_t visible_data = artist->parent->parent->dragger.vlen;
    Vector2 spline_buffer[visible_data]; // TODO: FIX THIS
    for (size_t i = 0; i < visible_data; ++i) {
      spline_buffer[i] = (Vector2){xdata[i], line_data.data[i]};
    }
    DrawSplineLinear(spline_buffer, artist->parent->parent->dragger.vlen,
                     artist->thickness, *artist->color);
    return;
  }
  case LINE_TYPE_H_LINE:
    if (isnan(line_data.data[1]))
      goto label_non_segmented_line_having_nan;
    DrawLineEx((Vector2){artist->parent->startX, line_data.data[1]},
               (Vector2){artist->parent->startX + artist->parent->width,
                         line_data.data[1]},
               artist->thickness, *artist->color);
    return;
  case LINE_TYPE_V_LINE:
    if (isnan(line_data.data[1]))
      goto label_non_segmented_line_having_nan;
    DrawLineEx((Vector2){line_data.data[1], artist->parent->startY},
               (Vector2){line_data.data[1],
                         artist->parent->startY + artist->parent->height},
               artist->thickness, *artist->color);
    return;
  default:
    RC_ERROR("uknown line_type\n");
  }
label_non_segmented_line_having_nan: {
  Limit limit = artist->parent->ylocator.limit;
  RC_ERROR("non-segmented line unreachable condition .plotting exactly 1 "
           "point %f "
           "on axes with limits (min=%f, max=%f)\n",
           line_data.data[0], limit.limit_min, limit.limit_max);
}
}

static void artist_candle_plot(Artist *artist) {
  RC_ASSERT(artist->artist_type == ARTIST_TYPE_CANDLE);
  CandleData *candledata = (CandleData *)artist->data;
  int width = candledata->width;
  for (size_t cindex = 0; cindex < artist->parent->parent->dragger.vlen;
       ++cindex) {
    Color color = artist->color[candledata->color_indexes[cindex]];
    DrawRectangleLinesEx(
        (Rectangle){candledata->d0[cindex], candledata->p1[cindex], width,
                    fmax(candledata->p2[cindex] - candledata->p1[cindex], 1.f)},
        artist->thickness, color);
    /* DrawRectangleV((Vector2){candledata->d0[cindex],candledata->p1[cindex]},(Vector2){width,RC_MAX(candledata->p2[cindex]-candledata->p1[cindex],1.f)},color);
     */
    DrawLineEx((Vector2){candledata->d1[cindex], candledata->p0[cindex]},
               (Vector2){candledata->d1[cindex], candledata->p1[cindex]},
               artist->thickness, color);
    DrawLineEx((Vector2){candledata->d1[cindex], candledata->p2[cindex]},
               (Vector2){candledata->d1[cindex], candledata->p3[cindex]},
               artist->thickness, color);
  }
}

static void artist_line_draw_icon(Artist *artist, Vector2 startPos) {
  startPos.y +=
      artist->parent->parent->font_size / 2.f; // draw a line middle as icon
  DrawLineEx(startPos, (Vector2){startPos.x + RC_LEGEND_ICON_WIDTH, startPos.y},
             1, *artist->color);
}

static void artist_candle_draw_icon(Artist *artist, Vector2 startPos) {
  int y = artist->parent->parent->font_size / 3; // draw a half rect as icon
  DrawRectangleLines(startPos.x, startPos.y + y, RC_LEGEND_ICON_WIDTH,
                     artist->parent->parent->font_size - y * 2,
                     artist->color[0]);
}

inline Artist *get_artist(Axes *axes, size_t index) {
  if ((long int)index < 0 || index + 1 > axes->artist_len)
    RC_ERROR("requested artist index %zu but axes only has %zu items\n", index,
             axes->artist_len);
  Artist *artist = axes->artist;
  for (size_t i = 0; i < index; ++i)
    artist = artist->next;
  return artist;
}

void artist_update_data_buffer(Artist *artist, LimitChanged lim) {
  switch (artist->artist_type) {
  case ARTIST_TYPE_LINE:
    artist_line_update_data_buffer(artist, lim);
    break;
  case ARTIST_TYPE_CANDLE:
    artist_candle_update_data_buffer(artist, lim);
    break;
  default:
    RC_ERROR("artist type %d has not been implemented on '%s'\n",
             artist->artist_type, __func__);
  }
}

void draw_artist(Artist *artist) {
  switch (artist->artist_type) {
  case ARTIST_TYPE_LINE:
    return artist_line_plot(artist);
  case ARTIST_TYPE_CANDLE:
    return artist_candle_plot(artist);
  default:
    RC_ERROR("imlpement %d\n for draw_artist", artist->artist_type);
  }
}

void artist_draw_icon(Artist *artist, Vector2 startPos) {
  switch (artist->artist_type) {
  case ARTIST_TYPE_CANDLE:
    artist_candle_draw_icon(artist, startPos);
    break;
  case ARTIST_TYPE_LINE:
    artist_line_draw_icon(artist, startPos);
    break;
  default:
    RC_ERROR("imlpement %d\n for draw_icon", artist->artist_type);
  }
}

Artist *create_artist(Axes *axes, ArtistType artist_type, Gdata gdata,
                      double ydata_minmax[2], float thickness, Color *color,
                      void *config) {
  Artist *CM_MALLOC(artist,sizeof(Artist));
  *artist = (Artist){
      .artist_type = artist_type,
      .gdata = gdata,
      .parent = axes,
      .thickness = thickness,
      .next = NULL,
      .color = color,
      .ylim_consider = true,
  };
  if (gdata.label != NULL) {
    RC_ASSERT(gdata.label[0] != '\0');
  }
  if (axes->artist_len == 0) {
    RC_ASSERT(axes->artist == NULL);
    axes->artist = artist;
  } else {
    get_artist(axes, axes->artist_len - 1)->next = artist;
  }
  if (isnan(ydata_minmax[0]) || isnan(ydata_minmax[1])) {
    RC_ERROR("create_artist failed to create Artist(name='%s'). Data limits "
             "cannot "
             "contain NaN\n",
             (artist->gdata.label == NULL ? "Nameless" : artist->gdata.label));
  }
  if (!axes->ylocator.limit.is_static) {
    if (axes->artist_len == 0) {
      axes->ylocator.limit = (Limit){.is_static = false,
                                     .limit_min = ydata_minmax[0],
                                     .limit_max = ydata_minmax[1],
                                     .diff = ydata_minmax[1] - ydata_minmax[0]};
    } else {
      axes->ylocator.limit.limit_min =
          fmin(axes->ylocator.limit.limit_min, ydata_minmax[0]);
      axes->ylocator.limit.limit_max =
          fmax(axes->ylocator.limit.limit_max, ydata_minmax[1]);
      axes->ylocator.limit.diff =
          axes->ylocator.limit.limit_max - axes->ylocator.limit.limit_min;
    }
  }
  axes->artist_len += 1;
  artist_init(artist, config);
  return artist;
}
