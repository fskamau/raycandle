#ifndef LOG_H
#define LOG_H

/**
 * @file log.h
 * @brief Simple logging system with Info/Warn/Error levels.
 * 
 * Features:
 * - Correct caller file:line:function using macro wrappers
 * - Optional debug formatting
 * - Optional mutex / thread safety
 * - Optional exit() on error
 * 
 * Configuration macros (define *before* including log.h):
 *   LOG_DEBUG            (0/1) include file/line/func
 *   LOG_USE_MUTEX        (0/1) enable pthread mutex
 *   LOG_LEVEL_EXIT_QUITS (0/1) exit() on LOG_LEVEL_ERROR
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <pthread.h>

typedef enum {
  LOG_LEVEL_INFO = 0,
  LOG_LEVEL_WARN = 1,
  LOG_LEVEL_ERROR = 2
} Log_Level;

/* Default config */
#ifndef LOG_DEBUG
#define LOG_DEBUG 0
#endif

#ifndef LOG_USE_MUTEX
#define LOG_USE_MUTEX 0
#endif

#ifndef LOG_LEVEL_EXIT_QUITS
#define LOG_LEVEL_EXIT_QUITS 1
#endif


void log_print(Log_Level level, int should_exit,
               const char* file, int line, const char* func,
               const char* format, ...);

#ifdef LOG_IMPLEMENTATION

#if LOG_USE_MUTEX
static pthread_mutex_t log_mutex = PTHREAD_MUTEX_INITIALIZER;
#endif

static const char* log_level_to_str(Log_Level lvl) {
  switch (lvl) {
  case LOG_LEVEL_INFO:  return "info";
  case LOG_LEVEL_WARN:  return "warn";
  case LOG_LEVEL_ERROR: return "error";
  default:              return "unknown";
  }
}

void log_print(Log_Level level, int should_exit,
               const char* file, int line, const char* func,
               const char* format, ...)
{
#if LOG_USE_MUTEX
  pthread_mutex_lock(&log_mutex);
#endif

  FILE* out = (level == LOG_LEVEL_INFO) ? stdout : stderr;

  if (LOG_DEBUG) {
    fprintf(out, "%s:%d: %s: in %s(): ",
            file, line, log_level_to_str(level), func);
  } else {
    fprintf(out, "%s: ", log_level_to_str(level));
  }

  va_list args;
  va_start(args, format);
  vfprintf(out, format, args);
  va_end(args);

  fflush(out);

  if (LOG_LEVEL_EXIT_QUITS && should_exit)
    exit(EXIT_FAILURE);

#if LOG_USE_MUTEX
  pthread_mutex_unlock(&log_mutex);
#endif
}

#endif /* LOG_IMPLEMENTATION */


#define log_info(...)                                                   \
  log_print(LOG_LEVEL_INFO, 0, __FILE__, __LINE__, __func__, __VA_ARGS__)

#define log_warn(...)                                                   \
  log_print(LOG_LEVEL_WARN, 0, __FILE__, __LINE__, __func__, __VA_ARGS__)

#define log_error(...)                                                  \
  log_print(LOG_LEVEL_ERROR, 1, __FILE__, __LINE__, __func__, __VA_ARGS__)

#endif /* LOG_H */
