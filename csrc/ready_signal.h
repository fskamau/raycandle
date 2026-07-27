#ifndef READY_SIGNAL_H
#define READY_SIGNAL_H
#include <stdbool.h>
#include <pthread.h>

typedef struct {
  pthread_mutex_t lock;
  pthread_cond_t cond;
  bool ready;
} ReadySignal;

typedef struct Arena Arena;
ReadySignal *ready_signal_create(Arena* arena);
void ready_signal_destroy(ReadySignal *signal);
void ready_signal_set(ReadySignal *signal);
void ready_signal_wait(ReadySignal *signal);
void ready_signal_clear(ReadySignal *signal);
#endif // READY_SIGNAL_H
