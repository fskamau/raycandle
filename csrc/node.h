#ifndef __NODE_H__
#define __NODE_H__

#include <stdbool.h>
#include <assert.h>
#include "arena.h"
#include "utils.h"

#define NODE_RECT_PRINT(r) printf("(Rect){% 6.1f % 6.1f % 6.1f % 6.1f }\n",(r).x,(r).y,(r).w,(r).h)
#define NODE_RECT_UNWRAP(r) (r).x, (r).y, (r).w, (r).h
#define NODE_RECT2RECTANGLE(r) (Rectangle){NODE_RECT_UNWRAP((r))}


typedef struct {
  float x,y,w,h;
}Rect;

typedef struct Node Node;
struct Node{
  Rect area;
  size_t count;
  Node* nodes;
};

typedef struct NData NData;
typedef struct NC NC;
struct NC {
  Node* root;//default root node
  NData* data;
}; //NodeContext



NC *nc_create(size_t bytes);
void nc_divide(NC *nc, Node *parent, size_t* ratios, size_t count, bool vertical); //split a node. if ratios is NULL then split into equal `count` parts
void nc_grid(NC* nc, Node *node, size_t rows, size_t cols);
size_t nc_compile(NC *nc); // compiles nodes and returns their length. Allocate Rects of length returned, then call nc_set_area to update
void nc_set_area(NC* nc, Rect* dest, Rect area, float spacing); // set area the dest of len nc->count. only after nc_compile.
Node* node_at(Node* parent, size_t index);
#endif // __NODE_H__
