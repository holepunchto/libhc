#ifndef HC_CORE_H
#define HC_CORE_H

#include <stddef.h>
#include <stdint.h>

#include "db.h"
#include "merkle_tree.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct hc_core_s hc_core_t;

struct hc_core_s {
  hc__db_core_t db;
  hc_merkle_tree_node_array_t roots;
  uint64_t length;
  uint64_t byte_length;
};

int
hc_core_init (hc_core_t *core, uint64_t core_ptr, uint64_t data_ptr);

void
hc_core_destroy (hc_core_t *core);

// Roll the in-memory state to the given length. Loads the root nodes from
// storage and recomputes byte_length. Stubbed for now.
int
hc_core_checkout (hc_core_t *core, uint64_t length);

typedef struct hc_core_upgrade_s hc_core_upgrade_t;

struct hc_core_upgrade_s {
  hc_core_t *core;
  hc_merkle_tree_node_array_t roots;
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
