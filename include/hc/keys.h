#ifndef HC_KEYS_H
#define HC_KEYS_H

#include <stdint.h>

#include "buffer.h"
#include "crypto.h"

#ifdef __cplusplus
extern "C" {
#endif

// Top-level namespaces
#define HC_KEY_TL_HEAD          0
#define HC_KEY_TL_CORE_BY_DKEY  1
#define HC_KEY_TL_CORE_BY_ALIAS 2
#define HC_KEY_TL_CORE          3
#define HC_KEY_TL_DATA          4

// TL_CORE subtypes
#define HC_KEY_CORE_AUTH     0
#define HC_KEY_CORE_SESSIONS 1

// TL_DATA subtypes
#define HC_KEY_DATA_HEAD       0
#define HC_KEY_DATA_DEPENDENCY 1
#define HC_KEY_DATA_HINTS      2
#define HC_KEY_DATA_BLOCK      3
#define HC_KEY_DATA_TREE       4
#define HC_KEY_DATA_BITFIELD   5
#define HC_KEY_DATA_USER_DATA  6
#define HC_KEY_DATA_LOCAL      7
#define HC_KEY_DATA_MARK       8

// Carrier for stack-allocated keys with inline backing storage. Pass
// &small_key->buf (or the equivalent (hc_buf_t *) cast) to any hc_buf_t API.
// Size of 64 is enough for any of the lexkey-encoded keys we generate,
// including store-level keys that carry a 32-byte discovery key.
typedef struct {
  hc_buf_t buf;
  uint8_t data[64];
} hc_small_key_t;

typedef struct {
  hc_small_key_t *buffers;
  size_t length;
  size_t capacity;
} hc_small_key_array_t;

// All encoders use lexkey UINTs (order-preserving big-endian) so encoded
// keys sort lexicographically in the same order as the logical (tl, ptr,
// subtype, index) tuple.

void
hc_key_store_head (hc_small_key_t *key);

void
hc_key_store_core (hc_small_key_t *key, const hc_hash_t discovery_key);

void
hc_key_core_core (hc_small_key_t *key, uint64_t core_ptr);

void
hc_key_core_data (hc_small_key_t *key, uint64_t data_ptr);

void
hc_key_core_auth (hc_small_key_t *key, uint64_t core_ptr);

void
hc_key_core_sessions (hc_small_key_t *key, uint64_t core_ptr);

void
hc_key_core_head (hc_small_key_t *key, uint64_t data_ptr);

void
hc_key_core_dependency (hc_small_key_t *key, uint64_t data_ptr);

void
hc_key_core_hints (hc_small_key_t *key, uint64_t data_ptr);

void
hc_key_core_block (hc_small_key_t *key, uint64_t data_ptr, uint64_t index);

void
hc_key_core_tree (hc_small_key_t *key, uint64_t data_ptr, uint64_t index);

void
hc_key_core_mark (hc_small_key_t *key, uint64_t data_ptr, uint64_t index);

#ifdef __cplusplus
}
#endif

#endif // HC_KEYS_H
