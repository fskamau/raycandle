/*
 enum for helping perfomance by avoiding too much
 recalculation of limits everytime and scaling artist buffers
 */
typedef enum{
  DATA_STATE_APPENDED_1_END; //1 data item has been added to the data end. No other change to data
  DATA_STATE_CHANGED_1_END;//last data point has changed. No other change to data
  DATA_STATE_CHANGED;//more  than one data points have been affected. we will have to recalc  all limits
}DataState;
