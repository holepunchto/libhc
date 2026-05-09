#ifndef HC_BUFFER_H
#define HC_BUFFER_H

#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
  size_t len;
  uint8_t *buffer;
} hc_buf_t;

// Generic { buffers, length, capacity } shape used by the typed array
// structs (hc_small_key_array_t, hc__tree_node_buffer_array_t, ...). Casting
// any of those to hc__array_header_t * works because the leading three
// fields share representation (pointer + 2 size_t).
typedef struct {
  void *buffers;
  size_t length;
  size_t capacity;
} hc__array_header_t;

static inline int
hc_array_grow (void *array, size_t elem_size, size_t min_capacity) {
  hc__array_header_t *h = array;
  if (h->capacity >= min_capacity) return 0;
  size_t new_capacity = h->capacity == 0 ? 8 : h->capacity * 2;
  while (new_capacity < min_capacity) new_capacity *= 2;
  void *new_buffers = realloc(h->buffers, new_capacity * elem_size);
  if (new_buffers == NULL) return -1;
  h->buffers = new_buffers;
  h->capacity = new_capacity;
  return 0;
}

// Type-safe wrapper: derives the element size from the array's buffers
// field so a mismatched sizeof can't sneak in.
#define HC_ARRAY_GROW(arr, n) \
  hc_array_grow(&(arr), sizeof(*(arr).buffers), (n))

#ifdef __cplusplus
}
#endif

#endif // HC_BUFFER_H
