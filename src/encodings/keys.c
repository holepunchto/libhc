#include <string.h>

#include <compact.h>
#include <lexkey.h>

#include "keys.h"

#define TRY(expr) \
  do { \
    int _err = (expr); \
    if (_err < 0) return _err; \
  } while (0)

// ---- store namespace ----

int
hc_key_preencode_store_head (compact_state_t *state) {
  return lexkey_preencode_uint(state, HC_KEY_TL_HEAD);
}

int
hc_key_encode_store_head (compact_state_t *state) {
  return lexkey_encode_uint(state, HC_KEY_TL_HEAD);
}

int
hc_key_preencode_store_core (compact_state_t *state, const uint8_t discovery_key[HC_KEY_DISCOVERY_KEY_SIZE]) {
  TRY(lexkey_preencode_uint(state, HC_KEY_TL_CORE_BY_DKEY));
  return compact_preencode_fixed32(state, discovery_key);
}

int
hc_key_encode_store_core (compact_state_t *state, const uint8_t discovery_key[HC_KEY_DISCOVERY_KEY_SIZE]) {
  TRY(lexkey_encode_uint(state, HC_KEY_TL_CORE_BY_DKEY));
  return compact_encode_fixed32(state, discovery_key);
}

int
hc_key_preencode_store_core_start (compact_state_t *state) {
  return lexkey_preencode_uint(state, HC_KEY_TL_CORE_BY_DKEY);
}

int
hc_key_encode_store_core_start (compact_state_t *state) {
  return lexkey_encode_uint(state, HC_KEY_TL_CORE_BY_DKEY);
}

int
hc_key_preencode_store_core_end (compact_state_t *state) {
  return lexkey_preencode_uint(state, HC_KEY_TL_CORE_BY_DKEY + 1);
}

int
hc_key_encode_store_core_end (compact_state_t *state) {
  return lexkey_encode_uint(state, HC_KEY_TL_CORE_BY_DKEY + 1);
}

int
hc_key_preencode_store_core_by_alias (compact_state_t *state, const uint8_t namespace[HC_KEY_NAMESPACE_SIZE], const uint8_t *name, size_t name_len) {
  TRY(lexkey_preencode_uint(state, HC_KEY_TL_CORE_BY_ALIAS));
  TRY(compact_preencode_fixed32(state, namespace));
  return lexkey_preencode_buffer(state, name, name_len);
}

int
hc_key_encode_store_core_by_alias (compact_state_t *state, const uint8_t namespace[HC_KEY_NAMESPACE_SIZE], const uint8_t *name, size_t name_len) {
  TRY(lexkey_encode_uint(state, HC_KEY_TL_CORE_BY_ALIAS));
  TRY(compact_encode_fixed32(state, namespace));
  return lexkey_encode_buffer(state, name, name_len);
}

int
hc_key_preencode_store_core_by_alias_start (compact_state_t *state, const uint8_t *namespace) {
  TRY(lexkey_preencode_uint(state, HC_KEY_TL_CORE_BY_ALIAS));
  if (namespace) return compact_preencode_fixed32(state, namespace);
  return 0;
}

int
hc_key_encode_store_core_by_alias_start (compact_state_t *state, const uint8_t *namespace) {
  TRY(lexkey_encode_uint(state, HC_KEY_TL_CORE_BY_ALIAS));
  if (namespace) return compact_encode_fixed32(state, namespace);
  return 0;
}

int
hc_key_preencode_store_core_by_alias_end (compact_state_t *state, const uint8_t *namespace) {
  if (namespace) {
    TRY(lexkey_preencode_uint(state, HC_KEY_TL_CORE_BY_ALIAS));
    TRY(compact_preencode_fixed32(state, namespace));
    state->end += 1; // trailing 0xff sentinel
    return 0;
  }
  return lexkey_preencode_uint(state, HC_KEY_TL_CORE_BY_ALIAS + 1);
}

int
hc_key_encode_store_core_by_alias_end (compact_state_t *state, const uint8_t *namespace) {
  if (namespace) {
    TRY(lexkey_encode_uint(state, HC_KEY_TL_CORE_BY_ALIAS));
    TRY(compact_encode_fixed32(state, namespace));
    state->buffer[state->start++] = 0xff;
    return 0;
  }
  return lexkey_encode_uint(state, HC_KEY_TL_CORE_BY_ALIAS + 1);
}

int
hc_key_preencode_store_clear_start (compact_state_t *state) {
  return lexkey_preencode_uint(state, HC_KEY_TL_HEAD);
}

