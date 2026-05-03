#include <stdlib.h>

#include "hc/storage.h"

#define HC_STORAGE_INITIAL_OPS 8

static int
grow_read_ops (hc_storage_read_t *read) {
  size_t new_capacity = read->capacity == 0 ? HC_STORAGE_INITIAL_OPS : read->capacity * 2;
  hc_storage_read_op_t *new_ops = realloc(read->ops, new_capacity * sizeof(hc_storage_read_op_t));
  if (new_ops == NULL) return -1;
  read->ops = new_ops;
  read->capacity = new_capacity;
  return 0;
}

static int
grow_write_ops (hc_storage_write_t *write) {
  size_t new_capacity = write->capacity == 0 ? HC_STORAGE_INITIAL_OPS : write->capacity * 2;
  hc_storage_write_op_t *new_ops = realloc(write->ops, new_capacity * sizeof(hc_storage_write_op_t));
  if (new_ops == NULL) return -1;
  write->ops = new_ops;
  write->capacity = new_capacity;
  return 0;
}

int
hc_storage_init (hc_storage_t *storage) {
  kv_init(&storage->kv);
  return 0;
}

void
hc_storage_destroy (hc_storage_t *storage) {
  kv_destroy(&storage->kv);
}

int
hc_storage_read (hc_storage_t *storage, hc_storage_read_t *read, int suggested_size) {
  if (suggested_size < 0) suggested_size = HC_STORAGE_INITIAL_OPS;
  read->storage = storage;
  read->ops = NULL;
  read->len = 0;
  read->capacity = 0;
  if (suggested_size > 0) {
    read->ops = malloc(suggested_size * sizeof(hc_storage_read_op_t));
    if (read->ops == NULL) return -1;
    read->capacity = suggested_size;
  }
  return 0;
}

int
hc_storage_write (hc_storage_t *storage, hc_storage_write_t *write, int suggested_size) {
  if (suggested_size < 0) suggested_size = HC_STORAGE_INITIAL_OPS;
  write->storage = storage;
  write->ops = NULL;
  write->len = 0;
  write->capacity = 0;
  if (suggested_size > 0) {
    write->ops = malloc(suggested_size * sizeof(hc_storage_write_op_t));
    if (write->ops == NULL) return -1;
    write->capacity = suggested_size;
  }
  return 0;
}

int
hc_storage_read_flush (hc_storage_read_t *read) {
  for (size_t i = 0; i < read->len; i++) {
    // TODO: actually fetch from kv and write into read->ops[i].out
    (void) read->ops[i];
  }
  free(read->ops);
  read->ops = NULL;
  read->len = 0;
  read->capacity = 0;
  return 0;
}

int
hc_storage_write_flush (hc_storage_write_t *write) {
  for (size_t i = 0; i < write->len; i++) {
    // TODO: serialize each op's payload into a single flush buffer and apply to kv
    (void) write->ops[i];
  }
  free(write->ops);
  write->ops = NULL;
  write->len = 0;
  write->capacity = 0;
  return 0;
}

int
hc_storage_read_get_tree_node (hc_storage_read_t *read, uint64_t index, hc_merkle_tree_node_t *node) {
  if (read->len == read->capacity && grow_read_ops(read) < 0) return -1;
  read->ops[read->len].index = index;
  read->ops[read->len].out = node;
  read->len++;
  return 0;
}

int
hc_storage_write_put_tree_node (hc_storage_write_t *write, uint64_t index, const hc_merkle_tree_node_t *node) {
  if (write->len == write->capacity && grow_write_ops(write) < 0) return -1;
  write->ops[write->len].type = HC_STORAGE_WRITE_PUT_TREE_NODE;
  write->ops[write->len].index = index;
  write->ops[write->len].payload.tree_node = node;
  write->len++;
  return 0;
}

int
hc_storage_write_delete_tree_node (hc_storage_write_t *write, uint64_t index) {
  if (write->len == write->capacity && grow_write_ops(write) < 0) return -1;
  write->ops[write->len].type = HC_STORAGE_WRITE_DELETE_TREE_NODE;
  write->ops[write->len].index = index;
  write->len++;
  return 0;
}
