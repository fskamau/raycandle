#ifndef _RAYCANDLE_UTILS_
#define  _RAYCANDLE_UTILS_
#include "arena.h"
#include "str.h"

#define ray_error(fmt, ...) do {                                        \
    fprintf(stderr,"%s:%d: Error " fmt ,__FILE__, __LINE__, ##__VA_ARGS__); \
    exit(1);                                                            \
  } while(0)



#define ASSERT(cond) if(!(cond)) ray_error("cond ("#cond ") failed\n")
#define ASSERTM(cond, fmt, ...) if(!(cond)) ray_error(fmt, ##__VA_ARGS__);

#endif //_RAYCANDLE_UTILS_
