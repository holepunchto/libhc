#include <stdlib.h>

#include <compact.h>
#include <sodium.h>

#include "hc/schema.h"

// MANIFEST_CAP = BLAKE2b(BLAKE2b("hypercore") || 0x03)
// This is caps.MANIFEST from hypercore/lib/caps.js (namespace index 3).
static void
manifest_cap (hc_hash_t out) {
  uint8_t ns[33];
  crypto_generichash(ns, 32, (const uint8_t *) "hypercore", 9, NULL, 0);
  ns[32] = 3;
  crypto_generichash(out, 32, ns, 33, NULL, 0);
}

int
hc_manifest_hash (hc_hash_t out, const hc_manifest_t *manifest) {
  compact_state_t state = {0, 0, NULL};
  if (hc_manifest_preencode(&state, manifest) < 0) return -1;

  uint8_t *buf = malloc(state.end);
  if (buf == NULL) return -1;

  state = (compact_state_t){0, state.end, buf};
  if (hc_manifest_encode(&state, manifest) < 0) {
    free(buf);
    return -1;
  }

  hc_hash_t cap;
  manifest_cap(cap);

  crypto_generichash_state hash_state;
  crypto_generichash_init(&hash_state, NULL, 0, HC_CRYPTO_HASH_SIZE);
  crypto_generichash_update(&hash_state, cap, HC_CRYPTO_HASH_SIZE);
  crypto_generichash_update(&hash_state, buf, state.start);
  crypto_generichash_final(&hash_state, out, HC_CRYPTO_HASH_SIZE);

  free(buf);
  return 0;
}
