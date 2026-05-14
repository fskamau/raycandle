#include "str.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdint.h>
#include "utils.h"

typedef struct {
    size_t len;
    size_t capacity;
    uint8_t is_external;
} StrHeader;

typedef enum {
    STR_ERR_CORRUPT,
    STR_ERR_BOUNDS,
    STR_ERR_ALIGN,
    STR_ERR_NOMEM,
    STR_ERR_OVERFLOW,
    STR_ERR_NULL
} StrError;

static const char* str_strerror(StrError err) {
    switch (err) {
        case STR_ERR_CORRUPT:  return "Metadata corruption";
        case STR_ERR_BOUNDS:   return "Index out of bounds";
        case STR_ERR_ALIGN:    return "Memory alignment violation";
        case STR_ERR_NOMEM:    return "Insufficient memory / Buffer too small";
        case STR_ERR_OVERFLOW: return "Operation would exceed capacity";
        case STR_ERR_NULL:     return "Null pointer argument";
        default:               return "Unknown error";
    }
}

#define STR_ERROR(msg)do{\
    fprintf(stderr, "[STR] %s\n", msg);\
    exit(1);\
}while(0)

#define STR_FAIL(err_enum) STR_ERROR(str_strerror(err_enum))
#define STR_FAIL_MSG(msg)  STR_ERROR(msg)
   
#define STR_ALIGNMENT (sizeof(void*))
#define STR_HEADER_SIZE ((sizeof(StrHeader) + (STR_ALIGNMENT - 1)) & ~(STR_ALIGNMENT - 1))



static inline StrHeader* str_get_hdr(char* s) {
  if (!s) STR_FAIL(STR_ERR_NULL);
  StrHeader* hdr=(StrHeader*)(s - STR_HEADER_SIZE);
  if(hdr->capacity==0||
     (long long)hdr->len<0 ||
     (long long)hdr->capacity<0||
     hdr->len>hdr->capacity)
    STR_FAIL(STR_ERR_CORRUPT);   
  return hdr;
}


char* str_create(char* external_buf, size_t buf_size) {
    StrHeader* hdr = NULL;
    size_t total_needed;
    if (external_buf) {
        if (buf_size <= STR_HEADER_SIZE) {
            STR_FAIL_MSG("External buffer too small for header");
        }        
        if ((uintptr_t)external_buf % STR_ALIGNMENT != 0) {
            STR_FAIL(STR_ERR_ALIGN);
        }        
        hdr = (StrHeader*)external_buf;
        hdr->is_external = 1;
        hdr->capacity = buf_size - STR_HEADER_SIZE - 1;
    } else {
        total_needed = STR_HEADER_SIZE + buf_size + 1;        
        hdr = (StrHeader*)malloc(total_needed);
        if (!hdr) STR_FAIL(STR_ERR_NOMEM);        
        hdr->is_external = 0;
        hdr->capacity = buf_size;
    }
    hdr->len = 0;
    char* data = (char*)hdr + STR_HEADER_SIZE;
    data[0] = '\0';
    return data;
}

char* str_create_with_arena(Arena* arena, size_t buf_size)
{
  return str_create(arena_alloc_or_die(arena,STR_HEADER_SIZE+buf_size+1),buf_size+STR_HEADER_SIZE+1);
 }


void str_free(char* s) {
    StrHeader* hdr = str_get_hdr(s);
    int was_external = hdr->is_external;
    hdr->len = 0;
    hdr->capacity = 0;
    if (!was_external) {
        free(hdr);
    }
}

void str_clear(char* s) {
    StrHeader* hdr = str_get_hdr(s);
    hdr->len = 0;
    s[0] = '\0';
}

void str_append(char* s, const char* fmt, ...)
{
    StrHeader* hdr = str_get_hdr(s);
    va_list args;
    va_start(args, fmt);
    size_t available = hdr->capacity - hdr->len;
    int written = vsnprintf(s + hdr->len, available + 1, fmt, args);
    va_end(args);
    if (written < 0) {
        STR_FAIL_MSG("str_append: encoding error");
    }
    if ((size_t)written > available) {     
        hdr->len = hdr->capacity;
        STR_FAIL(STR_ERR_OVERFLOW);
    }
    hdr->len += (size_t)written;
}

char str_at(char* s, size_t index) {
    StrHeader* hdr = str_get_hdr(s);
    if (index >= hdr->len) {
        STR_FAIL(STR_ERR_BOUNDS);
    }
    return s[index];
}


void str_summarize(char* s){
  StrHeader* hdr=str_get_hdr(s);
  printf("Str(cap: %zu len: %zu external: %u)\n",hdr->capacity,hdr->len,hdr->is_external);
}
