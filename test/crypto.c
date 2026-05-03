#include <assert.h>
#include <string.h>

#include <sodium.h>

#include "hc/crypto.h"

int
main () {
  assert(sodium_init() >= 0);

  // keypair: random
  hc_crypto_keypair_t kp;
  hc_crypto_keypair(&kp, NULL);
  assert(hc_crypto_validate_keypair(&kp));

  // keypair: deterministic from seed
  uint8_t seed[HC_CRYPTO_KEY_SIZE];
  memset(seed, 0x42, sizeof(seed));
  hc_crypto_keypair_t kp2;
  hc_crypto_keypair(&kp2, seed);
  hc_crypto_keypair_t kp3;
  hc_crypto_keypair(&kp3, seed);
  assert(memcmp(kp2.public_key, kp3.public_key, HC_CRYPTO_KEY_SIZE) == 0);
  assert(memcmp(kp2.secret_key, kp3.secret_key, HC_CRYPTO_SECRET_KEY_SIZE) == 0);
  assert(hc_crypto_validate_keypair(&kp2));

  // validate_keypair: tampered public key fails
  hc_crypto_keypair_t bad = kp;
  bad.public_key[0] ^= 0xff;
  assert(!hc_crypto_validate_keypair(&bad));

  // sign + verify
  uint8_t msg[] = "hello hypercore";
  uint8_t sig[HC_CRYPTO_SIGN_SIZE];
  hc_crypto_sign(sig, msg, sizeof(msg), kp.secret_key);
  assert(hc_crypto_verify(sig, msg, sizeof(msg), kp.public_key));

  // verify fails on tampered message
  uint8_t bad_msg[] = "hello hypercoRe";
  assert(!hc_crypto_verify(sig, bad_msg, sizeof(bad_msg), kp.public_key));

  // data hash: deterministic
  uint8_t h1[HC_CRYPTO_HASH_SIZE];
  uint8_t h2[HC_CRYPTO_HASH_SIZE];
  hc_crypto_data(h1, msg, sizeof(msg));
  hc_crypto_data(h2, msg, sizeof(msg));
  assert(memcmp(h1, h2, HC_CRYPTO_HASH_SIZE) == 0);

  // data hash: different inputs produce different hashes
  uint8_t h3[HC_CRYPTO_HASH_SIZE];
  hc_crypto_data(h3, bad_msg, sizeof(bad_msg));
  assert(memcmp(h1, h3, HC_CRYPTO_HASH_SIZE) != 0);

  // parent hash: commutative (argument order shouldn't matter)
  hc_crypto_node_t node_a = {.index = 0, .size = 1};
  hc_crypto_node_t node_b = {.index = 2, .size = 1};
  memset(node_a.hash, 0xaa, HC_CRYPTO_HASH_SIZE);
  memset(node_b.hash, 0xbb, HC_CRYPTO_HASH_SIZE);

  uint8_t p1[HC_CRYPTO_HASH_SIZE];
  uint8_t p2[HC_CRYPTO_HASH_SIZE];
  hc_crypto_parent(p1, &node_a, &node_b);
  hc_crypto_parent(p2, &node_b, &node_a);
  assert(memcmp(p1, p2, HC_CRYPTO_HASH_SIZE) == 0);

  // tree hash: deterministic
  hc_crypto_node_t roots[2];
  roots[0] = node_a;
  roots[1] = node_b;
  uint8_t t1[HC_CRYPTO_HASH_SIZE];
  uint8_t t2[HC_CRYPTO_HASH_SIZE];
  hc_crypto_tree(t1, roots, 2);
  hc_crypto_tree(t2, roots, 2);
  assert(memcmp(t1, t2, HC_CRYPTO_HASH_SIZE) == 0);

  // hash: deterministic
  uint8_t gh1[HC_CRYPTO_HASH_SIZE];
  uint8_t gh2[HC_CRYPTO_HASH_SIZE];
  hc_crypto_hash(gh1, msg, sizeof(msg));
  hc_crypto_hash(gh2, msg, sizeof(msg));
  assert(memcmp(gh1, gh2, HC_CRYPTO_HASH_SIZE) == 0);

  // discovery_key: deterministic, different from key
  uint8_t key[HC_CRYPTO_KEY_SIZE];
  memset(key, 0x11, sizeof(key));
  uint8_t dk1[HC_CRYPTO_HASH_SIZE];
  uint8_t dk2[HC_CRYPTO_HASH_SIZE];
  hc_crypto_discovery_key(dk1, key);
  hc_crypto_discovery_key(dk2, key);
  assert(memcmp(dk1, dk2, HC_CRYPTO_HASH_SIZE) == 0);
  assert(memcmp(dk1, key, HC_CRYPTO_HASH_SIZE) != 0);

  // random_bytes: doesn't crash, produces non-zero bytes (probabilistically)
  uint8_t rnd[32];
  hc_crypto_random_bytes(rnd, sizeof(rnd));
  uint8_t zeros[32] = {0};
  assert(memcmp(rnd, zeros, 32) != 0);

  return 0;
}
