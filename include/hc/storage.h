#ifndef HC_STORAGE_H
#define HC_STORAGE_H

#include <stddef.h>
#include <stdint.h>

#include <kv.h>

#include "buffer.h"
#include "keys.h"
#include "merkle_tree.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct hc_storage_core_s hc_storage_core_t;

struct hc_storage_core_s {
  kv_t kv;
  uint64_t core_ptr;
  uint64_t data_ptr;
};

static inline int
hc_storage_core_init (hc_storage_core_t *storage, uint64_t core_ptr, uint64_t data_ptr) {
  kv_init(&storage->kv);
  storage->core_ptr = core_ptr;
  storage->data_ptr = data_ptr;
  return 0;
}

static inline void
hc_storage_core_destroy (hc_storage_core_t *storage) {
  kv_destroy(&storage->kv);
}

// Stack-allocatable key+value pair for a tree-node kv record.
typedef struct {
  hc_small_key_t key;
  hc__tree_node_buffer_t value;
} hc__tree_node_kv_t;

typedef struct {
  hc__tree_node_kv_t *buffers;
  size_t length;
  size_t capacity;
} hc__tree_node_kv_array_t;

// Pre-allocated payload slots for a tree-node write batch.
typedef struct {
  hc__tree_node_kv_array_t tree_nodes;
} hc_storage_write_batch_t;

static inline int
hc_storage_write_batch_init (hc_storage_write_batch_t *batch, size_t initial_tree_node_capacity) {
  batch->tree_nodes.buffers = NULL;
  batch->tree_nodes.length = 0;
  batch->tree_nodes.capacity = 0;
  return HC_ARRAY_GROW(batch->tree_nodes, initial_tree_node_capacity);
}

static inline void
hc_storage_write_batch_destroy (hc_storage_write_batch_t *batch) {
  free(batch->tree_nodes.buffers);
}

// Write batch: thin wrapper over kv_write_batch_t that takes hc_buf_t.

typedef kv_write_batch_t hc_write_batch_t;

static inline void
hc_write_batch_init (hc_write_batch_t *batch, kv_t *kv, size_t suggested_size) {
  kv_write_batch_init(batch, kv, suggested_size);
}

static inline int
hc_write_batch_put (hc_write_batch_t *batch, hc_buf_t key, hc_buf_t value) {
  return kv_write_batch_put(batch, key.buffer, key.len, value.buffer, value.len);
}

static inline int
hc_write_batch_del (hc_write_batch_t *batch, hc_buf_t key) {
  return kv_write_batch_del(batch, key.buffer, key.len);
}

static inline int
hc_write_batch_flush (hc_write_batch_t *batch) {
  return kv_write_batch_flush(batch);
}

static inline void
hc_write_batch_destroy (hc_write_batch_t *batch) {
  kv_write_batch_destroy(batch);
}

// Read batch: thin wrapper over kv_read_batch_t that takes hc_buf_t for the
// key and returns the value into a caller-provided hc_buf_t. The out struct
// must remain valid until hc_read_batch_flush returns; after flush, an
// unfound key leaves out->buffer == NULL.

typedef kv_read_batch_t hc_read_batch_t;

static inline void
hc_read_batch_init (hc_read_batch_t *batch, kv_t *kv, size_t suggested_size) {
  kv_read_batch_init(batch, kv, suggested_size);
}

static inline int
hc_read_batch_get (hc_read_batch_t *batch, hc_buf_t key, hc_buf_t *out) {
  return kv_read_batch_get(batch, key.buffer, key.len, &out->buffer, &out->len);
}

static inline int
hc_read_batch_flush (hc_read_batch_t *batch) {
  return kv_read_batch_flush(batch);
}

static inline void
hc_read_batch_destroy (hc_read_batch_t *batch) {
  kv_read_batch_destroy(batch);
}

#ifdef __cplusplus
}
#endif

#endif // HC_STORAGE_H
