#ifndef HC_STORAGE_H
#define HC_STORAGE_H

#include <stdint.h>

#include <kv.h>

#include "merkle_tree.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct hc_storage_s hc_storage_t;
typedef struct hc_storage_read_s hc_storage_read_t;
typedef struct hc_storage_write_s hc_storage_write_t;

struct hc_storage_s {
  kv_t kv;
};

struct hc_storage_read_s {
  hc_storage_t *storage;
};

struct hc_storage_write_s {
  hc_storage_t *storage;
};

int
hc_storage_init (hc_storage_t *storage);

void
hc_storage_destroy (hc_storage_t *storage);

int
hc_storage_read (hc_storage_t *storage, hc_storage_read_t *read);

int
hc_storage_write (hc_storage_t *storage, hc_storage_write_t *write);

int
hc_storage_read_flush (hc_storage_read_t *read);

int
hc_storage_write_flush (hc_storage_write_t *write);

int
hc_storage_read_get_tree_node (hc_storage_read_t *read, uint64_t index, hc_merkle_tree_node_t *node);

int
hc_storage_write_put_tree_node (hc_storage_write_t *write, uint64_t index, const hc_merkle_tree_node_t *node);

int
hc_storage_write_delete_tree_node (hc_storage_write_t *write, uint64_t index);

#ifdef __cplusplus
}
#endif

#endif // HC_STORAGE_H
