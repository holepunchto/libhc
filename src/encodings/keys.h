#ifndef HC_ENCODINGS_KEYS_H
#define HC_ENCODINGS_KEYS_H

#include <stddef.h>
#include <stdint.h>

#include <compact.h>
#include <utf.h>

#define HC_KEY_DISCOVERY_KEY_SIZE 32
#define HC_KEY_NAMESPACE_SIZE     32

// Top-level namespaces
#define HC_KEY_TL_HEAD          0
#define HC_KEY_TL_CORE_BY_DKEY  1
#define HC_KEY_TL_CORE_BY_ALIAS 2
#define HC_KEY_TL_CORE          3
#define HC_KEY_TL_DATA          4
#define HC_KEY_TL_END           5

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

// core.local has a hard cap on key length.
#define HC_KEY_CORE_LOCAL_MAX 2048

// Upper bound on the encoded size of any key whose inputs are all scalar
// (uint64) or fixed-size (32-byte hash). Conservative — assumes every uint
// hits the 9-byte maximum. The largest such key today is bitfield
// (TL + ptr + subtype + index + type). Lets callers skip the preencode
// pass and just use a stack buffer of this size.
//
// Variable-length keys (user_data, alias_name, local raw bytes) need to
// add their buffer-encoded sizes on top of this.
#define HC_KEY_MAX_SIZE 64

// Each key has a paired preencode/encode using libcompact's two-pass cursor.
// Both return 0 on success, < 0 on error.

// ---- store namespace ----

int
hc_key_preencode_store_head (compact_state_t *state);
int
hc_key_encode_store_head (compact_state_t *state);

int
hc_key_preencode_store_core (compact_state_t *state, const uint8_t discovery_key[HC_KEY_DISCOVERY_KEY_SIZE]);
int
hc_key_encode_store_core (compact_state_t *state, const uint8_t discovery_key[HC_KEY_DISCOVERY_KEY_SIZE]);

int
hc_key_preencode_store_core_start (compact_state_t *state);
int
hc_key_encode_store_core_start (compact_state_t *state);

int
hc_key_preencode_store_core_end (compact_state_t *state);
int
hc_key_encode_store_core_end (compact_state_t *state);

int
hc_key_preencode_store_core_by_alias (compact_state_t *state, const uint8_t namespace[HC_KEY_NAMESPACE_SIZE], const uint8_t *name, size_t name_len);
int
hc_key_encode_store_core_by_alias (compact_state_t *state, const uint8_t namespace[HC_KEY_NAMESPACE_SIZE], const uint8_t *name, size_t name_len);

// Pass namespace = NULL for the all-namespaces variant.
int
hc_key_preencode_store_core_by_alias_start (compact_state_t *state, const uint8_t *namespace);
int
hc_key_encode_store_core_by_alias_start (compact_state_t *state, const uint8_t *namespace);

int
hc_key_preencode_store_core_by_alias_end (compact_state_t *state, const uint8_t *namespace);
int
hc_key_encode_store_core_by_alias_end (compact_state_t *state, const uint8_t *namespace);

// store.clear range (start inclusive, end exclusive).
int
hc_key_preencode_store_clear_start (compact_state_t *state);
int
hc_key_encode_store_clear_start (compact_state_t *state);

int
hc_key_preencode_store_clear_end (compact_state_t *state);
int
hc_key_encode_store_clear_end (compact_state_t *state);

// ---- core namespace (per-core records, keyed by ptr) ----

int
hc_key_preencode_core_core (compact_state_t *state, uint64_t ptr);
int
hc_key_encode_core_core (compact_state_t *state, uint64_t ptr);

int
hc_key_preencode_core_data (compact_state_t *state, uint64_t ptr);
int
hc_key_encode_core_data (compact_state_t *state, uint64_t ptr);

int
hc_key_preencode_core_auth (compact_state_t *state, uint64_t ptr);
int
hc_key_encode_core_auth (compact_state_t *state, uint64_t ptr);

int
hc_key_preencode_core_sessions (compact_state_t *state, uint64_t ptr);
int
hc_key_encode_core_sessions (compact_state_t *state, uint64_t ptr);

int
hc_key_preencode_core_head (compact_state_t *state, uint64_t ptr);
int
hc_key_encode_core_head (compact_state_t *state, uint64_t ptr);

int
hc_key_preencode_core_dependency (compact_state_t *state, uint64_t ptr);
int
hc_key_encode_core_dependency (compact_state_t *state, uint64_t ptr);

int
hc_key_preencode_core_hints (compact_state_t *state, uint64_t ptr);
int
hc_key_encode_core_hints (compact_state_t *state, uint64_t ptr);

int
hc_key_preencode_core_block (compact_state_t *state, uint64_t ptr, uint64_t index);
int
hc_key_encode_core_block (compact_state_t *state, uint64_t ptr, uint64_t index);

int
hc_key_preencode_core_tree (compact_state_t *state, uint64_t ptr, uint64_t index);
int
hc_key_encode_core_tree (compact_state_t *state, uint64_t ptr, uint64_t index);

int
hc_key_preencode_core_bitfield (compact_state_t *state, uint64_t ptr, uint64_t index, uint64_t type);
int
hc_key_encode_core_bitfield (compact_state_t *state, uint64_t ptr, uint64_t index, uint64_t type);

int
hc_key_preencode_core_user_data (compact_state_t *state, uint64_t ptr, const utf8_string_view_t key);
int
hc_key_encode_core_user_data (compact_state_t *state, uint64_t ptr, const utf8_string_view_t key);

int
hc_key_preencode_core_user_data_end (compact_state_t *state, uint64_t ptr);
int
hc_key_encode_core_user_data_end (compact_state_t *state, uint64_t ptr);

int
hc_key_preencode_core_mark (compact_state_t *state, uint64_t ptr, uint64_t index);
int
hc_key_encode_core_mark (compact_state_t *state, uint64_t ptr, uint64_t index);

// core.local does not frame its key (raw bytes appended). key_len must be
// <= HC_KEY_CORE_LOCAL_MAX.
int
hc_key_preencode_core_local (compact_state_t *state, uint64_t ptr, size_t key_len);
int
hc_key_encode_core_local (compact_state_t *state, uint64_t ptr, const uint8_t *key, size_t key_len);

int
hc_key_preencode_core_local_end (compact_state_t *state, uint64_t ptr);
int
hc_key_encode_core_local_end (compact_state_t *state, uint64_t ptr);

#endif // HC_ENCODINGS_KEYS_H
