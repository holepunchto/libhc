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
  hc__db_t db;
  hc_store_head_t head;
} hc_store_t;

// Opens the persistent store at `path`. `loop` is forwarded to librocksdb;
// in sync mode it isn't actually serviced but is still required by the
// API. Returns -1 if `path` is NULL or the store cannot be opened.
int
hc_store_init (hc_store_t *store, const char *path, uv_loop_t *loop);

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
