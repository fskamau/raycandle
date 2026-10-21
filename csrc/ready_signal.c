#include "ready_signal.h"
#include "arena.h"

#include <stdlib.h>
#include <stdbool.h>

#ifdef _WIN32
    #include <windows.h>
#else
    #include <pthread.h>
#endif

struct ReadySignal {
#ifdef _WIN32
    SRWLOCK lock;
    CONDITION_VARIABLE cond;
#else
    pthread_mutex_t lock;
    pthread_cond_t cond;
#endif
    bool ready;
};

ReadySignal* ready_signal_create(Arena* arena) {
    if (!arena) return NULL;

    ReadySignal* s = arena_calloc(arena, sizeof(*s), 0);
    if (!s) return NULL;

#ifdef _WIN32
    InitializeSRWLock(&s->lock);
    InitializeConditionVariable(&s->cond);
#else
    if (pthread_mutex_init(&s->lock, NULL) != 0)
        abort();

    if (pthread_cond_init(&s->cond, NULL) != 0) {
        pthread_mutex_destroy(&s->lock);
        abort();
    }
#endif

    s->ready = false;
    return s;
}

void ready_signal_set(ReadySignal* s) {
    if (!s) return;

#ifdef _WIN32
    AcquireSRWLockExclusive(&s->lock);
    s->ready = true;
    WakeAllConditionVariable(&s->cond);
    ReleaseSRWLockExclusive(&s->lock);
#else
    pthread_mutex_lock(&s->lock);
    s->ready = true;
    pthread_cond_broadcast(&s->cond);
    pthread_mutex_unlock(&s->lock);
#endif
}

void ready_signal_wait(ReadySignal* s) {
    if (!s) return;

#ifdef _WIN32
    AcquireSRWLockExclusive(&s->lock);

    while (!s->ready) {
        if (!SleepConditionVariableSRW(
                &s->cond, &s->lock, INFINITE, 0)) {
            ReleaseSRWLockExclusive(&s->lock);
            abort();
        }
    }

    ReleaseSRWLockExclusive(&s->lock);
#else
    pthread_mutex_lock(&s->lock);

    while (!s->ready) {
        if (pthread_cond_wait(&s->cond, &s->lock) != 0) {
            pthread_mutex_unlock(&s->lock);
            abort();
        }
    }

    pthread_mutex_unlock(&s->lock);
#endif
}

void ready_signal_clear(ReadySignal* s) {
    if (!s) return;

#ifdef _WIN32
    AcquireSRWLockExclusive(&s->lock);
    s->ready = false;
    ReleaseSRWLockExclusive(&s->lock);
#else
    pthread_mutex_lock(&s->lock);
    s->ready = false;
    pthread_mutex_unlock(&s->lock);
#endif
}

void ready_signal_destroy(ReadySignal* s) {
    if (!s) return;

#ifndef _WIN32
    pthread_cond_destroy(&s->cond);
    pthread_mutex_destroy(&s->lock);
#endif

    // Memory belongs to Arena; do not free(s).
}

void ready_signal_lock(ReadySignal* s) {
    if (!s) return;

#ifdef _WIN32
    AcquireSRWLockExclusive(&s->lock);
#else
    pthread_mutex_lock(&s->lock);
#endif
}

void ready_signal_unlock(ReadySignal* s) {
    if (!s) return;

#ifdef _WIN32
    ReleaseSRWLockExclusive(&s->lock);
#else
    pthread_mutex_unlock(&s->lock);
#endif
}
