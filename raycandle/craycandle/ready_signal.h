#ifndef CONDITION_H
#define CONDITION_H
#define CM_SILENT 1
#include "cust_malloc.h"
#undef CM_SILENT

#include <pthread.h>

typedef struct {
  int ready;
  pthread_mutex_t lock;
  pthread_cond_t cond;
} ReadySignal;

ReadySignal *ready_signal_create(void);
void ready_signal_destroy(ReadySignal *signal);
void ready_signal_set(ReadySignal *signal);
void ready_signal_wait(ReadySignal *signal);

#endif // CONDITION_H
