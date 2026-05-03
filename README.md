# libhc

C library for hypercore. Implements the core cryptographic primitives used by the hypercore protocol.

> AI assisted

## API

See `include/hc.h` and the sub-headers it includes.

### Crypto (`include/hc/crypto.h`)

```c
// sodium_init() must be called before using these functions.

void hc_crypto_keypair(hc_crypto_keypair_t *out, const uint8_t seed[HC_CRYPTO_KEY_SIZE]);
bool hc_crypto_validate_keypair(const hc_crypto_keypair_t *kp);

void hc_crypto_sign(uint8_t sig[HC_CRYPTO_SIGN_SIZE], const uint8_t *msg, size_t msg_len, const uint8_t sk[HC_CRYPTO_SECRET_KEY_SIZE]);
bool hc_crypto_verify(const uint8_t sig[HC_CRYPTO_SIGN_SIZE], const uint8_t *msg, size_t msg_len, const uint8_t pk[HC_CRYPTO_KEY_SIZE]);

void hc_crypto_data(uint8_t out[HC_CRYPTO_HASH_SIZE], const uint8_t *data, size_t len);
void hc_crypto_parent(uint8_t out[HC_CRYPTO_HASH_SIZE], const hc_crypto_node_t *a, const hc_crypto_node_t *b);
void hc_crypto_tree(uint8_t out[HC_CRYPTO_HASH_SIZE], const hc_crypto_node_t *roots, size_t count);

void hc_crypto_hash(uint8_t out[HC_CRYPTO_HASH_SIZE], const uint8_t *data, size_t len);
void hc_crypto_discovery_key(uint8_t out[HC_CRYPTO_HASH_SIZE], const uint8_t key[HC_CRYPTO_KEY_SIZE]);

void hc_crypto_random_bytes(uint8_t *buf, size_t len);
```

Hash functions are wire-compatible with [hypercore-crypto](https://github.com/holepunchto/hypercore-crypto).

## Building

```sh
npm install
bare-make generate --debug
bare-make build
bare-make test
```

## License

Apache-2.0
