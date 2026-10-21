#ifndef RAYCANDLE_ARENA_H
#define RAYCANDLE_ARENA_H
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

typedef struct ArenaChunk ArenaChunk;
struct ArenaChunk
{
  ArenaChunk *next;
  size_t cap, offset;
  unsigned char *buffer;
};
typedef struct Arena
{
  ArenaChunk *first;
  ArenaChunk *current;
  size_t initial_capacity;
} Arena;

Arena *arena_create(size_t bytes);
void *arena_alloc(Arena *arena, size_t bytes);
void *arena_calloc(Arena *arena, size_t bytes, unsigned char fill);
void arena_free(Arena *arena);
void arena_print(Arena *arena);
void arena_reset(Arena *arena);

#ifdef ARENA_IMPLEMENTATION
#define ARENA_ALIGNMENT sizeof(void *)
static ArenaChunk *arena_new_chunk(size_t capacity)
{
  if (capacity > SIZE_MAX - sizeof(ArenaChunk))
    return NULL;
  ArenaChunk *chunk = (ArenaChunk *)malloc(sizeof(ArenaChunk) + capacity);
  if (!chunk)
    return NULL;
  chunk->next = NULL;
  chunk->cap = capacity;
  chunk->offset = 0;
  chunk->buffer = (unsigned char *)(chunk + 1);
  return chunk;
}
Arena *arena_create(size_t capacity)
{
  if (!capacity)
    return NULL;
  Arena *a = (Arena *)calloc(1, sizeof(Arena));
  if (!a)
    return NULL;
  a->initial_capacity = capacity;
  a->first = a->current = arena_new_chunk(capacity);
  if (!a->first)
  {
    free(a);
    return NULL;
  }
  return a;
}
void *arena_alloc(Arena *a, size_t size)
{
  if (!a || !size || size > SIZE_MAX - ARENA_ALIGNMENT)
  {
    fprintf(stderr, "raycandle: invalid arena allocation (%zu bytes)\n", size);
    abort();
  }
  ArenaChunk *c = a->current;
  uintptr_t at = (uintptr_t)c->buffer + c->offset;
  uintptr_t aligned = (at + ARENA_ALIGNMENT - 1) & ~(uintptr_t)(ARENA_ALIGNMENT - 1);
  size_t off = (size_t)(aligned - (uintptr_t)c->buffer);
  if (off > c->cap || size > c->cap - off)
  {
    size_t need = size + ARENA_ALIGNMENT;
    size_t cap = a->initial_capacity;
    if (cap < need)
      cap = need;
    c = arena_new_chunk(cap);
    if (!c)
    {
      fprintf(stderr, "raycandle: out of memory requesting %zu bytes\n", size);
      abort();
    }
    a->current->next = c;
    a->current = c;
    at = (uintptr_t)c->buffer;
    aligned = (at + ARENA_ALIGNMENT - 1) & ~(uintptr_t)(ARENA_ALIGNMENT - 1);
    off = (size_t)(aligned - at);
  }
  c->offset = off + size;
  return c->buffer + off;
}
void *arena_calloc(Arena *a, size_t size, unsigned char fill)
{
  void *p = arena_alloc(a, size);
  memset(p, fill, size);
  return p;
}
void arena_free(Arena *a)
{
  if (!a)
    return;
  ArenaChunk *c = a->first;
  while (c)
  {
    ArenaChunk *next = c->next;
    free(c);
    c = next;
  }
  free(a);
}
void arena_print(Arena *a)
{
  if (!a)
    return;
  size_t used = 0, cap = 0;
  for (ArenaChunk *c = a->first; c; c = c->next)
  {
    used += c->offset;
    cap += c->cap;
  }
  fprintf(stderr, "Arena(used=%zu, capacity=%zu bytes)\n", used, cap);
}
void arena_reset(Arena *a)
{
  if (!a)
    return;
  ArenaChunk *c = a->first->next;
  while (c)
  {
    ArenaChunk *next = c->next;
    free(c);
    c = next;
  }
  a->first->next = NULL;
  a->first->offset = 0;
  a->current = a->first;
}
#endif
#endif
