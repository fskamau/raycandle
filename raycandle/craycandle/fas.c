#include "fas.h"

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#define CM_IMPLEMENTATION
#define CM_SILENT 1
#include "cust_malloc.h"

#define FAS_ERROR(format, ...)                                          \
  do {                                                                  \
    fprintf(stderr, "%s:%d: error:[fas] " format, __FILE__, __LINE__,   \
            ##__VA_ARGS__);                                             \
    exit(EXIT_FAILURE);                                                 \
  } while (0)

typedef struct As As;
struct As {
  As *next;
  uint32_t x, y, w, h;
  char c;
}; // axes skeleton
#define FAS_AS_IS_EQUAL(as1, as2)                                       \
  ((as1).x == (as2).x && (as1).y == (as2).y && (as1).w == (as2).w &&    \
   (as1).h == (as2).h && (as1).c == (as2).c)

static As *as_create(void);
static As *as_get_as_by_char(As *parent, char c);
void as_print(As *as);

static As *as_create(void) {
  CM_MALLOC(As * as, sizeof(As));
  memset(as, 0, sizeof(As));
  return as;
}

static As *as_get_as_by_char(As *parent, char c) {
  while (parent->c != '\0') {
    if (parent->c == c)
      return parent;
    if (parent->next == NULL)
      break;
    parent = parent->next;
  }
  return NULL;
}

As *as_add(As *parent, As child) {
  if (parent->c == '\0') {
    *parent = child;
    return parent;
  }
  As *as_new = as_get_as_by_char(parent, child.c);
  if (as_new != NULL)
    return as_new;
  while (parent->next != NULL)
    parent = parent->next;
  as_new = as_create();
  *as_new = child;
  as_new->next = NULL;
  parent->next = as_new;
  return as_new;
}

void as_print(As *as) {
  const char sep = '\n';
  printf("As[%c", sep);
  while (as != NULL) {
    printf("('%c', %d, %d, %d, %d)%c", as->c, as->x, as->y, as->w, as->h, sep);
    as = as->next;
  }
  printf("]\n");
}

unsigned int str_count_consecutive(char *str, char c) {
  unsigned int count = 0;
  while (str[count] != '\0' && str[count] == c) {
    count += 1;
  }
  return count;
}

#define FAS_CHAR_IS_SEP(c) ((c) == ' ' || (c) == '\t')

void jump_to_non_sep(char**outline){
  while(**outline!='\0'&&(**outline==' '||**outline=='\t'))
      (*outline)++;
}


#define FAS_CHECK_INVALID_SHAPE(cond) do{                               \
    if(!(cond))FAS_ERROR("invalid shape. row %d has %d column(s) != %d column(s) in row 1\n",rows,col,cols); \
  }while(0)

Fas fas_parse(char *outline) {
  if (!outline)
    FAS_ERROR("cannot parse NULL\n");
  if (outline[0] == '\0')
    FAS_ERROR("cannot parse empty string\n");
  uint32_t rows, col,cols, temp_row, temp_col, as_len;
  char this_char;
  As *parent = as_create();
  As *as = NULL;
  bool is_this_sep = false, is_last_sep = false, is_next_eol = false;
  unsigned int consecutive_count;

  rows=col=cols=1;
  cols=0;

  jump_to_non_sep(&outline);
  while (1) {
    this_char=*outline;      
    if(rows==1){
      cols+=1;
    }
    {
      as=as_get_as_by_char(parent,this_char);
      As temp=(As){.x=col,.y=rows,.w=1,.h=1,.c=this_char};
      if(!as){
        as_add(parent,temp);
      }
      else{
        if(rows==as->y){//same row
          if(col==as->x+as->w){
            as->w+=1;
          }
          else{
            FAS_ERROR("non consecutive column occurrence of char '%c'; skipped from col %d to col %d in row %d\n",this_char,as->x+as->w-1,col,rows);
          }
        }
        else{
          if(rows!=as->y+as->h){//skipped row
            FAS_ERROR("skipped  row occurrence of char '%c'; skipped from [%d,%d] to [%d,%d]\n",this_char,as->y,as->x,rows,col);
          }
          if(col!=as->x){//no column alignment
            FAS_ERROR("no column alignment of char '%c'  [row %d col %d] & [row %d col %d]\n",this_char,as->y,as->x,rows,col);
          }
          unsigned int consecutive_count;
          if(as->w!=1&&(consecutive_count=str_count_consecutive(outline,this_char))!=as->w){
            FAS_ERROR("char '%c' has %d occurrence in row %d !=  %d occurrences in row %d\n",this_char,consecutive_count,rows,as->w,as->y);
          }
          outline+=(as->w-1);
          col+=as->w-1;
          as->h+=1;
        }
      }
    }
    outline++;
    if(FAS_CHAR_IS_SEP(*outline)){
      if(rows!=1){
        FAS_CHECK_INVALID_SHAPE(col==cols);
      }
      jump_to_non_sep(&outline);
      if(*outline=='\0')break;
      col=1;
      rows+=1;
    }
    else{
      if(*outline=='\0'){
        if(!FAS_CHAR_IS_SEP(*(outline-1)))
          FAS_CHECK_INVALID_SHAPE(col==cols);
        exit(0);
      }
      col+=1;
    }
  }
}

void fas_print(Fas fas) {
  unsigned int *ld = fas.skel;
  printf("Fas(len: %d rows: %d cols: %d skel=[\n", fas.len, fas.rows, fas.cols);
  for (size_t i = 0; i < fas.len; ++i) {
    unsigned int *cur=ld + (i * 4);
    printf(" %c=[%d %d %d %d],\n", fas.labels[i], cur[0],cur[1], cur[2], cur[3]);
  }
  printf("])\n");
}

void fas_destroy(Fas fas){
  cm_free_ptrv(&fas.skel);
  cm_free_ptrv(&fas.labels);
}

int main(){

  Fas f=fas_parse(" teeee reeee reeee");
 fas_print(f);

  return 0;
}
  
