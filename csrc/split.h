#include "arena.h"
#include <stdbool.h>

typedef struct Node Node;
typedef struct {
  float x,y,w,h;
}Rect;

struct Node{
  Rect area;
  size_t node_count;
  float depth;
  Node* nodes;
  size_t* ratios;
  bool is_vertical;
};

typedef struct {
  float spacing;
  Arena* arena;
  Node* root;
}NodeContext;

typedef NodeContext NC;
