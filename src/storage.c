#include "hc/storage.h"

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
hc_storage_read (hc_storage_t *storage, hc_storage_read_t *read) {
  read->storage = storage;
  return 0;
}

int
hc_storage_write (hc_storage_t *storage, hc_storage_write_t *write) {
  write->storage = storage;
  return 0;
}

int
hc_storage_read_flush (hc_storage_read_t *read) {
  (void) read;
  return 0;
}

int
hc_storage_write_flush (hc_storage_write_t *write) {
  (void) write;
  return 0;
}

int
hc_storage_read_get_tree_node (hc_storage_read_t *read, uint64_t index, hc_merkle_tree_node_t *node) {
  (void) read;
  (void) index;
  (void) node;
  return 0;
}

int
hc_storage_write_put_tree_node (hc_storage_write_t *write, uint64_t index, const hc_merkle_tree_node_t *node) {
  (void) write;
  (void) index;
  (void) node;
  return 0;
}

int
hc_storage_write_delete_tree_node (hc_storage_write_t *write, uint64_t index) {
  (void) write;
  (void) index;
  return 0;
}
