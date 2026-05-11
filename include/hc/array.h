#ifndef HC_ARRAY_H
#define HC_ARRAY_H

#include <stddef.h>
#include <stdlib.h>

#ifdef __cplusplus
extern "C" {
#endif

// Internal layout used by hc__array_grow_ to manipulate any HC__ARRAY.
typedef struct {
  void *buffers;
  size_t length;
  size_t capacity;
} hc__array_t;

// Declares a typed growable array. Use as a struct member:
//   HC__ARRAY(my_type_t) items;
#define HC__ARRAY(T) struct { T *buffers; size_t length; size_t capacity; }

static inline int
hc__array_grow_ (void *array, size_t elem_size, size_t min_capacity) {
  hc__array_t *h = array;
  if (h->capacity >= min_capacity) return 0;
  size_t new_capacity = h->capacity == 0 ? 8 : h->capacity * 2;
  while (new_capacity < min_capacity) new_capacity *= 2;
  void *new_buffers = realloc(h->buffers, new_capacity * elem_size);
  if (new_buffers == NULL) return -1;
  h->buffers = new_buffers;
  h->capacity = new_capacity;
  return 0;
}

#define hc__array_grow(array, min_capacity) \
  hc__array_grow_((array), sizeof(*(array)->buffers), (min_capacity))

#ifdef __cplusplus
}
#endif

#endif // HC_ARRAY_H
