#include <assert.h>
#include <string.h>
#include <stdlib.h>
#define ARENA_IMPLEMENTATION
#include "arena.h"
#include "split.h"

NC *node_context_create(size_t bytes, float spacing)
{
  Arena *arena = arena_create(bytes);
  NC *nc = arena_alloc(arena, sizeof(NC));
  nc->root = arena_alloc(arena, sizeof(Node));
  *nc->root = (Node){0};
  nc->root->depth = 0.f;
  nc->arena = arena;
  nc->spacing = spacing;
  return nc;
}

#define NODE_AT(n,i) (assert(n&&n->nodes&&i<n->node_count),n->nodes+i)
#define RECT_PRINT(r) printf("(Rect){%.1f %.1f %.1f %.1f }\n",(r).x,(r).y,(r).w,(r).h)
Node *node_split(NC *ctx, Node *parent, size_t node_count, size_t* ratios, bool is_vertical)
{
  assert(ctx && parent);
  assert(node_count > 1);
  assert(!parent->nodes && "already split");
  parent->nodes = arena_alloc(ctx->arena, sizeof(Node) * node_count);
  memset(parent->nodes, 0, sizeof(Node) * node_count);
  parent->node_count = node_count;
  float depth = 1.f / (is_vertical ? 1.f : node_count);
  if(ratios)
    for(size_t i=0;i<node_count;++i)
      assert(ratios[i]>1);
  parent->ratios=ratios;
  for (size_t i = 0; i < node_count; ++i)
    parent->nodes[i].depth = parent->depth + depth * i;
  parent->is_vertical=is_vertical;
  return parent->nodes;
}

void node_grid(NC *ctx, Node* parent, size_t rows,size_t cols)
{
  if(rows>1)node_split(ctx,parent,rows,NULL,false);
  if(cols>1)
    for(size_t i=0;i<rows;++i)
      node_split(ctx,NODE_AT(parent,i),cols,NULL,true);    
}

static void node_count_(Node *node, size_t *count)
{
  if (!node->nodes)
  {
    (*count)++;
    return;
  }
  for (size_t i = 0; i < node->node_count; ++i)
    node_count_(&node->nodes[i], count);
}

size_t node_count(NC *ctx)
{
  size_t count = 0;
  node_count_(ctx->root,&count);
  return count;
}

static void node_collect_(Node *node, Node ***out)
{
  if (!node->nodes)
  {
    *(*out) = node;
    *out += 1;
    RECT_PRINT(node->area);
    return;
  }
  for (size_t i = 0; i < node->node_count; ++i)
    node_collect_(&node->nodes[i], out);
}

static int node_qsort_cmp(const void  *a, const void *b)
{
  return ((Node*)a)->depth-((Node*)b)->depth;
}

Node **node_collect(NC *ctx)
{
  size_t count = node_count(ctx);
  Node **out = arena_alloc(ctx->arena, sizeof(Node *) * count);
  Node ***out_ = &out;
  node_collect_(ctx->root,out_);
  qsort(out, count, sizeof(Node*), node_qsort_cmp);
  return out;
}

void node_set_area_(Node* node, Rect area)
{
  if(!node->nodes){
    node->area=area;
    return;
  }
  float *m,*n,o;    
  node->is_vertical?(m=&area.x,n=&area.w):(m=&area.y,n=&area.h);
  o=*m;
  *n/=node->node_count;  
  for(size_t i=0;i<node->node_count;++i){
    *m=o+*n*i;
    node_set_area_(node->nodes+i,area);
  }
}

void node_set_area(NC *ctx, Rect area)
{

  node_set_area_(ctx->root,area);

}

int main(){
  NC* ctx=node_context_create(1028*1024,20);
  node_grid(ctx,ctx->root,5,4);
  Rect area=(Rect){0,0,100,100};
  node_set_area(ctx, area);
  node_collect(ctx);
  return 0;
}
