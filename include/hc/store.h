#ifndef HC_STORE_H
#define HC_STORE_H

#include <stdint.h>

#include <kv.h>

#include "crypto.h"

struct hc_core_s;

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
  uint64_t cores;
  uint64_t datas;
  uint64_t groups;
  uint8_t has_seed;
  hc_hash_t seed;
  uint8_t has_default_discovery_key;
  hc_hash_t default_discovery_key;
} hc_store_head_t;

typedef struct {
  kv_t kv;
  hc_store_head_t head;
} hc_store_t;

int
hc_store_init (hc_store_t *store);

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
