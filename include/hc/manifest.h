#ifndef HC_MANIFEST_H
#define HC_MANIFEST_H

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

#include "array.h"
#include "buffer.h"
#include "crypto.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
  HC_HASH_FUNC_BLAKE2B = 0,
} hc_hash_func_t;

typedef enum {
  HC_SIGNATURE_FUNC_ED25519 = 0,
} hc_signature_func_t;

typedef struct {
  hc_signature_func_t signature;
  hc_hash_t namespace;
  uint8_t public_key[HC_CRYPTO_KEY_SIZE];
} hc_signer_t;

typedef HC__ARRAY(hc_signer_t) hc_signer_array_t;

typedef HC__ARRAY(hc_hash_t) hc_hash_array_t;

typedef struct {
  hc_hash_t hash;
  uint64_t length;
} hc_prologue_t;

// All pointer fields are caller-owned after decode: free signers.buffers,
// prologue, linked.buffers, and user_data.buffer when done.
typedef struct {
  uint32_t version;
  hc_hash_func_t hash;
  bool allow_patch;
  uint32_t quorum;
  hc_signer_array_t signers;
  hc_prologue_t *prologue;
  hc_hash_array_t linked;
  hc_buf_t user_data;
} hc_manifest_t;

static inline void
hc_manifest_destroy (hc_manifest_t *manifest) {
  hc__array_destroy(&manifest->signers);
  free(manifest->prologue);
  manifest->prologue = NULL;
  hc__array_destroy(&manifest->linked);
  free(manifest->user_data.buffer);
  manifest->user_data.buffer = NULL;
  manifest->user_data.len = 0;
}

// Initialise a v1 single-signer ed25519 manifest using the keypair's public
// key and the default namespace. Heap-allocates the signers array; caller
// must call hc_manifest_destroy.
int
hc_manifest_init_single_signer (hc_manifest_t *manifest, const hc_crypto_keypair_t *keypair);

#ifdef __cplusplus
}
#endif

#endif // HC_MANIFEST_H
