#include "node.h"
#include <string.h>
#include "utils.h"
#include "math.h"
#include <string.h>

#define RECT_PRINT(r) printf("(Rect){% 6.2f % 6.2f % 6.2f % 6.2f }\n",(r).x,(r).y,(r).w,(r).h)


struct NData {
  Arena* arena;
  Rect** nodes;  
  size_t count;
};
  
NC *nc_create(size_t bytes)
{
  Arena* arena=arena_create(bytes);
  NC *nc = arena_alloc(arena, sizeof(NC));    
  nc->root = arena_alloc(arena, sizeof(Node));
  memset(nc->root, 0, sizeof(Node));
  nc->root->area=(Rect){0.f,0.f,1.f,1.f};
  nc->data=arena_alloc(arena,sizeof(*nc->data));
  memset(nc->data,0,sizeof(*nc->data));
  nc->data->arena=arena;
  return nc;
}


void nc_divide(NC *nc, Node *parent, size_t* ratios, size_t count, bool vertical)
{
  ASSERT(nc&&parent);
  ASSERTM(!nc->data->nodes, "node already compiled");
  ASSERT(count>1);
  ASSERTM(parent->count==0,"node already divided\n");
  parent->count=count;
  parent->nodes=arena_alloc(nc->data->arena, sizeof(Node) * count);
  memset(parent->nodes,0,sizeof(Node) * count);
  float total_count=0;
  float cum=0.f;
  float divr=1.f/count;
  Rect p_area=parent->area;
  if(ratios){
    for(size_t i=0; i<count; ++i){
      ASSERT(ratios[i]>0);
      total_count+=ratios[i];
    }
  }
  for(size_t i=0; i<count; ++i){
    float r=ratios?(ratios[i]/total_count):divr;
    Rect* area=&parent->nodes[i].area;
    *area=p_area;
    if(vertical){
      area->x=p_area.x+cum;
      area->w=p_area.w*r;
      cum+=area->w;
    }
    else{
      area->y=p_area.y+cum;
      area->h=p_area.h*r;
      cum+=area->h;
    }
  }
}

void nc_grid(NC* nc, Node *node, size_t rows, size_t cols)
{
  ASSERT(nc&&node);
  ASSERT(rows>0&&cols>0);
  ASSERTM(!(rows==1&&cols==1), "already a 1row x 1col\n");
  if(rows>1){
    nc_divide(nc,node, NULL, rows , false);
    node=node->nodes;
  }
  if(cols>1)
    for(size_t i=0; i<rows;++i)
      nc_divide(nc,node+i, NULL, cols , true); //if node has no nodes then its ok to node+0 since if !node->nodes then rows=1~0
}


static void node_count(Node* node, size_t* count)
{
  if(node->count==0){
    *count+=1;
    return;
  }
  for(size_t i=0; i<node->count; ++i)node_count(&node->nodes[i],count);        
}

static size_t nc_count(NC* nc)
{
  ASSERT(nc);
  size_t count=0;
  node_count(nc->root,&count);
  return  count;
}

static void nc_set_nodes(NC*nc, Node* node, size_t *count)
{
  if(node->count==0){
    nc->data->nodes[*count]=&node->area;
    (*count)++;
    return;
  }
  for(size_t i=0; i<node->count;++i)
    nc_set_nodes(nc,&node->nodes[i], count);
}

static int qsort_compare_nodes(const void* a, const void* b){
    Rect *ra = *(Rect**)a;
    Rect *rb = *(Rect**)b;
    if (fabsf(ra->y - rb->y) > 0.001f) 
        return (ra->y < rb->y) ? -1 : 1;
    if (ra->x < rb->x) return -1;
    if (ra->x > rb->x) return 1;    
    return 0;
}

size_t nc_compile(NC *nc)
{
  ASSERT(nc);
  ASSERTM(!nc->data->nodes,"already compiled\n");
  size_t count=nc_count(nc);
  Rect** nodes=arena_alloc(nc->data->arena, sizeof(*nc->data->nodes)*count);  
  nc->data->nodes=nodes;
  nc->data->count=count;
  count=0;
  nc_set_nodes(nc, nc->root, &count);
  qsort(nc->data->nodes, nc->data->count, sizeof(*nc->data->nodes),qsort_compare_nodes);
  return nc->data->count;
}


void nc_set_area(NC* nc, Rect* dest, Rect area, float spacing)
{
    ASSERT(nc && dest);
    ASSERTM(nc->data->nodes, "call nc_compile first");
    const int pad = (int)roundf(spacing);
    for (size_t i = 0; i < nc->data->count; ++i) {
        Rect* na = nc->data->nodes[i];
        float fx0 = area.x + na->x * area.w;
        float fy0 = area.y + na->y * area.h;
        float fx1 = area.x + (na->x + na->w) * area.w;
        float fy1 = area.y + (na->y + na->h) * area.h;

        int x0 = (int)floorf(fx0);
        int y0 = (int)floorf(fy0);
        int x1 = (int)ceilf(fx1);
        int y1 = (int)ceilf(fy1);

        x0 += pad;
        y0 += pad;
        x1 -= pad;
        y1 -= pad;

        if (x1 < x0) x1 = x0;
        if (y1 < y0) y1 = y0;

        dest[i] = (Rect) {
            .x = (float)x0,
            .y = (float)y0,
            .w = (float)(x1 - x0),
            .h = (float)(y1 - y0)
        };
    }
}


Node* node_at(Node* parent, size_t index)
{
  ASSERT(parent);
  ASSERTM(parent->count!=0,"no children\n");
  ASSERTM(index<parent->count,"index error");
  return parent->nodes+index;
}
