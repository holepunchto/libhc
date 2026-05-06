#include <stdlib.h>
#include <string.h>

#include <compact.h>

#include "encodings/keys.h"
#include "encodings/schema.h"
#include "hc/storage.h"

#define HC_STORAGE_CORE_INITIAL_OPS 8

struct hc_storage_core_read_op_s {
  uint8_t key[HC_KEY_MAX_SIZE];
  size_t key_len;
  hc_merkle_tree_node_t *out;
  uint8_t *val; // populated by kv_get; we own and must free
  size_t val_len;
};

static int
grow_read_ops (hc_storage_core_read_t *read) {
  size_t new_capacity = read->capacity == 0 ? HC_STORAGE_CORE_INITIAL_OPS : read->capacity * 2;
  hc_storage_core_read_op_t **new_ops = realloc(read->ops, new_capacity * sizeof(*new_ops));
  if (new_ops == NULL) return -1;
  read->ops = new_ops;
  read->capacity = new_capacity;
  return 0;
}

static int
grow_write_bufs (hc_storage_core_write_t *write) {
  size_t new_capacity = write->capacity == 0 ? HC_STORAGE_CORE_INITIAL_OPS : write->capacity * 2;
  uint8_t **new_bufs = realloc(write->bufs, new_capacity * sizeof(*new_bufs));
  if (new_bufs == NULL) return -1;
  write->bufs = new_bufs;
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
hc_storage_core_read (hc_storage_core_t *storage, hc_storage_core_read_t *read, size_t suggested_size) {
  read->storage = storage;
  read->ops = NULL;
  read->len = 0;
  read->capacity = 0;
  kv_read_batch_init(&read->batch, &storage->kv, suggested_size);
  if (suggested_size > 0) {
    read->ops = malloc(suggested_size * sizeof(*read->ops));
    if (read->ops == NULL) {
      kv_read_batch_destroy(&read->batch);
      return -1;
    }
    read->capacity = suggested_size;
  }
  return 0;
}

int
hc_storage_core_write (hc_storage_core_t *storage, hc_storage_core_write_t *write, size_t suggested_size) {
  write->storage = storage;
  write->bufs = NULL;
  write->len = 0;
  write->capacity = 0;
  kv_write_batch_init(&write->batch, &storage->kv, suggested_size);
  if (suggested_size > 0) {
    write->bufs = malloc(suggested_size * sizeof(*write->bufs));
    if (write->bufs == NULL) {
      kv_write_batch_destroy(&write->batch);
      return -1;
    }
    write->capacity = suggested_size;
  }
  return 0;
}

int
hc_storage_core_read_flush (hc_storage_core_read_t *read) {
  kv_read_batch_flush(&read->batch);

  // TODO: signal not-found to the caller; for now leave op->out untouched.
  for (size_t i = 0; i < read->len; i++) {
    hc_storage_core_read_op_t *op = read->ops[i];
    if (op->val != NULL) {
      compact_state_t val_state = {0, op->val_len, op->val};
      hc_schema_decode_tree_node(&val_state, op->out);
      free(op->val);
    }
    free(op);
  }

  kv_read_batch_destroy(&read->batch);
  free(read->ops);
  read->ops = NULL;
  read->len = 0;
  read->capacity = 0;
  return 0;
}

int
hc_storage_core_write_flush (hc_storage_core_write_t *write) {
  int rc = kv_write_batch_flush(&write->batch);

  for (size_t i = 0; i < write->len; i++) free(write->bufs[i]);

  kv_write_batch_destroy(&write->batch);
  free(write->bufs);
  write->bufs = NULL;
  write->len = 0;
  write->capacity = 0;
  return rc;
}

int
hc_storage_core_read_get_tree_node (hc_storage_core_read_t *read, uint64_t index, hc_merkle_tree_node_t *node) {
  if (read->len == read->capacity && grow_read_ops(read) < 0) return -1;

  hc_storage_core_read_op_t *op = malloc(sizeof(*op));
  if (op == NULL) return -1;

  compact_state_t key_state = {0, HC_KEY_MAX_SIZE, op->key};
  hc_key_encode_core_tree(&key_state, read->storage->data_ptr, index);
  op->key_len = key_state.start;
  op->out = node;
  op->val = NULL;
  op->val_len = 0;

  read->ops[read->len++] = op;

  return kv_read_batch_get(&read->batch, op->key, op->key_len, &op->val, &op->val_len);
}

int
hc_storage_core_write_put_tree_node (hc_storage_core_write_t *write, uint64_t index, const hc_merkle_tree_node_t *node) {
  if (write->len == write->capacity && grow_write_bufs(write) < 0) return -1;

  uint8_t key_tmp[HC_KEY_MAX_SIZE];
  compact_state_t key_state = {0, HC_KEY_MAX_SIZE, key_tmp};
  hc_key_encode_core_tree(&key_state, write->storage->data_ptr, index);

  compact_state_t val_state = {0, 0, NULL};
  hc_schema_preencode_tree_node(&val_state, node);

  uint8_t *buf = malloc(key_state.start + val_state.end);
  if (buf == NULL) return -1;

  memcpy(buf, key_tmp, key_state.start);
  val_state.buffer = buf + key_state.start;
  val_state.start = 0;
  hc_schema_encode_tree_node(&val_state, node);

  write->bufs[write->len++] = buf;

  return kv_write_batch_put(&write->batch, buf, key_state.start, buf + key_state.start, val_state.start);
}

int
hc_storage_core_write_delete_tree_node (hc_storage_core_write_t *write, uint64_t index) {
  if (write->len == write->capacity && grow_write_bufs(write) < 0) return -1;

  uint8_t key_tmp[HC_KEY_MAX_SIZE];
  compact_state_t key_state = {0, HC_KEY_MAX_SIZE, key_tmp};
  hc_key_encode_core_tree(&key_state, write->storage->data_ptr, index);

  uint8_t *buf = malloc(key_state.start);
  if (buf == NULL) return -1;
  memcpy(buf, key_tmp, key_state.start);

  write->bufs[write->len++] = buf;

  return kv_write_batch_del(&write->batch, buf, key_state.start);
}
