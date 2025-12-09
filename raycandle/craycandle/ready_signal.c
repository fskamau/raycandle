#include "ready_signal.h"

#include <stdio.h>
#include <stdlib.h>

#define RS_ERROR(format, ...)                                                  \
  do {                                                                         \
    fprintf(stderr, "[ready_signal] ERROR: " format "\n", ##__VA_ARGS__);      \
    exit(EXIT_FAILURE);                                                        \
  } while (0)

ReadySignal *ready_signal_create(void) {
  CM_MALLOC(ReadySignal * signal, sizeof(ReadySignal));
  signal->ready = 0;
  if (pthread_mutex_init(&signal->lock, NULL) != 0) {
    CM_FREE(signal);
    RS_ERROR("cannot init mutex\n");
  }
  if (pthread_cond_init(&signal->cond, NULL) != 0) {
    pthread_mutex_destroy(&signal->lock);
    CM_FREE(signal);
    RS_ERROR("cannot init cond\n");
  }
  return signal;
}

void ready_signal_set(ReadySignal *signal) {
  pthread_mutex_lock(&signal->lock);
  signal->ready = 1;
  pthread_cond_signal(&signal->cond);
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
  CM_FREE(signal);
}
