#include <assert.h>
#include <string.h>

#include "hc/core.h"
#include "hc/store.h"

int
main () {
  hc_store_t store;
  assert(hc_store_init(&store) == 0);

  assert(store.head.cores == 0);
  assert(store.head.datas == 0);

  hc_hash_t key = {0};
  hc_hash_t discovery_key = {0};
  memset(key, 0xaa, sizeof(key));
  memset(discovery_key, 0xbb, sizeof(discovery_key));

  // Create a core and verify counters advance.
  hc_core_t core;
  assert(hc_store_create(&store, &core, key, discovery_key) == 0);
  assert(store.head.cores == 1);
  assert(store.head.datas == 1);
  assert(core.db.core_ptr == 0);
  assert(core.db.data_ptr == 0);
  assert(memcmp(core.key, key, sizeof(key)) == 0);
  assert(memcmp(core.discovery_key, discovery_key, sizeof(discovery_key)) == 0);
  hc_core_destroy(&core);

  // Create a second core and verify it gets the next pointers.
  hc_hash_t key2 = {0};
  hc_hash_t discovery_key2 = {0};
  memset(key2, 0xcc, sizeof(key2));
  memset(discovery_key2, 0xdd, sizeof(discovery_key2));

  hc_core_t core2;
  assert(hc_store_create(&store, &core2, key2, discovery_key2) == 0);
  assert(store.head.cores == 2);
  assert(core2.db.core_ptr == 1);
  assert(core2.db.data_ptr == 1);
  hc_core_destroy(&core2);

  // Get the first core back by discovery key.
  hc_core_t got;
  assert(hc_store_get(&store, &got, key, discovery_key) == 0);
  assert(got.db.core_ptr == 0);
  assert(got.db.data_ptr == 0);
  assert(memcmp(got.key, key, sizeof(key)) == 0);
  hc_core_destroy(&got);

  // Get a non-existent core returns error.
  hc_hash_t unknown_dkey = {0};
  memset(unknown_dkey, 0xff, sizeof(unknown_dkey));
  assert(hc_store_get(&store, &got, key, unknown_dkey) < 0);

  hc_store_destroy(&store);
  return 0;
}
