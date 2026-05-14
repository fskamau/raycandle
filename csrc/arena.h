#ifndef _ARENA_H_
#define _ARENA_H_

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

typedef struct {
    size_t cap;
    size_t offset;
    char *buffer;
} Arena;

Arena *arena_create(size_t bytes);
void  *arena_alloc(Arena *arena, size_t bytes);
void  *arena_alloc_or_die(Arena *arena, size_t bytes);
void   arena_free(Arena *arena);
void   arena_print(Arena *arena);

#ifdef ARENA_IMPLEMENTATION

#define ARENA_ALIGNMENT 8
#define ARENA_ERROR(fmt, ...)do {\
        fprintf(stderr, "[ARENA]: " fmt "\n", ##__VA_ARGS__); \
        exit(1); \
    } while (0)

Arena *arena_create(size_t bytes) {
    if (bytes == 0) ARENA_ERROR("Cannot create arena with 0 bytes.");
    Arena *arena = (Arena *)malloc(sizeof(Arena) + bytes);
    if (arena == NULL) return NULL;
    arena->cap = bytes;
    arena->offset = 0;
    arena->buffer = (char *)(arena + 1);
    return arena;
}

void *arena_alloc(Arena *arena, size_t bytes) {
    if (bytes == 0) {
        ARENA_ERROR("arena_alloc: Requested 0 bytes.");
    }
    uintptr_t current_ptr = (uintptr_t)arena->buffer + arena->offset;
    uintptr_t aligned_ptr = (current_ptr + ARENA_ALIGNMENT - 1) & ~(ARENA_ALIGNMENT - 1);
    size_t aligned_offset = aligned_ptr - (uintptr_t)arena->buffer;
    if (aligned_offset + bytes > arena->cap) {
        return NULL;
    }
    void *ptr = &arena->buffer[aligned_offset];
    arena->offset = aligned_offset + bytes;
    return ptr;
}

void *arena_alloc_or_die(Arena *arena, size_t bytes) {
    void *ptr = arena_alloc(arena, bytes);
    if (ptr == NULL) {
        ARENA_ERROR("arena_alloc_or_die: Out of memory (cap: %zu, requested: %zu)", arena->cap, bytes);
    }
    return ptr;
}

void arena_free(Arena *arena) {
    free(arena);
}

void arena_print(Arena *arena) {
    double percent = ((double)arena->offset / (double)arena->cap) * 100.0;
    printf("Arena(cap=%zu, free=%zu, %%free=%.2f%%)\n", 
            arena->cap, 
            arena->cap - arena->offset, 
            100.0 - percent);
}

#endif // ARENA_IMPLEMENTATION
#endif // _ARENA_H_
