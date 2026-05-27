#ifndef STR_H
#define STR_H

#include <stddef.h>
#include <stdarg.h>
#include "arena.h"

typedef char* Str;

char* str_create(char* external_buf, size_t buf_size);
char* str_createf(const char* fmt, ...);
char* str_create_with_arena(Arena* arena, size_t buf_size);
void  str_free(char* s);
void  str_clear(char* s);
void str_append(char* s, const char* fmt, ...);
char  str_at(char* s, size_t index);
void  str_summarize(char* s);
char* str_cur(char *s); //returns a pointer to where \0 is
size_t str_rem(char* s); //return remaining capacity
#endif
