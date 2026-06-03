#ifndef HC_CORE_H
#define HC_CORE_H

#include <stddef.h>
#include <stdint.h>

#include "crypto.h"
#include "db.h"
#include "manifest.h"
#include "merkle_tree.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct hc_core_s hc_core_t;

struct hc_core_s {
  hc__db_core_t db;
  hc_hash_t key;
  hc_hash_t discovery_key;
  hc_manifest_t *manifest;
  hc_tree_node_array_t roots;
  uint64_t fork;
  uint64_t length;
  uint64_t byte_length;
};

int
hc_core_init (hc_core_t *core, uint64_t core_ptr, uint64_t data_ptr, hc__db_t *db, const hc_hash_t key, const hc_hash_t discovery_key);

void
hc_core_destroy (hc_core_t *core);

int
hc_core_append (hc_core_t *core, const hc_buf_t *buffers, size_t count);

// Roll the in-memory state to the given length. Loads the root nodes from
// storage and recomputes byte_length. Stubbed for now.
int
hc_core_checkout (hc_core_t *core, uint64_t length);

int
hc_core_load (hc_core_t *core);

typedef struct hc_core_upgrade_s hc_core_upgrade_t;

struct hc_core_upgrade_s {
  hc_core_t *core;
  hc_tree_node_array_t roots;
  uint64_t length;
  uint64_t byte_length;
};

int
hc_core_upgrade_init (hc_core_upgrade_t *upgrade, hc_core_t *core);

void
hc_core_upgrade_destroy (hc_core_upgrade_t *upgrade);

// Move upgrade state onto core and reset upgrade.
void
hc_core_commit (hc_core_upgrade_t *upgrade);

#ifdef __cplusplus
}
#endif

#endif // HC_CORE_H
