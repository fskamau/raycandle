#include "fas.h"
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdbool.h>

/* #define CM_IMPLEMENTATION */
#include "cust_malloc.h"

#define FAS_ERROR(format,...)do{                    \
    fprintf(stderr,"%s:%d: error:[fas] "format ,    \
            __FILE__,__LINE__,##__VA_ARGS__);       \
    exit(EXIT_FAILURE);                             \
  }while(0)

typedef struct As As;
struct As {
  As* next;
  uint32_t x,y,w,h;
  char c;
};//axes skeleton
#define FAS_AS_IS_EQUAL(as1,as2) ((as1).x==(as2).x&&(as1).y==(as2).y&&(as1).w==(as2).w&&(as1).h==(as2).h&&(as1).c==(as2).c)

static As* as_create(void);
static As* as_get_as_by_char(As* parent, char c);
void as_print(As* as);

static As* as_create(void){
  CM_MALLOC(As *as,sizeof(As));
  memset(as,0,sizeof(As));
  return as;
}

static As* as_get_as_by_char(As* parent, char c){
  while(parent->c!='\0'){
    if(parent->c==c)return parent;
    if(parent->next==NULL)break;
    parent=parent->next;
  }
  return NULL;
}

As* as_add(As* parent,As child){
  if(parent->c=='\0'){
    *parent=child;
    return parent;
  }
  As *as_new=as_get_as_by_char(parent,child.c);
  if(as_new!=NULL)return as_new;    
  while(parent->next!=NULL)
    parent=parent->next;
  as_new=as_create();
  *as_new=child;
  as_new->next=NULL;
  parent->next=as_new;
  return as_new;
}

void as_print(As* as){
  const  char sep='\n';
  printf("As[%c",sep);
  while(as!=NULL){
    printf("('%c', %d, %d, %d, %d)%c",
           as->c,as->x,as->y,as->w,as->h,sep);
    as=as->next;}
  printf("]\n");
}
  
unsigned int str_count_consecutive(char* str,char c){
  unsigned int count=0;
  while(str[count]!='\0'&&str[count]==c){
    count+=1;
  }
  return count;
}

#define FAS_CHAR_IS_SEP(c) ((c)==' '||(c)=='\t')

Fas fas_parse(char *outline){
  if(!outline)FAS_ERROR("cannot parse NULL\n");
  if(outline[0]=='\0')FAS_ERROR("cannot parse empty string\n");
  uint32_t rows,cols,temp_row,temp_col,as_len,char_counter;
  char this_char;
  rows=cols=temp_row=temp_col=as_len=char_counter=0;
  As *parent=as_create();
  As* as=NULL;
  bool is_this_sep=false,is_last_sep=false,is_eol=false;

  unsigned int consecutive_count;
  temp_col=0;
  temp_row=1;
  cols=0;
  rows=1;
  while((this_char=*(outline++))){
    is_eol=false;
    temp_col+=1;
    char_counter+=1;
    is_last_sep=is_this_sep;
    is_this_sep=FAS_CHAR_IS_SEP(this_char);
    if((char_counter>1&&is_this_sep&&!is_last_sep))temp_row+=1;
    if(cols!=0
       &&((char_counter>1&&!is_last_sep&&is_this_sep)||((is_eol=(*outline=='\0'))&&(temp_col!=1)))
       &&(is_eol?temp_col:temp_col-1)<cols){
      FAS_ERROR("corrupt shape: %d column(s) in row %d are less than %d column(s) in row 1\n"
                ,temp_col-1,rows,cols);
    }
    if(is_this_sep){
      if(as_len==0){temp_col=0;continue;}
      else if(cols==0)cols=temp_col-1;
      temp_col=0;
      if(!is_last_sep){
        rows+=1;
        temp_row=rows;
      }
      continue;
    }
    if(cols!=0&&temp_col>cols){
      FAS_ERROR("corrupt shape: %d column(s) in row %d exceed %d column(s) in row 1\n"
                ,temp_col,temp_row,cols);
    }
    As temp=(As){.x=temp_col,.y=temp_row,.w=1,.h=1,.c=this_char};
    As* as=as_add(parent,temp);
    if(FAS_AS_IS_EQUAL(*as,temp)){//new char
      as_len+=1;
      as->w=str_count_consecutive(outline-1,this_char);
      outline+=as->w-1;
      temp_col+=as->w-1;
    }
    else{//char exists
      if(as->x!=temp.x)//not consecutive e.g aca
        FAS_ERROR("skipped char %c occurrence in row %d col %d\n",
                  this_char,temp_row,temp_row);
      if(as->y+as->h==temp.y){//new consecutive row
        if((consecutive_count=str_count_consecutive(outline-1,this_char))!=as->w)
          FAS_ERROR("less or more char %c occurrence in row %d than in previous row %d\n",
                    this_char,temp_row,as->h+as->y-1);
        temp_col+=consecutive_count-1;
        outline+=consecutive_count-1;
        as->h+=1;
      }
      else{
        FAS_ERROR("skipped char occurrence between row %d and row  %d along col %d\n",
                  as->y+as->h-1,temp_row,temp_col);}
    }
  }
  CM_MALLOC(unsigned int *skel,sizeof(*skel)*4*as_len);
  CM_MALLOC(char* labels,sizeof(*labels)*as_len);
  unsigned int counter=0;
  while(parent!=NULL){
    unsigned int buffer[4]={parent->x-1,parent->y-1,parent->w,parent->h};
    memcpy(skel+counter*4,buffer,4*sizeof(*skel));
    labels[counter]=parent->c;
    counter+=1;
    as=parent;
    parent=parent->next;
    cm_free_ptrv(&as);
  }
  Fas fas;
  fas=(Fas){.len=as_len,.rows=rows,.cols=cols,.skel=skel,.labels=labels};
  return fas;
}


void fas_print(Fas fas){
  printf("Fas(len: %d rows: %d cols: %d skel=[\n",fas.len,fas.rows,fas.cols);
  unsigned int* ld=fas.skel;
  for(size_t i=0;i<fas.len;++i){
    printf(" %c=[%d %d %d %d],\n",
           fas.labels[i],(ld+(i*4))[0],(ld+(i*4))[1],(ld+(i*4))[2],(ld+(i*4))[3]);
  }
  printf("])\n");
}
