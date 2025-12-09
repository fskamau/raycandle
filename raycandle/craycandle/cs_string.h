/*
 *allocated string and returns a printable adress to start of the string
 *if mem is NULL, string will be dynamic otherwise, it maybe a stack one
 *Please NOTE:
 *  1.The string structure is stored inside the string use string_len to get
 * the length #define LEN 16 char mystr[LEN]={0}; char*
 * str=string_create(LEN,mystr); string_len(str)=LEN-sizeof(_String)-1(due to
 * \0)
 *  2. A dynamic string len will not change. i.e
 *     char* str=string_create(LEN,NULL);
 *     string_len(str)=LEN
 *  3.string_create_formatted may take len=0 and mem=NULL to allocate the
 * formatted string in memory
 */

#ifndef __CS__ // custom_string
#define __CS__

typedef char *Str;

Str string_create(unsigned int len, void *mem);
Str string_create_from_format(unsigned int len, void *mem, const char *format,
                              ...); // return a formatted string
void string_append(Str str, const char *format, ...);
char *string_pat(Str str, unsigned int index);
char string_at(Str str, unsigned int index);
unsigned int string_count(Str str, char c);
unsigned int
string_count_consecutive(Str str, char c,
                         unsigned int start); // count consecutive occurrences
                                              // of c in str starting from start
Str string_split(Str str, unsigned int start, unsigned int len);
void string_summarize(Str str); // print a summary of the string
void string_destroy(Str str);   // frees a string if its a dynamic string
unsigned int string_len(Str str);
unsigned int string_get_remaining(Str str);
char *string_get_current_point(Str str);
void string_clear(Str str);

#ifdef CS_IMPLEMENTATION

#include <stdarg.h>
#include <stddef.h>
#include <stdio.h>

#define CM_IMPLEMENTATION
#include "cust_malloc.h"

