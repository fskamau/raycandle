/**
                                CustomMalloc(CM)
                                ------------------
simple 1 header for printing calls to malloc and free (provided by stdlib)
                                NOTES
                                -----
1. To use the functionalities, define CM_IMPLEMENTATION before including this header
3. If an error occurs in the 4 functions,all allocated memory will be freed and the error is sent to stderr before exit(1)
4. When satisfied one may need to use the default malloc and free. To do this, define CM_OFF
                                provided functions
                                ------------------
1. void * cm_alloc(size_t bytes,const char* target):
            allocated bytes, prints the address  and target of allocation and returns its pointer
2. void  cm_free_ptrv(void* ptrv):
            frees the pointer stored in ptrv ( free(*ptrv)) and prints the freed pointer and **sets *ptrv to NULL
            Fails if ptrv || *ptrv is NULL or *ptrv was not allocated through alloc
3. void cm_free_all():
            frees all pointers that had not been freed yet and prints freed bytes
5. cm_malloc_size() returns the total memory that has been allocated. note deallocation does not reduce it
6. chain_length() returns length of stored pointers

1. CM_OFF: if defined, the functions will behave as follows:
    alloc(bytes,target) - call stdlib  malloc(bytes)
    free_ptrv(ptrv) - call stdlib free(*ptrv)
    cm_free_all()   -  will not exist
    cm_malloc_size() - will not exist
    cm_chain_length() - will not exist
2. CM_PREFIX: if defined to a value, functions are prefixed to value. eg #define CM_PREFIX MY_, alloc will be MY_alloc
3. CM_IMPLEMENTATION: if defined, including file will have the function implementation
4. CM_SILENT: if defined to any value with boolean *true*, no messages will be printed except for errors

*/

#ifndef __CM__
#define __CM__
#include <stdlib.h>

#ifndef CM_SILENT
#define CM_SILENT 0
#endif // CM_SILENT 


#define CM_MALLOC(bytes,target) target=cm_malloc((bytes),#target)

#ifndef CM_OFF
#define cm_malloc(bytes,target) _cm_malloc((bytes),(target),__FILE__,__LINE__)
#define cm_free_ptrv(ptrv) _cm_free_ptrv(ptrv,__FILE__,__LINE__)
#define cm_free_all() _cm_free_all(__FILE__,__LINE__)
void *_cm_malloc(size_t bytes, const char *target,const char *fname,int lineno);//malloc `bytes` 
void _cm_free_ptrv(void *ptrv,const char *fname,int lineno) ; // frees pointer stored in ptrc and sets ptrv to NULL
size_t cm_malloc_size();//returns bytes allocated
size_t cm_chain_length();//returns len of subsequent calls to malloc
void _cm_free_all(const char *fname,int lineno);//free the chain

#else //CM_OFF is defined
#define cm_malloc(bytes,_) malloc((bytes))
#define cm_free_ptrv(ptrv)  do{void* tmp;				\
  memcpy(&tmp, ptrv, sizeof(tmp));					\
  free(tmp);								\
  memset((ptrv), 0, sizeof(tmp));					\
  }while(0);
#define cm_malloc_size  cm_malloc_size_is_undefined_when_CM_OFF_is_defined_to_1
#define cm_chain_length  cm_chain_length_is_undefined_when_CM_OFF_is_defined_to_1
#define cm_free_all  cm_free_all_is_undefined_when_CM_OFF_is_defined_to_1
#endif // CM_OFF


#ifdef CM_IMPLEMENTATION
#include <stdarg.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>

#ifndef CM_OFF
typedef struct CM_PointerChain CM_PointerChain;
struct CM_PointerChain {
  void *ptr;
  size_t size;
  struct CM_PointerChain *next;
};

static CM_PointerChain cm_pointer_chain = {.ptr = NULL, .next = NULL};
static size_t cm_memory_allocated = 0.f;

#define CM_ERROR(format,...)do {fprintf(stderr,"%s:%d: ERROR: ",fname,lineno);fprintf(stderr, (format), ##__VA_ARGS__);cm_free_all();exit(EXIT_FAILURE);} while (0)
#define CM_INFO(format, ...)do {if(!(CM_SILENT)){fprintf(stdout,"%s:%d ",fname,lineno);fprintf(stdout, (format), ##__VA_ARGS__);}} while (0)


