#include "hc/storage.h"

int
hc_storage_init (hc_storage_t *storage) {
  (void) storage;
  return 0;
}

int
hc_storage_batch_get_tree_node (hc_storage_t *storage, uint64_t index, hc_merkle_tree_node_t *node) {
  (void) storage;
  (void) index;
  (void) node;
  return 0;
}

int
hc_storage_batch_put_tree_node (hc_storage_t *storage, uint64_t index, const hc_merkle_tree_node_t *node) {
  (void) storage;
  (void) index;
  (void) node;
  return 0;
}

int
hc_storage_batch_delete_tree_node (hc_storage_t *storage, uint64_t index) {
  (void) storage;
  (void) index;
  return 0;
}