#define STRING_ERROR(format, ...)                                              \
  do {                                                                         \
    fprintf(stderr, "%s:%d: %s [string]: " format, __FILE__, __LINE__,         \
            __func__, ##__VA_ARGS__);                                          \
    exit(EXIT_FAILURE);                                                        \
  } while (0)

#define STRING_CHECK_CONDITION(cond, msg)                                      \
  do {                                                                         \
    if (!(cond))                                                               \
      STRING_ERROR("condition %s failed%s%s\n", #cond, (msg) ? ": " : "",      \
                   (msg) ? msg : "");                                          \
  } while (0)

#define STRING_ALLOC(name, size) CM_MALLOC(name ,(size))
#define STRING_GET_START(string_ptr)                                           \
  (((char *)(string_ptr)) + (sizeof(_String)))
#define STRING_GET_CURRENT(string_ptr)                                         \
  (STRING_GET_START((string_ptr)) + strlen(STRING_GET_START((string_ptr))))

typedef struct {
  unsigned int capacity;
  unsigned char stack;
} _String;

static inline _String *string_get(Str str);
static inline void _string_append(Str str, const char *format, va_list args,
                                  unsigned int total_length);

static inline _String *string_get(Str str) {
  STRING_CHECK_CONDITION(str != NULL, "string may be corrupted\n");
  _String *s = (_String *)((char *)str - sizeof(_String));
  STRING_CHECK_CONDITION((long)strlen(str) <= ((long)s->capacity),
                         "string may be corrupted\n");
  return s;
}

static inline void _string_append(Str str, const char *format, va_list args,
                                  unsigned int total_length) {
  _String *string = string_get(str);
  unsigned int newl, sl = strlen(str);
  if (total_length == 0) {
    va_list args2;
    va_copy(args2, args);
    if ((newl = vsnprintf(NULL, 0, format, args2)) + sl > string->capacity) {
      newl -= string->capacity - sl;
      STRING_ERROR("need more space to fit %d item%s : %s\n", newl,
                   newl == 1 ? "" : "s", format);
    }
    va_end(args2);
  } else
    STRING_CHECK_CONDITION(total_length <= string->capacity, "");
  STRING_CHECK_CONDITION(sl +
                                 vsnprintf(STRING_GET_CURRENT(string),
                                           string->capacity - sl + 1, format,
                                           args) +
                                 1 <=
                             string->capacity + 1,
                         "string full\n");
}

Str string_create(unsigned int len, void *mem) {
  if ((int)len < 0)
    STRING_ERROR("len <= 0\n");
  char *str;
  if (mem) {
    STRING_CHECK_CONDITION(len + 1 >= sizeof(_String),
                           "stack string too small to fit _String struct "
                           "size + null terminator");
    str = mem;
    len -= (1 + sizeof(_String));
  } else {
    STRING_ALLOC(str, len + sizeof(_String) + 1);
  }
  *((_String *)str) = (_String){.capacity = len, .stack = mem ? 1 : 0};
  memset(STRING_GET_START(str), 0, len + 1);
  return STRING_GET_START(str);
}

Str string_create_from_format(unsigned int len, void *mem, const char *format,
                              ...) {
  char *str;
  va_list args;
  va_start(args, format);
  unsigned char has_total_len = 0;
  if (!mem && len) {
    STRING_ERROR("len is not 0 but mem is NULL\n");
  }
  if (len == 0 && !mem) {
    va_list args2;
    va_copy(args2, args);
    len = vsnprintf(NULL, 0, format, args2);
    va_end(args2);
    has_total_len = 1;
  }
  str = string_create(len, mem);
  _string_append(str, format, args, has_total_len ? len : 0);
  return str;
}

void string_append(Str str, const char *format, ...) {
  va_list args;
  va_start(args, format);
  _string_append(str, format, args, 0);
  va_end(args);
}

char *string_pat(Str str, unsigned int index) {
  _String *string = string_get(str);
  if (string->capacity == 0 || string->capacity < index)
    STRING_ERROR("Index error len is %d; requested %d\n", string->capacity,
                 index);
  return &str[index];
}

char string_at(Str str, unsigned int index) { return *string_pat(str, index); }

unsigned int string_count(Str str, char c) {
  unsigned int count = 0;
  while ((str = strchr(str, c)) != NULL) {
    count += 1;
    str += 1;
  }
  return count;
}

unsigned int string_count_consecutive(Str str, char c, unsigned int start) {
  STRING_CHECK_CONDITION(start < string_len(str), "corrupted string");
  unsigned int count = 0;
  for (size_t i = start; i < string_len(str); ++i) {
    if (string_at(str, i) != c)
      break;
    count++;
  }
  return count;
}

Str string_split(Str str, unsigned int start, unsigned int len) {
  STRING_CHECK_CONDITION(strlen(str) > start, "");
  STRING_CHECK_CONDITION(strlen(str) >= start + len, "");
  Str str2 = string_create(len, NULL);
  memcpy(str2, string_pat(str, start), len);
  return str2;
}

void string_summarize(Str str) {
  _String *string = string_get(str);
  printf("String(capacity=%'d len=%'zu, inheap=%d)\n", string->capacity,
         strlen(str), !string->stack);
}

void string_destroy(Str str) {
  _String *string = string_get(str);
  string->capacity = 0;
  if (string->stack)
    return;
  CM_FREE(string);
}

unsigned int string_len(Str str) {
  string_get(str);
  return strlen(str);
}

unsigned int string_get_remaining(Str str) {
  _String *string = string_get(str);
  return string->capacity - strlen(str);
}

char *string_get_current_point(Str str) {
  return STRING_GET_CURRENT(string_get(str));
}

void string_clear(Str str) { str[0] = '\0'; }

#endif // CS_IMPLEMENTATION

#undef STRING_GET_START
#undef STRING_GET_CURRENT
#undef STRING_ERROR
#undef STRING_CHECK_CONDITION

#endif //__CS__
