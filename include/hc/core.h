#ifndef HC_CORE_H
#define HC_CORE_H

#include <stddef.h>
#include <stdint.h>

#include "merkle_tree.h"
#include "storage.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct hc_core_s hc_core_t;

struct hc_core_s {
  hc_storage_core_t storage;
  hc_merkle_tree_node_t *roots;
  size_t roots_len;
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

#ifdef __cplusplus
}
#endif

#endif // HC_CORE_H
