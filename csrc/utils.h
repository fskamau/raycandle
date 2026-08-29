#ifndef _RAYCANDLE_UTILS_
#define  _RAYCANDLE_UTILS_
#include "arena.h"
#include "str.h"

#define ray_error(fmt, ...) do {                                        \
    fprintf(stderr,"%s:%d: Error " fmt ,__FILE__, __LINE__, ##__VA_ARGS__); \
    exit(1);                                                            \
  } while(0)



#define ASSERT(cond) if(!(cond)) ray_error("(assert) "#cond " failed\n")
#define ASSERTM(cond, fmt, ...) if(!(cond)) ray_error(fmt, ##__VA_ARGS__);

#define MIN(a,b) (a)<(b)?(a):(b)
#define MAX(a,b) (a)>(b)?(a):(b)

#define LIMIT_PRINT(l) printf("Limit(min=%f, max:%f)\n",(l).min, (l).max)


typedef enum {
  ALIGNMENT_LEFT,
  ALIGNMENT_CENTER,
  ALIGNMENT_RIGHT,
} Alignment;


typedef struct Font Font;
float align_text(Font *font, const char *text, int width, int font_size, int spacing, Alignment alignment);
void epoch2strftime(int epoch, Str buffer, const char *format);
void set_window_icon(char *file_path);
#endif //_RAYCANDLE_UTILS_
