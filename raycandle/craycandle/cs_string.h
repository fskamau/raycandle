#ifndef __CS__//custom_string
#define __CS__

/*
allocated string and returns a printable adress to start of the string
if mem is NULL, string will be dynamic otherwise, it maybe a stack one
Please NOTE Carefully:
  1.The string structure is stored inside the string use string_len to get the length
     #define LEN 16
     char mystr[LEN]={0};
     char* str=string_create(LEN,mystr);
  string_len(str)=LEN-sizeof(_String)-1(due to \0)
  2. A dynamic string len will not change. i.e
     char* str=string_create(LEN,NULL);
     string_len(str)=LEN
  3.string_create_formatted may take len=0 and mem=NULL to allocate the formatted string in memory
 */

typedef char* Str;

Str string_create(unsigned int len,void* mem);
Str string_create_from_format(unsigned int len,void* mem,const char* format,...);//return a formatted string
void string_append(Str str,const char* format,...);
void string_summarize(Str str);//print a summary of the string
void string_destroy(Str str);//frees a string if its a dynamic string
unsigned int string_len(Str str);
unsigned int string_get_remaining(Str str);
char* string_get_current_point(Str str);
void string_print(Str str);
void string_clear(Str str);
void string_increase_len(Str str,unsigned int len);//some functions take str and return how much they put in the str

#ifdef CS_IMPLEMENTATION

#include <stddef.h>
#include <stdarg.h>
#include <stdio.h>

#define CM_IMPLEMENTATION
#include "cust_malloc.h"

#define STRING_ERROR(format,...)do{				\
    fprintf(stderr,"%s:%d: _StringError: ",__FILE__,__LINE__);	\
    fprintf(stderr,format ,##__VA_ARGS__);exit(1);}		\
  while(0)

#define STRING_CHECK_CONDITION(cond,msg) do{				\
    if(!(cond)) STRING_ERROR("condition %s failed%s%s\n",#cond,(msg)?": ":"",(msg)?msg:"");} \
  while(0)

#define STRING_ALLOC(name,size) name=cm_malloc((size),#name)
#define STRING_GET_START(string_ptr) (((char*)(string_ptr))+(sizeof(_String)))
#define STRING_GET_CURRENT(string_ptr) (STRING_GET_START((string_ptr))+(string_ptr)->len)


typedef struct{
  unsigned int capacity;
  unsigned int len;
  unsigned char stack;
}_String;

static inline _String* string_get(Str str);//returns a reference to the string structure
static inline void _string_append(Str str,const char* format,va_list args,unsigned int total_length);


static inline  _String* string_get(Str str){
  _String* s=(_String*)((char*)str-sizeof(_String));
#define STRING_SMC "string may be corrupted\n"
  STRING_CHECK_CONDITION((int)s->len>=0,STRING_SMC);
  STRING_CHECK_CONDITION((int)s->len<=((int)s->capacity),"");
#undef STRING_SMC
  return s;
}


static inline  void _string_append(Str str, const char* format,va_list args,unsigned int total_length){
  _String* string=string_get(str);
  unsigned int newl;
  if(total_length==0){
    va_list args2;
    va_copy(args2,args);
    if((newl=vsnprintf(NULL,0,format,args2))+string->len>string->capacity){
      newl-=string->capacity-string->len;
      STRING_ERROR(string_create_from_format(0,NULL,"need more space to fit %d item%s\n",newl,newl==1?"":"s"));
    }
    va_end(args2);
  }else STRING_CHECK_CONDITION(total_length<=string->capacity,"");
  string->len+=vsnprintf(STRING_GET_CURRENT(string),string->capacity-string->len+1,format,args);
}


Str string_create(unsigned int len,void* mem){
  if((int)len<=0)STRING_ERROR("len <= 0\n");
  char* str;
  if(mem){
    STRING_CHECK_CONDITION(len>=sizeof(_String),"stack string too small to fit _String struct size");
    str=mem;
    len-=(1+sizeof(_String));
  }else{
    STRING_ALLOC(str,len+sizeof(_String)+1);
  }
  *((_String*)str)=(_String){.capacity=len,.len=0,.stack=mem?1:0};
  str[sizeof(_String)]='\0';
  string_get(str+(sizeof(_String)));
  return str+(sizeof(_String));
}


Str string_create_from_format(unsigned int len,void* mem,const char* format,...) {
  char*str;
  va_list args;
  va_start(args,format);
  unsigned char has_total_len=0;
  if(!mem &&len){STRING_ERROR("len is not 0 but mem is NULL\n");}
  if(len==0&&!mem){
    va_list args2;
    va_copy(args2,args);
    len=vsnprintf(NULL,0,format,args2);
    va_end(args2);
    has_total_len=1;
  }
  str=string_create(len,mem);
  _string_append(str,format,args,has_total_len?len:0);
  return str;
}

void string_append(Str str,const char* format,...){
  va_list args;
  va_start(args,format);
  _string_append(str,format,args,0);
  va_end(args);
}


void string_summarize(Str str){
  _String* string=string_get(str);
  printf("String(capacity=%'d len=%'d, inheap=%d)\n",string->capacity,string->len,!string->stack);
}


void string_destroy(Str str){
  _String* string=string_get(str);
  string->capacity=0;
  string->len=0;
  if(string->stack)return;
  cm_free_ptrv(&string);
}


unsigned int string_len(Str str){
  return string_get(str)->len;
}

unsigned int string_get_remaining(Str str){
  _String* string=string_get(str);
  return string->capacity-string->len;
}


char* string_get_current_point(Str str){
  return STRING_GET_CURRENT(string_get(str));
}

void string_clear(Str str){
  _String* string=string_get(str);
  string->len=0;
  str[0]='\0';
}


void string_print(Str str){
  _String* string=string_get(str);
  unsigned int len=0;
  while(len<string->len){putchar(str[len++]);}
}

/*
  some funtions take a char* and return number of chars written. This len maybe passed here.
  Mostly, string_get_current_point should be used to return current buffe point to write.
  e.g string_increase_len(buffer,snprintf(string_get_current_point(buffer),64,"%s\n","hello"));

 */
void string_increase_len(Str str,unsigned int len){
  unsigned int remaining;
  if (len==0)return;
  _String* string=string_get(str);
  remaining=string->capacity-string->len;    
  if(len>remaining)
    STRING_ERROR("OverflowError: cannot increase len by %d; remaining is %d\n",len,remaining);
  string->len+=len;
  if(str[string->len]!='\0')
    STRING_ERROR("string len increased by %d but end is missing null terminator\n",len);
}

#endif// CS_IMPLEMENTATION

#undef STRING_GET_START
#undef STRING_GET_CURRENT
#undef STRING_ERROR
#undef STRING_CHECK_CONDITION

#endif //__CS__
