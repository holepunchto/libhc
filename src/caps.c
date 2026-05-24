#include <stdlib.h>
#include <string.h>

#include <compact.h>
#include <sodium.h>

#include "hc/caps.h"
#include "hc/schema.h"

static void
namespace_cap (hc_hash_t out, uint8_t index) {
  uint8_t ns[33];
  crypto_generichash(ns, 32, (const uint8_t *) "hypercore", 9, NULL, 0);
  ns[32] = index;
  crypto_generichash(out, 32, ns, 33, NULL, 0);
}

void
hc_caps_init (hc_caps_t *caps) {
  namespace_cap(caps->tree, 0);
  namespace_cap(caps->replicate_initiator, 1);
  namespace_cap(caps->replicate_responder, 2);
  namespace_cap(caps->manifest, 3);
  namespace_cap(caps->default_namespace, 4);
  namespace_cap(caps->default_encryption, 5);
}

void
hc_caps_replicate (hc_hash_t out, const hc_caps_t *caps, int is_initiator, const hc_hash_t key, const hc_hash_t handshake_hash) {
  uint8_t input[64];
  memcpy(input, is_initiator ? caps->replicate_initiator : caps->replicate_responder, 32);
  memcpy(input + 32, key, 32);
  crypto_generichash(out, 32, input, 64, handshake_hash, 32);
}

static void
write_uint64_le (uint8_t *dst, uint64_t v) {
  dst[0] = (uint8_t) (v);
  dst[1] = (uint8_t) (v >> 8);
  dst[2] = (uint8_t) (v >> 16);
  dst[3] = (uint8_t) (v >> 24);
  dst[4] = (uint8_t) (v >> 32);
  dst[5] = (uint8_t) (v >> 40);
  dst[6] = (uint8_t) (v >> 48);
  dst[7] = (uint8_t) (v >> 56);
}

void
hc_caps_tree_signable (uint8_t out[112], const hc_caps_t *caps, const hc_hash_t manifest_hash, const hc_hash_t tree_hash, uint64_t length, uint64_t fork) {
  memcpy(out, caps->tree, 32);
  memcpy(out + 32, manifest_hash, 32);
  memcpy(out + 64, tree_hash, 32);
  write_uint64_le(out + 96, length);
  write_uint64_le(out + 104, fork);
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

  hc_caps_t caps;
  hc_caps_init(&caps);

  crypto_generichash_state hash_state;
  crypto_generichash_init(&hash_state, NULL, 0, HC_CRYPTO_HASH_SIZE);
  crypto_generichash_update(&hash_state, caps.manifest, HC_CRYPTO_HASH_SIZE);
  crypto_generichash_update(&hash_state, buf, state.start);
  crypto_generichash_final(&hash_state, out, HC_CRYPTO_HASH_SIZE);

  free(buf);
  return 0;
}
