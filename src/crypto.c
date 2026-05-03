#include <string.h>

#include <sodium.h>

#include "hc/crypto.h"

static const uint8_t LEAF_TYPE = 0x00;
static const uint8_t PARENT_TYPE = 0x01;
static const uint8_t ROOT_TYPE = 0x02;

static void
encode_uint64 (uint8_t buf[8], uint64_t v) {
  buf[0] = (uint8_t) (v);
  buf[1] = (uint8_t) (v >> 8);
  buf[2] = (uint8_t) (v >> 16);
  buf[3] = (uint8_t) (v >> 24);
  buf[4] = (uint8_t) (v >> 32);
  buf[5] = (uint8_t) (v >> 40);
  buf[6] = (uint8_t) (v >> 48);
  buf[7] = (uint8_t) (v >> 56);
}

void
hc_crypto_keypair (hc_crypto_keypair_t *out, const uint8_t seed[HC_CRYPTO_KEY_SIZE]) {
  if (seed) {
    crypto_sign_seed_keypair(out->public_key, out->secret_key, seed);
  } else {
    crypto_sign_keypair(out->public_key, out->secret_key);
  }
}

bool
hc_crypto_validate_keypair (const hc_crypto_keypair_t *kp) {
  uint8_t derived[HC_CRYPTO_KEY_SIZE];
  crypto_sign_ed25519_sk_to_pk(derived, kp->secret_key);
  return memcmp(derived, kp->public_key, HC_CRYPTO_KEY_SIZE) == 0;
}

void
hc_crypto_sign (uint8_t sig[HC_CRYPTO_SIGN_SIZE], const uint8_t *msg, size_t msg_len, const uint8_t sk[HC_CRYPTO_SECRET_KEY_SIZE]) {
  crypto_sign_detached(sig, NULL, msg, msg_len, sk);
}

bool
hc_crypto_verify (const uint8_t sig[HC_CRYPTO_SIGN_SIZE], const uint8_t *msg, size_t msg_len, const uint8_t pk[HC_CRYPTO_KEY_SIZE]) {
  return crypto_sign_verify_detached(sig, msg, msg_len, pk) == 0;
}

void
hc_crypto_data (uint8_t out[HC_CRYPTO_HASH_SIZE], const uint8_t *data, size_t len) {
  crypto_generichash_state state;
  uint8_t buf[9];
  buf[0] = LEAF_TYPE;
  encode_uint64(buf + 1, (uint64_t) len);
  crypto_generichash_init(&state, NULL, 0, HC_CRYPTO_HASH_SIZE);
  crypto_generichash_update(&state, buf, 9);
  crypto_generichash_update(&state, data, len);
  crypto_generichash_final(&state, out, HC_CRYPTO_HASH_SIZE);
}

void
hc_crypto_parent (uint8_t out[HC_CRYPTO_HASH_SIZE], const hc_crypto_node_t *a, const hc_crypto_node_t *b) {
  const hc_crypto_node_t *lo = a->index < b->index ? a : b;
  const hc_crypto_node_t *hi = a->index < b->index ? b : a;
  crypto_generichash_state state;
  uint8_t buf[9];
  buf[0] = PARENT_TYPE;
  encode_uint64(buf + 1, lo->index);
  crypto_generichash_init(&state, NULL, 0, HC_CRYPTO_HASH_SIZE);
  crypto_generichash_update(&state, buf, 9);
  crypto_generichash_update(&state, lo->hash, HC_CRYPTO_HASH_SIZE);
  encode_uint64(buf, lo->size);
  crypto_generichash_update(&state, buf, 8);
  encode_uint64(buf, hi->index);
  crypto_generichash_update(&state, buf, 8);
  crypto_generichash_update(&state, hi->hash, HC_CRYPTO_HASH_SIZE);
  encode_uint64(buf, hi->size);
  crypto_generichash_update(&state, buf, 8);
  crypto_generichash_final(&state, out, HC_CRYPTO_HASH_SIZE);
}

void
hc_crypto_tree (uint8_t out[HC_CRYPTO_HASH_SIZE], const hc_crypto_node_t *roots, size_t count) {
  crypto_generichash_state state;
  uint8_t buf[8];
  crypto_generichash_init(&state, NULL, 0, HC_CRYPTO_HASH_SIZE);
  crypto_generichash_update(&state, &ROOT_TYPE, 1);
  for (size_t i = 0; i < count; i++) {
    encode_uint64(buf, roots[i].index);
    crypto_generichash_update(&state, buf, 8);
    crypto_generichash_update(&state, roots[i].hash, HC_CRYPTO_HASH_SIZE);
    encode_uint64(buf, roots[i].size);
    crypto_generichash_update(&state, buf, 8);
  }
  crypto_generichash_final(&state, out, HC_CRYPTO_HASH_SIZE);
}

void
hc_crypto_hash (uint8_t out[HC_CRYPTO_HASH_SIZE], const uint8_t *data, size_t len) {
  crypto_generichash(out, HC_CRYPTO_HASH_SIZE, data, len, NULL, 0);
}

void
hc_crypto_discovery_key (uint8_t out[HC_CRYPTO_HASH_SIZE], const uint8_t key[HC_CRYPTO_KEY_SIZE]) {
  static const uint8_t ns[] = "hypercore";
  crypto_generichash(out, HC_CRYPTO_HASH_SIZE, ns, sizeof(ns) - 1, key, HC_CRYPTO_KEY_SIZE);
}

void
hc_crypto_random_bytes (uint8_t *buf, size_t len) {
  randombytes_buf(buf, len);
}
