#ifndef HC_STORAGE_H
#define HC_STORAGE_H

#include <stdint.h>

#include "merkle_tree.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
  int _placeholder;
} hc_storage_t;

int
hc_storage_init (hc_storage_t *storage);

int
hc_storage_batch_get_tree_node (hc_storage_t *storage, uint64_t index, hc_merkle_tree_node_t *node);

int
hc_storage_batch_put_tree_node (hc_storage_t *storage, uint64_t index, const hc_merkle_tree_node_t *node);

int
hc_storage_batch_delete_tree_node (hc_storage_t *storage, uint64_t index);

#ifdef __cplusplus
}
#endif

#endif // HC_STORAGE_H
