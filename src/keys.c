#include <compact.h>
#include <lexkey.h>

#include "hc/keys.h"

static void
encode_tl_ptr (hc_small_key_t *key, uint64_t tl, uint64_t ptr) {
  compact_state_t state = {0, sizeof(key->data), key->data};
  lexkey_encode_uint(&state, tl);
  lexkey_encode_uint(&state, ptr);
  key->buf.buffer = key->data;
  key->buf.len = state.start;
}

static void
encode_tl_ptr_subtype (hc_small_key_t *key, uint64_t tl, uint64_t ptr, uint64_t subtype) {
  compact_state_t state = {0, sizeof(key->data), key->data};
  lexkey_encode_uint(&state, tl);
  lexkey_encode_uint(&state, ptr);
  lexkey_encode_uint(&state, subtype);
  key->buf.buffer = key->data;
  key->buf.len = state.start;
}

static void
encode_tl_ptr_subtype_index (hc_small_key_t *key, uint64_t tl, uint64_t ptr, uint64_t subtype, uint64_t index) {
  compact_state_t state = {0, sizeof(key->data), key->data};
  lexkey_encode_uint(&state, tl);
  lexkey_encode_uint(&state, ptr);
  lexkey_encode_uint(&state, subtype);
  lexkey_encode_uint(&state, index);
  key->buf.buffer = key->data;
  key->buf.len = state.start;
}

void
hc_key_core_core (hc_small_key_t *key, uint64_t core_ptr) {
  encode_tl_ptr(key, HC_KEY_TL_CORE, core_ptr);
}

void
hc_key_core_data (hc_small_key_t *key, uint64_t data_ptr) {
  encode_tl_ptr(key, HC_KEY_TL_DATA, data_ptr);
}

void
hc_key_core_auth (hc_small_key_t *key, uint64_t core_ptr) {
  encode_tl_ptr_subtype(key, HC_KEY_TL_CORE, core_ptr, HC_KEY_CORE_AUTH);
}

void
hc_key_core_sessions (hc_small_key_t *key, uint64_t core_ptr) {
  encode_tl_ptr_subtype(key, HC_KEY_TL_CORE, core_ptr, HC_KEY_CORE_SESSIONS);
}

void
hc_key_core_head (hc_small_key_t *key, uint64_t data_ptr) {
  encode_tl_ptr_subtype(key, HC_KEY_TL_DATA, data_ptr, HC_KEY_DATA_HEAD);
}

void
hc_key_core_dependency (hc_small_key_t *key, uint64_t data_ptr) {
  encode_tl_ptr_subtype(key, HC_KEY_TL_DATA, data_ptr, HC_KEY_DATA_DEPENDENCY);
}

void
hc_key_core_hints (hc_small_key_t *key, uint64_t data_ptr) {
  encode_tl_ptr_subtype(key, HC_KEY_TL_DATA, data_ptr, HC_KEY_DATA_HINTS);
}

void
hc_key_core_block (hc_small_key_t *key, uint64_t data_ptr, uint64_t index) {
  encode_tl_ptr_subtype_index(key, HC_KEY_TL_DATA, data_ptr, HC_KEY_DATA_BLOCK, index);
}

void
hc_key_core_tree (hc_small_key_t *key, uint64_t data_ptr, uint64_t index) {
  encode_tl_ptr_subtype_index(key, HC_KEY_TL_DATA, data_ptr, HC_KEY_DATA_TREE, index);
}

void
hc_key_core_mark (hc_small_key_t *key, uint64_t data_ptr, uint64_t index) {
  encode_tl_ptr_subtype_index(key, HC_KEY_TL_DATA, data_ptr, HC_KEY_DATA_MARK, index);
}
