#ifndef HC_STORE_H
#define HC_STORE_H

#include <stdint.h>

#include "crypto.h"
#include "db.h"

struct hc_core_s;

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
  hc__db_store_t db;
  hc_store_head_t head;
} hc_store_t;

// path == NULL opens an in-memory store. Non-NULL paths are reserved for
// the file-backed kv backend (not yet implemented).
int
hc_store_init (hc_store_t *store, const char *path);

void
hc_store_destroy (hc_store_t *store);

// Create a new core, allocating pointers from the store counters.
int
hc_store_create (hc_store_t *store, struct hc_core_s *core, const hc_hash_t key, const hc_hash_t discovery_key);

// Get an existing core by discovery key. key must be supplied by the caller.
int
hc_store_get (hc_store_t *store, struct hc_core_s *core, const hc_hash_t key, const hc_hash_t discovery_key);

#ifdef __cplusplus
}
#endif

#endif // HC_STORE_H
