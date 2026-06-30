#ifndef CONDITION_H
#define CONDITION_H

#include <pthread.h>

typedef struct {
  int ready;
  pthread_mutex_t lock;
  pthread_cond_t cond;
} ReadySignal;

typedef struct Arena Arena;
ReadySignal *ready_signal_create(Arena* arena);
void ready_signal_destroy(ReadySignal *signal);
void ready_signal_set(ReadySignal *signal);
void ready_signal_wait(ReadySignal *signal);
void ready_signal_clear(ReadySignal *signal);
#endif // CONDITION_H
