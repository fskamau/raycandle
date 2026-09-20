#include <stddef.h>
#include "utils.h"

typedef struct {
  float *o,*h,*l,*c;
} Candles;


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
  Candles candles;
  UpdateReason ur;
  double* epochs;
  char* title;
};


BarSeries* bar_series_create(size_t timeframe, size_t len, Candles candles, double* epochs, Arena *arena, const char *title);
void bar_series_update(BarSeries *bs, double epoch, float price);
