#ifndef HC_MANIFEST_H
#define HC_MANIFEST_H

#include <hc_schema.h>

#include "crypto.h"

#ifdef __cplusplus
extern "C" {
#endif

// hash/signature funcs encode as a uint; the generated hc_manifest_t / hc_signer_t
// carry them as uint64_t. These constants name the values callers assign.
typedef enum {
  HC_HASH_FUNC_BLAKE2B = 0,
} hc_hash_func_t;

typedef enum {
  HC_SIGNATURE_FUNC_ED25519 = 0,
} hc_signature_func_t;

// Initialise a v1 single-signer ed25519 manifest using the keypair's public
// key and the default namespace. Heap-allocates the signers array; caller
// must call hc_manifest_destroy.
int
hc_manifest_init_single_signer (hc_manifest_t *manifest, const hc_crypto_keypair_t *keypair);

#ifdef __cplusplus
}
#endif

#endif // HC_MANIFEST_H
