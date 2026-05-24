#ifndef HC_CAPS_H
#define HC_CAPS_H

#include <stdint.h>

#include "crypto.h"
#include "manifest.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
  hc_hash_t tree;
  hc_hash_t replicate_initiator;
  hc_hash_t replicate_responder;
  hc_hash_t manifest;
  hc_hash_t default_namespace;
  hc_hash_t default_encryption;
} hc_caps_t;

void
hc_caps_init (hc_caps_t *caps);

void
hc_caps_replicate (hc_hash_t out, const hc_caps_t *caps, int is_initiator, const hc_hash_t key, const hc_hash_t handshake_hash);

// Writes 112 bytes: tree(32) || manifest_hash(32) || tree_hash(32) || length(8 LE) || fork(8 LE)
void
hc_caps_tree_signable (uint8_t out[112], const hc_caps_t *caps, const hc_hash_t manifest_hash, const hc_hash_t tree_hash, uint64_t length, uint64_t fork);

int
hc_manifest_hash (hc_hash_t out, const hc_manifest_t *manifest);

#ifdef __cplusplus
}
#endif

#endif // HC_CAPS_H
