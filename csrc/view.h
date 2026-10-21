#include <stddef.h>
#include "utils.h"
#include "raycandle.h"


typedef enum {
  UPDATE_REASON_INIT=0, //initialization, calc everything
  UPDATE_REASON_TICK, // update last candle only
  UPDATE_REASON_CANDLE, // candle shifted, cacl last candle only
}UpdateReason;


typedef enum {
  STUDY_TYPE_LINE,
  STUDY_TYPE_CANDLE,
}StudyType;


typedef struct BarSeries BarSeries;


struct BarSeries  {
  size_t curr_epoch,timeframe, len;
  /* Candles live in their Artist, not in BarSeries. */
  UpdateReason ur;
  double* epochs;
  char* title;
};



BarSeries* bar_series_create(size_t timeframe, size_t len, const double* epochs, Arena *arena, const char *title);
/* Return 1 on candle rollover, 0 for same timeframe, -1 for stale tick. */
int bar_series_update_epoch(BarSeries* bs, double epoch);
