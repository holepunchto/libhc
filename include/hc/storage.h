#ifndef HC_STORAGE_H
#define HC_STORAGE_H

#include <stddef.h>
#include <stdint.h>

#include <kv.h>

#include "merkle_tree.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct hc_storage_core_s hc_storage_core_t;
typedef struct hc_storage_core_read_s hc_storage_core_read_t;
typedef struct hc_storage_core_write_s hc_storage_core_write_t;

// Internal per-op state, opaque to callers.
typedef struct hc_storage_core_read_op_s hc_storage_core_read_op_t;

struct hc_storage_core_s {
  kv_t kv;
  uint64_t core_ptr;
  uint64_t data_ptr;
};

struct hc_storage_core_read_s {
  hc_storage_core_t *storage;
  kv_read_batch_t batch;
  hc_storage_core_read_op_t **ops;
  size_t len;
  size_t capacity;
};

struct hc_storage_core_write_s {
  hc_storage_core_t *storage;
  kv_write_batch_t batch;
  uint8_t **bufs; // per-op heap buffers we own; kv batch points into them
  size_t len;
  size_t capacity;
};

int
hc_storage_core_init (hc_storage_core_t *storage, uint64_t core_ptr, uint64_t data_ptr);

void
hc_storage_core_destroy (hc_storage_core_t *storage);

int
hc_storage_core_read (hc_storage_core_t *storage, hc_storage_core_read_t *read, size_t suggested_size);

int
hc_storage_core_write (hc_storage_core_t *storage, hc_storage_core_write_t *write, size_t suggested_size);

// Commits all queued ops and tears down the batch. For reads, the result
// buffers passed to hc_storage_core_read_get_tree_node are only valid after
// this returns.
int
hc_storage_core_read_flush (hc_storage_core_read_t *read);

int
hc_storage_core_write_flush (hc_storage_core_write_t *write);

int
hc_storage_core_read_get_tree_node (hc_storage_core_read_t *read, uint64_t index, hc_merkle_tree_node_t *node);

int
hc_storage_core_write_put_tree_node (hc_storage_core_write_t *write, uint64_t index, const hc_merkle_tree_node_t *node);

int
hc_storage_core_write_delete_tree_node (hc_storage_core_write_t *write, uint64_t index);

#ifdef __cplusplus
}
#endif

#endif // HC_STORAGE_H
