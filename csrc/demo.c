#include "utils.h"
#include "raylib.h"
#include "figure.h"
#include "node.h"
/* (Rect){   0.0    0.0  181.8  600.0 } */
/* (Rect){ 181.8    0.0   72.7  600.0 } */
/* (Rect){ 254.5    0.0  109.1  600.0 } */
/* (Rect){ 363.6    0.0  181.8  600.0 } */
/* (Rect){ 763.6    0.0  181.8  600.0 } */
/* (Rect){ 727.3    0.0   72.7  600.0 } */
/* (Rect){   0.0    0.0  181.8  600.0 } */
#define RECT_PRINT(r) printf("(Rect){% 6.1f % 6.1f % 6.1f % 6.1f }\n",(r).x,(r).y,(r).w,(r).h)
#define UNWRAP_RECT(r) (r).x, (r).y, (r).w, (r).h

void node_draw(Rect* areas, size_t count){
  for(size_t i=0;i<count;++i){
    DrawRectangle(UNWRAP_RECT(areas[i]),RED);
    char buffer_[128]={0};
    char *buffer=str_create(buffer_,sizeof(buffer_)/sizeof(buffer_[0]));
    str_append(buffer,"%zu [%.2f %.2f %.2f %.2f ]",i,UNWRAP_RECT(areas[i]));
    DrawText(buffer,areas[i].x+10,areas[i].y+20,10,YELLOW);
    str_clear(buffer);    
  }
  
}
int main(){

  Arena *arena=arena_create(1024);
  NC* nc=nc_create(1024*1024);
    Node *x=nc_divide(nc,nc->root,(size_t[]){5,2,3,10,2},5,1);
    for(size_t i=0;i<5;++i) nc_divide(nc,&x[i],(size_t[]){5,2,3,10,2},5,0);
/*     nc_divide(nc,&x[3],(size_t[]){2,2,5,3,4},5,1); */
/*     x=nc_divide(nc,x,(size_t[]){2,2},2,0); */

/*     x=nc_divide(nc,&x[0],NULL,20,1); */
/*     x=nc_divide(nc,&x[1],(size_t[]){3,2},2,1); */

/*   nc_grid(nc, nc->root, 4,5); */
  nc_compile(nc);
  Rect* dest=arena_alloc(arena,sizeof(*dest)*nc->count);
    
/*   return 0; */

  const int screenWidth = 800;
  const int screenHeight = 600;
  SetConfigFlags(FLAG_WINDOW_RESIZABLE);
  SetConfigFlags(FLAG_MSAA_4X_HINT);
  SetTraceLogLevel(LOG_ERROR);
  InitWindow(screenWidth, screenHeight, "RayCandle - Grid Layout");
  SetTargetFPS(60);
  while (!WindowShouldClose()) {
    // Update: Recalculate areas if window resized (optional)
    Rect screen = { 0, 0, (float)GetScreenWidth(), (float)GetScreenHeight() };
    nc_set_area(nc, dest, screen, 10.f);

    SetExitKey(KEY_Q);
    BeginDrawing();
    ClearBackground(RAYWHITE);
            
    // Draw the nodes
    node_draw(dest,nc->count);
    DrawFPS(10, 10);
    EndDrawing();
  }

  CloseWindow();
  return 0;  
  return 0;
}