int
hc_key_encode_store_clear_start (compact_state_t *state) {
  return lexkey_encode_uint(state, HC_KEY_TL_HEAD);
}

int
hc_key_preencode_store_clear_end (compact_state_t *state) {
  return lexkey_preencode_uint(state, HC_KEY_TL_END);
}

int
hc_key_encode_store_clear_end (compact_state_t *state) {
  return lexkey_encode_uint(state, HC_KEY_TL_END);
}

// ---- core namespace ----

static int
preencode_core_prefix (compact_state_t *state, uint64_t tl, uint64_t ptr) {
  TRY(lexkey_preencode_uint(state, tl));
  return lexkey_preencode_uint(state, ptr);
}

static int
encode_core_prefix (compact_state_t *state, uint64_t tl, uint64_t ptr) {
  TRY(lexkey_encode_uint(state, tl));
  return lexkey_encode_uint(state, ptr);
}

static int
preencode_core_subtype (compact_state_t *state, uint64_t tl, uint64_t ptr, uint64_t subtype) {
  TRY(preencode_core_prefix(state, tl, ptr));
  return lexkey_preencode_uint(state, subtype);
}

static int
encode_core_subtype (compact_state_t *state, uint64_t tl, uint64_t ptr, uint64_t subtype) {
  TRY(encode_core_prefix(state, tl, ptr));
  return lexkey_encode_uint(state, subtype);
}

int
hc_key_preencode_core_core (compact_state_t *state, uint64_t ptr) {
  return preencode_core_prefix(state, HC_KEY_TL_CORE, ptr);
}

int
hc_key_encode_core_core (compact_state_t *state, uint64_t ptr) {
  return encode_core_prefix(state, HC_KEY_TL_CORE, ptr);
}

int
hc_key_preencode_core_data (compact_state_t *state, uint64_t ptr) {
  return preencode_core_prefix(state, HC_KEY_TL_DATA, ptr);
}

int
hc_key_encode_core_data (compact_state_t *state, uint64_t ptr) {
  return encode_core_prefix(state, HC_KEY_TL_DATA, ptr);
}

int
hc_key_preencode_core_auth (compact_state_t *state, uint64_t ptr) {
  return preencode_core_subtype(state, HC_KEY_TL_CORE, ptr, HC_KEY_CORE_AUTH);
}

int
hc_key_encode_core_auth (compact_state_t *state, uint64_t ptr) {
  return encode_core_subtype(state, HC_KEY_TL_CORE, ptr, HC_KEY_CORE_AUTH);
}

int
hc_key_preencode_core_sessions (compact_state_t *state, uint64_t ptr) {
  return preencode_core_subtype(state, HC_KEY_TL_CORE, ptr, HC_KEY_CORE_SESSIONS);
}

int
hc_key_encode_core_sessions (compact_state_t *state, uint64_t ptr) {
  return encode_core_subtype(state, HC_KEY_TL_CORE, ptr, HC_KEY_CORE_SESSIONS);
}

int
hc_key_preencode_core_head (compact_state_t *state, uint64_t ptr) {
  return preencode_core_subtype(state, HC_KEY_TL_DATA, ptr, HC_KEY_DATA_HEAD);
}

int
hc_key_encode_core_head (compact_state_t *state, uint64_t ptr) {
  return encode_core_subtype(state, HC_KEY_TL_DATA, ptr, HC_KEY_DATA_HEAD);
}

int
hc_key_preencode_core_dependency (compact_state_t *state, uint64_t ptr) {
  return preencode_core_subtype(state, HC_KEY_TL_DATA, ptr, HC_KEY_DATA_DEPENDENCY);
}

int
hc_key_encode_core_dependency (compact_state_t *state, uint64_t ptr) {
  return encode_core_subtype(state, HC_KEY_TL_DATA, ptr, HC_KEY_DATA_DEPENDENCY);
}

int
hc_key_preencode_core_hints (compact_state_t *state, uint64_t ptr) {
  return preencode_core_subtype(state, HC_KEY_TL_DATA, ptr, HC_KEY_DATA_HINTS);
}

int
hc_key_encode_core_hints (compact_state_t *state, uint64_t ptr) {
  return encode_core_subtype(state, HC_KEY_TL_DATA, ptr, HC_KEY_DATA_HINTS);
}

int
hc_key_preencode_core_block (compact_state_t *state, uint64_t ptr, uint64_t index) {
  TRY(preencode_core_subtype(state, HC_KEY_TL_DATA, ptr, HC_KEY_DATA_BLOCK));
  return lexkey_preencode_uint(state, index);
}

