#include <stdlib.h>
#include <string.h>

#include <compact.h>

#include "encodings/keys.h"
#include "hc/storage.h"

#define HC_STORAGE_CORE_INITIAL_OPS 8

static int
grow_read_ops (hc_storage_core_read_t *read) {
  size_t new_capacity = read->capacity == 0 ? HC_STORAGE_CORE_INITIAL_OPS : read->capacity * 2;
  hc_storage_core_read_op_t *new_ops = realloc(read->ops, new_capacity * sizeof(hc_storage_core_read_op_t));
  if (new_ops == NULL) return -1;
  read->ops = new_ops;
  read->capacity = new_capacity;
  return 0;
}

static int
grow_write_ops (hc_storage_core_write_t *write) {
  size_t new_capacity = write->capacity == 0 ? HC_STORAGE_CORE_INITIAL_OPS : write->capacity * 2;
  hc_storage_core_write_op_t *new_ops = realloc(write->ops, new_capacity * sizeof(hc_storage_core_write_op_t));
  if (new_ops == NULL) return -1;
  write->ops = new_ops;
  write->capacity = new_capacity;
  return 0;
}

int
hc_storage_core_init (hc_storage_core_t *storage, uint64_t core_ptr, uint64_t data_ptr) {
  kv_init(&storage->kv);
  storage->core_ptr = core_ptr;
  storage->data_ptr = data_ptr;
  return 0;
}

void
hc_storage_core_destroy (hc_storage_core_t *storage) {
  kv_destroy(&storage->kv);
}

int
hc_storage_core_read (hc_storage_core_t *storage, hc_storage_core_read_t *read, int suggested_size) {
  if (suggested_size < 0) suggested_size = HC_STORAGE_CORE_INITIAL_OPS;
  read->storage = storage;
  read->ops = NULL;
  read->len = 0;
  read->capacity = 0;
  if (suggested_size > 0) {
    read->ops = malloc(suggested_size * sizeof(hc_storage_core_read_op_t));
    if (read->ops == NULL) return -1;
    read->capacity = suggested_size;
  }
  return 0;
}

int
hc_storage_core_write (hc_storage_core_t *storage, hc_storage_core_write_t *write, int suggested_size) {
  if (suggested_size < 0) suggested_size = HC_STORAGE_CORE_INITIAL_OPS;
  write->storage = storage;
  write->ops = NULL;
  write->len = 0;
  write->capacity = 0;
  if (suggested_size > 0) {
    write->ops = malloc(suggested_size * sizeof(hc_storage_core_write_op_t));
    if (write->ops == NULL) return -1;
    write->capacity = suggested_size;
  }
  return 0;
}

int
hc_storage_core_read_flush (hc_storage_core_read_t *read) {
  uint8_t key_buf[HC_KEY_MAX_SIZE];
  for (size_t i = 0; i < read->len; i++) {
    hc_storage_core_read_op_t *op = &read->ops[i];
    compact_state_t state = {0, HC_KEY_MAX_SIZE, key_buf};
    hc_key_encode_core_tree(&state, read->storage->data_ptr, op->index);

    const uint8_t *val;
    size_t val_len;
    int rc = kv_get(&read->storage->kv, key_buf, state.start, &val, &val_len);
    // TODO: signal not-found to the caller; for now leave op->out untouched.
    // TODO: replace raw memcpy with proper tree-node value decoding.
    if (rc == 0 && val_len == sizeof(hc_merkle_tree_node_t)) {
      memcpy(op->out, val, sizeof(hc_merkle_tree_node_t));
    }
  }
  free(read->ops);
  read->ops = NULL;
  read->len = 0;
  read->capacity = 0;
  return 0;
}

int
hc_storage_core_write_flush (hc_storage_core_write_t *write) {
  uint8_t key_buf[HC_KEY_MAX_SIZE];
  for (size_t i = 0; i < write->len; i++) {
    hc_storage_core_write_op_t *op = &write->ops[i];
    compact_state_t state = {0, HC_KEY_MAX_SIZE, key_buf};
    hc_key_encode_core_tree(&state, write->storage->data_ptr, op->index);

    if (op->type == HC_STORAGE_CORE_WRITE_PUT_TREE_NODE) {
      // TODO: replace raw memcpy with proper tree-node value encoding.
      kv_put(&write->storage->kv, key_buf, state.start, (const uint8_t *) op->payload.tree_node, sizeof(hc_merkle_tree_node_t));
    } else { // HC_STORAGE_CORE_WRITE_DELETE_TREE_NODE
      kv_del(&write->storage->kv, key_buf, state.start);
    }
  }
  free(write->ops);
  write->ops = NULL;
  write->len = 0;
  write->capacity = 0;
  return 0;
}

int
hc_storage_core_read_get_tree_node (hc_storage_core_read_t *read, uint64_t index, hc_merkle_tree_node_t *node) {
  if (read->len == read->capacity && grow_read_ops(read) < 0) return -1;
  read->ops[read->len].index = index;
  read->ops[read->len].out = node;
  read->len++;
  return 0;
}

int
hc_storage_core_write_put_tree_node (hc_storage_core_write_t *write, uint64_t index, const hc_merkle_tree_node_t *node) {
  if (write->len == write->capacity && grow_write_ops(write) < 0) return -1;
  write->ops[write->len].type = HC_STORAGE_CORE_WRITE_PUT_TREE_NODE;
  write->ops[write->len].index = index;
  write->ops[write->len].payload.tree_node = node;
  write->len++;
  return 0;
}

int
hc_storage_core_write_delete_tree_node (hc_storage_core_write_t *write, uint64_t index) {
  if (write->len == write->capacity && grow_write_ops(write) < 0) return -1;
  write->ops[write->len].type = HC_STORAGE_CORE_WRITE_DELETE_TREE_NODE;
  write->ops[write->len].index = index;
  write->len++;
  return 0;
}
