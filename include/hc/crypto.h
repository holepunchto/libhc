#ifndef HC_CRYPTO_H
#define HC_CRYPTO_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// sodium_init() must be called before using any functions in this header.

#define HC_CRYPTO_KEY_SIZE        32
#define HC_CRYPTO_SECRET_KEY_SIZE 64
#define HC_CRYPTO_SIGN_SIZE       64
#define HC_CRYPTO_HASH_SIZE       32

typedef uint8_t hc_hash_t[HC_CRYPTO_HASH_SIZE];

typedef struct {
  uint8_t public_key[HC_CRYPTO_KEY_SIZE];
  uint8_t secret_key[HC_CRYPTO_SECRET_KEY_SIZE];
} hc_crypto_keypair_t;

typedef struct {
  uint64_t index;
  uint64_t size;
  hc_hash_t hash;
} hc_crypto_node_t;

// Pass seed=NULL for a random keypair.
void
hc_crypto_keypair (hc_crypto_keypair_t *out, const uint8_t seed[HC_CRYPTO_KEY_SIZE]);

bool
hc_crypto_validate_keypair (const hc_crypto_keypair_t *kp);

void
hc_crypto_sign (uint8_t sig[HC_CRYPTO_SIGN_SIZE], const uint8_t *msg, size_t msg_len, const uint8_t sk[HC_CRYPTO_SECRET_KEY_SIZE]);

bool
hc_crypto_verify (const uint8_t sig[HC_CRYPTO_SIGN_SIZE], const uint8_t *msg, size_t msg_len, const uint8_t pk[HC_CRYPTO_KEY_SIZE]);

// BLAKE2b(0x00 | uint64le(len) | data) — leaf hash for the Merkle tree.
void
hc_crypto_data (hc_hash_t out, const uint8_t *data, size_t len);

// BLAKE2b(0x01 | lo.index | lo.hash | lo.size | hi.index | hi.hash | hi.size),
// sorted so the node with the lower index comes first.
void
hc_crypto_parent (hc_hash_t out, const hc_crypto_node_t *a, const hc_crypto_node_t *b);

// BLAKE2b(0x02 | root[0].index | root[0].hash | root[0].size | ...) over all peak nodes.
void
hc_crypto_tree (hc_hash_t out, const hc_crypto_node_t *roots, size_t count);

void
hc_crypto_hash (hc_hash_t out, const uint8_t *data, size_t len);

// Keyed BLAKE2b: key=key, msg="hypercore". Safe to advertise publicly.
void
hc_crypto_discovery_key (hc_hash_t out, const uint8_t key[HC_CRYPTO_KEY_SIZE]);

void
hc_crypto_random_bytes (uint8_t *buf, size_t len);

#ifdef __cplusplus
}
#endif

#endif // HC_CRYPTO_H
