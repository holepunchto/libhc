#ifndef HC_MANIFEST_H
#define HC_MANIFEST_H

#include <stdbool.h>
#include <stdint.h>

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

#ifdef __cplusplus
}
#endif

#endif // HC_MANIFEST_H