int
hc_key_encode_core_block (compact_state_t *state, uint64_t ptr, uint64_t index) {
  TRY(encode_core_subtype(state, HC_KEY_TL_DATA, ptr, HC_KEY_DATA_BLOCK));
  return lexkey_encode_uint(state, index);
}

int
hc_key_preencode_core_tree (compact_state_t *state, uint64_t ptr, uint64_t index) {
  TRY(preencode_core_subtype(state, HC_KEY_TL_DATA, ptr, HC_KEY_DATA_TREE));
  return lexkey_preencode_uint(state, index);
}

int
hc_key_encode_core_tree (compact_state_t *state, uint64_t ptr, uint64_t index) {
  TRY(encode_core_subtype(state, HC_KEY_TL_DATA, ptr, HC_KEY_DATA_TREE));
  return lexkey_encode_uint(state, index);
}

int
hc_key_preencode_core_bitfield (compact_state_t *state, uint64_t ptr, uint64_t index, uint64_t type) {
  TRY(preencode_core_subtype(state, HC_KEY_TL_DATA, ptr, HC_KEY_DATA_BITFIELD));
  TRY(lexkey_preencode_uint(state, index));
  return lexkey_preencode_uint(state, type);
}

int
hc_key_encode_core_bitfield (compact_state_t *state, uint64_t ptr, uint64_t index, uint64_t type) {
  TRY(encode_core_subtype(state, HC_KEY_TL_DATA, ptr, HC_KEY_DATA_BITFIELD));
  TRY(lexkey_encode_uint(state, index));
  return lexkey_encode_uint(state, type);
}

int
hc_key_preencode_core_user_data (compact_state_t *state, uint64_t ptr, const utf8_string_view_t key) {
  TRY(preencode_core_subtype(state, HC_KEY_TL_DATA, ptr, HC_KEY_DATA_USER_DATA));
  return lexkey_preencode_string(state, key);
}

int
hc_key_encode_core_user_data (compact_state_t *state, uint64_t ptr, const utf8_string_view_t key) {
  TRY(encode_core_subtype(state, HC_KEY_TL_DATA, ptr, HC_KEY_DATA_USER_DATA));
  return lexkey_encode_string(state, key);
}

int
hc_key_preencode_core_user_data_end (compact_state_t *state, uint64_t ptr) {
  return preencode_core_subtype(state, HC_KEY_TL_DATA, ptr, HC_KEY_DATA_USER_DATA + 1);
}

int
hc_key_encode_core_user_data_end (compact_state_t *state, uint64_t ptr) {
  return encode_core_subtype(state, HC_KEY_TL_DATA, ptr, HC_KEY_DATA_USER_DATA + 1);
}

int
hc_key_preencode_core_mark (compact_state_t *state, uint64_t ptr, uint64_t index) {
  TRY(preencode_core_subtype(state, HC_KEY_TL_DATA, ptr, HC_KEY_DATA_MARK));
  return lexkey_preencode_uint(state, index);
}

int
hc_key_encode_core_mark (compact_state_t *state, uint64_t ptr, uint64_t index) {
  TRY(encode_core_subtype(state, HC_KEY_TL_DATA, ptr, HC_KEY_DATA_MARK));
  return lexkey_encode_uint(state, index);
}

int
hc_key_preencode_core_local (compact_state_t *state, uint64_t ptr, size_t key_len) {
  TRY(preencode_core_subtype(state, HC_KEY_TL_DATA, ptr, HC_KEY_DATA_LOCAL));
  state->end += key_len;
  return 0;
}

int
hc_key_encode_core_local (compact_state_t *state, uint64_t ptr, const uint8_t *key, size_t key_len) {
  TRY(encode_core_subtype(state, HC_KEY_TL_DATA, ptr, HC_KEY_DATA_LOCAL));
  memcpy(state->buffer + state->start, key, key_len);
  state->start += key_len;
  return 0;
}

int
hc_key_preencode_core_local_end (compact_state_t *state, uint64_t ptr) {
  return preencode_core_subtype(state, HC_KEY_TL_DATA, ptr, HC_KEY_DATA_LOCAL + 1);
}

int
hc_key_encode_core_local_end (compact_state_t *state, uint64_t ptr) {
  return encode_core_subtype(state, HC_KEY_TL_DATA, ptr, HC_KEY_DATA_LOCAL + 1);
}
