#ifndef HC_HASHES_H
#define HC_HASHES_H

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
} hc_hashes_t;

void
hc_hashes_init (hc_hashes_t *hashes);

void
hc_hashes_replicate (hc_hash_t out, const hc_hashes_t *hashes, int is_initiator, const hc_hash_t key, const hc_hash_t handshake_hash);

// Writes 112 bytes: tree(32) || manifest_hash(32) || tree_hash(32) || length(8 LE) || fork(8 LE)
void
hc_hashes_tree_signable (uint8_t out[112], const hc_hashes_t *hashes, const hc_hash_t manifest_hash, const hc_hash_t tree_hash, uint64_t length, uint64_t fork);

int
hc_hashes_manifest (hc_hash_t out, const hc_manifest_t *manifest);

#ifdef __cplusplus
}
#endif

#endif // HC_HASHES_H
