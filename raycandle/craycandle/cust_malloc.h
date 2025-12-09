/**
 * CustomMalloc (CM)
 * =================
 * A tiny malloc/free tracker for catching segfaults, double frees,
 * freeing pointers not allocated by CM, and for logging memory activity.
 *
 * HOW TO USE
 * ----------
 * In the C  file:
 *     #define CM_IMPLEMENTATION
 *     #include "cust_malloc.h"
 *
 * In any file:
 *     #include "cust_malloc.h"
 *
 * Allocate mem:
 *
 *     CM_MALLOC(int*array,sizeof(int)*100);
 *
 * Free a:
 *     CM_FREE(array);
 *
 * Free_everything:
 *     CM_FREE_ALL();
 *
 *
 * OPTIONAL MACROS
 * ----------------
 * 1. CM_OFF
 *      Disable all tracking.
 *      CM_MALLOC → malloc
 *      CM_FREE → free
 *      cm_free_all / cm_chain_length / CM_MALLOC_size / cm_pointer_chain_print  ->unavailable
 *
 * 2. CM_SILENT
 *      If nonzero, suppress all INFO logging (errors still print).
 *
 *
 *When satisfied, default to malloc and free with #define CM_OFF
 *
 *
 **/

#ifndef __CM__
#define __CM__
#include <stdlib.h>

#ifndef CM_SILENT
#define CM_SILENT 0
#endif // CM_SILENT

#ifdef CM_OFF
#include <string.h>
#define CM_MALLOC(target,bytes) target=malloc(bytes)
#define CM_FREE(target) free(target)

#define cm_malloc_size cm_malloc_size_is_undefined_when_CM_OFF_is_defined
#define cm_chain_length cm_chain_length_is_undefined_when_CM_OFF_is_defined
#define cm_free_all cm_free_all_is_undefined_when_CM_OFF_is_defined
#define cm_pointer_chain_print  cm_pointer_chain_print_is_undefined_when_CM_OFF_is_defined


#else // CM_OFF is not defined

#define CM_MALLOC(target, bytes) target = CM_DEFINE_ADD_FLF(cm_malloc,(bytes), #target)
#define CM_FREE(target) CM_DEFINE_ADD_FLF(cm_free,&(target))
#define CM_FREE_ALL() CM_DEFINE_ADD_FLF(cm_free_all)



#define CM_DEFINE_ADD_FLF(f, ...)                                              \
  f(__VA_ARGS__ __VA_OPT__(, ) __FILE__, __LINE__, __func__)
#define CM_FUNCTION_ADD_FLF(f, ...)                                            \
  f(__VA_ARGS__ __VA_OPT__(, ) const char *fname, int lineno, const char *func)


void *CM_FUNCTION_ADD_FLF(cm_malloc, size_t bytes, const char *target);
void CM_FUNCTION_ADD_FLF(cm_free, void *ptrv);
size_t cm_malloc_size();  // returns bytes allocated
size_t cm_chain_length(); // returns len of subsequent calls to malloc
void CM_FUNCTION_ADD_FLF(cm_free_all); // free the chain
void cm_pointer_chain_print();          // print the chain

#endif // CM_OFF

#ifdef CM_IMPLEMENTATION
#include <errno.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#ifndef CM_OFF
typedef struct PointerChain PointerChain;
struct PointerChain {
  size_t size;
  struct PointerChain *next;
};

typedef struct {
  size_t allocated, chain_length;
} PointerChainInfo_t;

static PointerChain PointerChain_v = {.size = 0, .next = NULL};
static PointerChainInfo_t PointerChainInfo = {.allocated = 0,
                                              .chain_length = 0};

#define CM_ERROR(format, ...)                                           \
  do {                                                                  \
    fprintf(stderr, "%s:%d: error: %s " format, fname, lineno, func,##__VA_ARGS__); \
    exit(EXIT_FAILURE);                                                 \
  } while (0)

#define CM_INFO(format, ...)                                            \
  do {                                                                  \
    if (!(CM_SILENT))                                                   \
      fprintf(stdout, "%s:%d: info in %s " format, fname, lineno, func,##__VA_ARGS__); \
  } while (0)

void *CM_FUNCTION_ADD_FLF(cm_malloc, size_t bytes, const char *target) {
  if (bytes == 0) {
    CM_ERROR("cannot malloc 0 bytes for target '%s'\n", target);
  }
  PointerChain *ptrc = &PointerChain_v;
  while (ptrc->next != NULL) {
    ptrc = ptrc->next;
  }
  ptrc = ptrc->next = (PointerChain *)malloc(sizeof(PointerChain) + bytes);
  if (ptrc == NULL) {
    CM_ERROR("could not malloc %'zu bytes for target 'PointerChain'; %s\n",
             bytes + sizeof(PointerChain), strerror(errno));
  }
  ptrc->size = bytes;
  ptrc->next = NULL;
  PointerChainInfo.allocated += bytes;
  PointerChainInfo.chain_length += 1;
  CM_INFO("malloc@%zu: %p %'zu bytes total %'10zu target: %s/%s\n",
          PointerChainInfo.chain_length, (char *)ptrc + sizeof(PointerChain),
          bytes, PointerChainInfo.allocated, fname, target);
  return (char *)ptrc + sizeof(PointerChain);
}

void CM_FUNCTION_ADD_FLF(cm_free, void *ptrv) {
  PointerChain *ptrc = &PointerChain_v;
  size_t id = 0;
  void *ptr = *(void **)ptrv;
  if (ptr==NULL)
    CM_ERROR("cannot free NULL\n");
  ptr -= sizeof(PointerChain);
  while (ptrc->next != NULL) {
    if (ptrc->next == ptr) {
      CM_INFO("free_ptrv@%zu %p %'10zu\n", id, *(void **)ptrv,
              ptrc->next->size);
      PointerChainInfo.chain_length -= 1;
      PointerChainInfo.allocated -= ptrc->next->size;
      ptrc->next = ptrc->next->next; // join the remaining structs
      free(ptr);                     // free the pointer
      *(void **)ptrv = NULL;         // reset mem to NULL
      return;
    }
    ptrc = ptrc->next;
    id += 1;
  }
  CM_ERROR("could not locate pointer '%p'\n", ptrv);
}

size_t cm_chain_length() { return PointerChainInfo.chain_length; }

void CM_FUNCTION_ADD_FLF(cm_free_all) {
  CM_INFO("Before free_all: allocated %'10zu chain length %6zu\n",
          PointerChainInfo.allocated, PointerChainInfo.chain_length);
  while (PointerChain_v.next != NULL) {
    void *p = ((char *)PointerChain_v.next + sizeof(PointerChain));
    CM_FREE(p);
  }
  if (PointerChainInfo.allocated != 0 || PointerChainInfo.chain_length != 0)
    CM_ERROR("corrupt chain\n");
  return;
}

size_t cm_malloc_size() { return PointerChainInfo.allocated; }

void cm_pointer_chain_print() {
  PointerChain *temp = PointerChain_v.next;
  size_t id = 1;
  printf("PointerChain=[\n");
  while (temp != NULL) {
    printf("%6zu %p %10zu\n", id, (char *)temp + sizeof(PointerChain),
           temp->size);
    temp = temp->next;
    id += 1;
  }
  printf("\tTotal: %zu bytes\n]\n", PointerChainInfo.allocated);
}

#endif // CM_OFF is not defined

#undef CM_ERROR
#undef CM_INFO

#endif // CM_IMPLEMENTATION

#undef TKNCCT
#undef CONCAT_L2
#undef CM_SILENT
#endif //__CM__