void *_cm_malloc(size_t bytes, const char *target,const char *fname,int lineno) {
  if(bytes==0){CM_ERROR("cannot malloc 0 bytes for target '%s'\n",target);}
  CM_PointerChain *ptrc = &cm_pointer_chain;
  if (ptrc->ptr != NULL) {
    while (ptrc->next != NULL) {
      ptrc = ptrc->next;
    }
    ptrc = ptrc->next = (CM_PointerChain *)malloc(sizeof(CM_PointerChain));
    if (ptrc == NULL) {
      int errno;
      CM_ERROR("could not malloc %'zu bytes for target 'CM_PointerChain'; %s\n", sizeof(CM_PointerChain), strerror(errno));
    }
    *ptrc = (CM_PointerChain){.ptr=NULL,.size=0,.next=NULL};
  }
  void *mem = malloc(bytes);
  if (mem == NULL) {
    CM_ERROR("could not malloc %'zu bytes for target '%s'; %s\n", bytes, target, strerror(errno));
  }
  ptrc->ptr = mem;
  ptrc->size=bytes;
  cm_memory_allocated += bytes;
  CM_INFO("malloc [%zu]: %p %'10zu target: %s/%s\n", cm_chain_length(),mem,bytes, fname,target);
  return mem;
}


void _cm_free_ptrv(void *ptrv,const char *fname,int lineno) {
  if(ptrv==NULL)CM_ERROR("cannot free NULL\n");
  void *ptr = *(void **)ptrv;
  if (ptr==NULL )CM_ERROR("ptrv points to null; expects an &ptr not the ptr itself\n");
  CM_PointerChain *ptrc = &cm_pointer_chain;
  if (ptrc->ptr == ptr) {
    CM_INFO("free_ptrv %p size %'zu<- %p\n",ptr,ptrc->size,ptrv);
    cm_memory_allocated-=ptrc->size;
    free(ptr);
    ptrc->ptr = NULL;
    *(void **)ptrv = NULL;
    return;
  }
  while (ptrc->next != NULL) {
    if (ptrc->next->ptr == ptr) {
      void *nextnext = ptrc->next->next; // maybe null if we free last ptr
      CM_INFO("free_ptrv %p size %'zu <- %p\n", ptr, ptrc->next->size,ptrv);
      cm_memory_allocated-= ptrc->next->size;
      free(ptr);// free the pointer then
      free(ptrc->next);  // free the pointer struct  then
      ptrc->next = (CM_PointerChain *)nextnext; // join the remaining structs
      *(void **)ptrv = NULL;//reset mem to NULL
      return;
    }
    ptrc = ptrc->next;
  }
  CM_ERROR("could not locate pointer '%p'\n", ptr);
}

size_t cm_chain_length(){
  CM_PointerChain *ptrc = &cm_pointer_chain;
  size_t count=ptrc->ptr != NULL;
  while (ptrc->next != NULL) {
    count++;
    if ((ptrc=ptrc->next)== NULL) {break;}
  }
  return count;
}


void _cm_free_all(const char *fname,int lineno){
  CM_PointerChain *ptrc = &cm_pointer_chain;
  CM_PointerChain *temp;
  size_t free_calls=1;
  if (ptrc->ptr != NULL) {
    CM_INFO("free_all [%'6zu] %'10zu %16p\n",free_calls++,ptrc->size,ptrc->ptr);
    free(ptrc->ptr);
  }
  ptrc=ptrc->next;//dont free static
  while (ptrc!= NULL) {
    temp = ptrc->next;
    CM_INFO("free_all [%'6zu] %16p size %'10zu and chain %16p\n",free_calls++,ptrc->ptr,ptrc->size,ptrc);
    free(ptrc->ptr);
    free(ptrc);
    ptrc=temp;
  }
  const char *size= (cm_memory_allocated > 1024.f ? (cm_memory_allocated /= 1024.f) > 1000.f ? (cm_memory_allocated /= 1024.f) > 1000.f ? (cm_memory_allocated /= 1024.f, "Gb") : "Mb" : "Kb" : "B");
  CM_INFO("freed approximately %'zu %s\n",cm_memory_allocated, size);
  //since in the same process we can alloc again, reset cm_pointer_chain
  cm_memory_allocated=0;
  cm_pointer_chain=(CM_PointerChain){.ptr=NULL,.size=0,.next=NULL};
  return;
}

size_t cm_malloc_size() {
  return (size_t)cm_memory_allocated;
}

#undef CM_ERROR
#undef CM_INFO

#endif // CM_OFF is not defined

#endif // CM_IMPLEMENTATION

#undef TKNCCT
#undef CONCAT_L2
#undef CM_SILENT
#endif //__CM__
