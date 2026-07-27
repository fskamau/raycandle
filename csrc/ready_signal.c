#include "ready_signal.h"

#include <stdio.h>
#include <stdlib.h>
#include "arena.h"
#include <pthread.h>

#define RS_ERROR(fmt, ...) do{                                          \
    fprintf(stderr,"%s:%d: [SIGNAL ERROR] " fmt ,__FILE__, __LINE__, ##__VA_ARGS__); \
    exit(1);}while (0)                                           


ReadySignal *ready_signal_create(Arena* arena)
{
  ReadySignal * signal=arena_alloc(arena, sizeof(ReadySignal));
  signal->ready = 0;
  if (pthread_mutex_init(&signal->lock, NULL) != 0) {
    RS_ERROR("cannot init mutex\n");
  }
  if (pthread_cond_init(&signal->cond, NULL) != 0) {
    pthread_mutex_destroy(&signal->lock);
    RS_ERROR("cannot init cond\n");
  }
  return signal;
}

void ready_signal_set(ReadySignal *signal) {
  pthread_mutex_lock(&signal->lock);
  signal->ready = 1;
  pthread_cond_broadcast(&signal->cond);
  pthread_mutex_unlock(&signal->lock);
}

void ready_signal_wait(ReadySignal *signal) {
  pthread_mutex_lock(&signal->lock);
  while (!signal->ready) {
    pthread_cond_wait(&signal->cond, &signal->lock);
  }
  pthread_mutex_unlock(&signal->lock);
}

void ready_signal_destroy(ReadySignal *signal) {
  if (!signal)
    return;
  pthread_mutex_destroy(&signal->lock);
  pthread_cond_destroy(&signal->cond);
}

void ready_signal_clear(ReadySignal *signal)
{
  pthread_mutex_lock(&signal->lock);
  signal->ready = 0;
  pthread_mutex_unlock(&signal->lock);
}
