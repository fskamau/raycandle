#include "arena.h"
#include <stdbool.h>

typedef struct Node Node;
typedef struct {
  float x,y,w,h;
}Rect;

struct Node{
  Rect area;
  size_t count;
  Node* nodes;
};

typedef struct {
  Arena* arena;
  Node* root;
  Rect** nodes;
  
  size_t count;
}NC; //NodeContext

NC *nc_create(size_t bytes);
//split a node. if ratios is NULL then split into equal `count` parts
Node *nc_divide(NC *nc, Node *parent, size_t* ratios, size_t count, bool vertical);
void nc_grid(NC* nc, Node *node, size_t rows, size_t cols);
void nc_compile(NC *nc); // allocate node rects. cannot split further. use nc->count afterwards.
void nc_set_area(NC* nc, Rect* dest, Rect area, float spacing); // set area the dest of len nc->count. only after nc_compile. 
