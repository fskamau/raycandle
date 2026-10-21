#include "view.h"
#include <string.h>
#include <math.h>
#include "raycandle.h"

BarSeries* bar_series_create(size_t timeframe, size_t len, const double* epochs, Arena* arena, const char* title)
{
  ASSERT(arena && epochs && timeframe > 0 && len >= 2);
  BarSeries* bs = arena_calloc(arena, sizeof(*bs), 0);
  bs->timeframe = timeframe;
  bs->len = len;
  bs->epochs = arena_alloc(arena, sizeof(double)*len);
  memcpy(bs->epochs, epochs, sizeof(double)*len);
  bs->curr_epoch = (size_t)floor(epochs[len-1] / timeframe) * timeframe;
  bs->ur = UPDATE_REASON_INIT;
  if (title) bs->title = str_createf_with_arena(arena, "%s", title);
  return bs;
}

int bar_series_update_epoch(BarSeries* bs, double epoch)
{
  if (!bs || !isfinite(epoch) || epoch < bs->epochs[bs->len-1]) return -1;
  size_t anchor = (size_t)floor(epoch / bs->timeframe) * bs->timeframe;
  size_t last = bs->len - 1;
  if (anchor > bs->curr_epoch) {
    memmove(bs->epochs, bs->epochs+1, last*sizeof(double));
    bs->epochs[last] = (double)anchor;
    bs->curr_epoch = anchor;
    bs->ur = UPDATE_REASON_CANDLE;
    return 1;
  }
  bs->epochs[last] = epoch;
  bs->ur = UPDATE_REASON_TICK;
  return 0;
}
