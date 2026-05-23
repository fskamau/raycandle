#pragma once
#include "raycandle.h"
/* #include "raylib.h" */
/* #include "utils.h" */
/* #include <string.h> */


/* typedef enum ArtistType ArtistType; */
Artist* artist_create(Axes *axes, ArtistType atype, float thick, Color* colors,void *data);

struct ArtistVTable{
  void (*init)(Artist* artist,Axes* axes);
  Limit (*get_limit)(Artist* artist, Axes* axes);
  void (*draw)(Artist* artist, Axes* axes, Rect area); 
};

  

/* #endif //[ */
