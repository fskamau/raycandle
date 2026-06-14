#ifndef _ARENA_H_
#define _ARENA_H_

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

typedef struct Arena Arena;
 struct Arena{
  size_t cap;
  size_t offset;
  char *buffer;
} ;

Arena *arena_create(size_t bytes);
void  *arena_alloc(Arena *arena, size_t bytes);
void   arena_free(Arena *arena);
void   arena_print(Arena *arena);
void  arena_reset(Arena* arena);

#ifdef ARENA_IMPLEMENTATION
#define ARENA_ALIGNMENT (sizeof(void*))

#define ARENA_ERROR(fmt, ...) do{                                       \
    fprintf(stderr,"%s:%d: [ARENA ERROR] " fmt ,__FILE__, __LINE__, ##__VA_ARGS__); \
    exit(1);}while (0)                                           

Arena *arena_create(size_t bytes) {
  if (bytes == 0) ARENA_ERROR("Cannot create arena with 0 bytes.\n");
  Arena *arena = (Arena *)malloc(sizeof(Arena) + bytes);
  if (arena == NULL) return NULL;
  arena->cap = bytes;
  arena->offset = 0;
  arena->buffer = (char *)(arena + 1);
  return arena;
}

void *arena_alloc(Arena *arena, size_t bytes) {
  if(!arena)ARENA_ERROR("null argument\n");
  if (bytes == 0) {
    ARENA_ERROR("arena_alloc: Requested 0 bytes.\n");
  }
  uintptr_t current_ptr = (uintptr_t)arena->buffer + arena->offset;
  uintptr_t aligned_ptr = (current_ptr + ARENA_ALIGNMENT - 1) & ~(ARENA_ALIGNMENT - 1);
  size_t aligned_offset = aligned_ptr - (uintptr_t)arena->buffer;
  if (aligned_offset>arena->cap || bytes > arena->cap||aligned_offset+bytes>arena->cap)goto arena_alloc_error;
  void *ptr = arena->buffer+aligned_offset;
  arena->offset = aligned_offset + bytes;
  if(!ptr)goto arena_alloc_error;
  return ptr;
 arena_alloc_error:{
    size_t rem=arena->cap-aligned_offset;
    ARENA_ERROR("arena_alloc: Out of memory: needs %'zub more; req (%'zu ~=%'.4fMB) cap (%'zub ~= %'.4fMB) free %'zu)\n",bytes-rem,bytes,(double)bytes/(1024*1024),arena->cap,(double)arena->cap/(1024*1024), rem);
  }
}

void arena_free(Arena *arena) {
  if(!arena)ARENA_ERROR("null argument\n");
  arena->offset=arena->cap=0;  
  free(arena);
}

void arena_print(Arena *arena) {
  if(!arena)ARENA_ERROR("null argument\n");
  double percent = ((double)arena->offset / (double)arena->cap) * 100.0;
  printf("Arena(cap=%'zu, used=%'zu free=%'zu, %%free=%.2f%%)\n", 
         arena->cap,
         arena->offset,
         arena->cap - arena->offset, 
         100.0 - percent);
}

void  arena_reset(Arena* arena)
{
  if(!arena)ARENA_ERROR("null argument\n");
  arena->offset=0;
}
#endif // ARENA_IMPLEMENTATION
#endif // _ARENA_H_
