#ifndef HC_STORAGE_H
#define HC_STORAGE_H

#include <stddef.h>
#include <stdint.h>

#include <kv.h>

#include "merkle_tree.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct hc_storage_s hc_storage_t;
typedef struct hc_storage_read_s hc_storage_read_t;
typedef struct hc_storage_write_s hc_storage_write_t;

typedef struct {
  uint64_t index;
  hc_merkle_tree_node_t *out;
} hc_storage_read_op_t;

typedef enum {
  HC_STORAGE_WRITE_PUT_TREE_NODE,
  HC_STORAGE_WRITE_DELETE_TREE_NODE,
} hc_storage_write_op_type_t;

typedef struct {
  hc_storage_write_op_type_t type;
  uint64_t index;
  union {
    const hc_merkle_tree_node_t *tree_node;
  } payload;
} hc_storage_write_op_t;

struct hc_storage_s {
  kv_t kv;
};

struct hc_storage_read_s {
  hc_storage_t *storage;
  hc_storage_read_op_t *ops;
  size_t len;
  size_t capacity;
};

struct hc_storage_write_s {
  hc_storage_t *storage;
  hc_storage_write_op_t *ops;
  size_t len;
  size_t capacity;
};

int
hc_storage_init (hc_storage_t *storage);

void
hc_storage_destroy (hc_storage_t *storage);

// Pass suggested_size = -1 for the default initial capacity.
int
hc_storage_read (hc_storage_t *storage, hc_storage_read_t *read, int suggested_size);

int
hc_storage_write (hc_storage_t *storage, hc_storage_write_t *write, int suggested_size);

// Commits all buffered ops. For reads, the result buffers passed to
// hc_storage_read_get_tree_node are only valid after this returns.
int
hc_storage_read_flush (hc_storage_read_t *read);

int
hc_storage_write_flush (hc_storage_write_t *write);

int
hc_storage_read_get_tree_node (hc_storage_read_t *read, uint64_t index, hc_merkle_tree_node_t *node);

// node must remain valid until hc_storage_write_flush is called.
int
hc_storage_write_put_tree_node (hc_storage_write_t *write, uint64_t index, const hc_merkle_tree_node_t *node);

int
hc_storage_write_delete_tree_node (hc_storage_write_t *write, uint64_t index);

#ifdef __cplusplus
}
#endif

#endif // HC_STORAGE_H
