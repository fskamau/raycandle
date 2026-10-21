#ifndef READY_SIGNAL_H
#define READY_SIGNAL_H

#include <stdbool.h>

typedef struct Arena Arena;
typedef struct ReadySignal ReadySignal;

ReadySignal* ready_signal_create(Arena* arena);
void ready_signal_destroy(ReadySignal* signal);
void ready_signal_set(ReadySignal* signal);
void ready_signal_wait(ReadySignal* signal);
void ready_signal_clear(ReadySignal* signal);
void ready_signal_lock(ReadySignal* signal);
void ready_signal_unlock(ReadySignal* signal);

#endif
