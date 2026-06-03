#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include <compact.h>

#include "hc/encodings.h"
#include "hc/store.h"

#define HC_MANIFEST_PATCH     0x01
#define HC_MANIFEST_PROLOGUE  0x02
#define HC_MANIFEST_LINKED    0x04
#define HC_MANIFEST_USER_DATA 0x08

#define HC_STORE_HEAD_SEED                   0x01
#define HC_STORE_HEAD_DEFAULT_DISCOVERY_KEY  0x02

int
hc_head_preencode (compact_state_t *state, const hc_head_t *head) {
  compact_preencode_uint(state, head->fork);
  compact_preencode_uint(state, head->length);
  compact_preencode_fixed32(state, head->root_hash);
  compact_preencode_uint8array(state, head->signature.buffer, head->signature.len);
  state->end++; // flags byte (uint, always at least 1 byte)
  if (head->timestamp) compact_preencode_uint64(state, head->timestamp);
  return 0;
}

int
hc_head_encode (compact_state_t *state, const hc_head_t *head) {
  uint8_t flags = head->timestamp ? 1 : 0;
  compact_encode_uint(state, head->fork);
  compact_encode_uint(state, head->length);
  compact_encode_fixed32(state, head->root_hash);
  compact_encode_uint8array(state, head->signature.buffer, head->signature.len);
  compact_encode_uint(state, flags);
  if (head->timestamp) compact_encode_uint64(state, head->timestamp);
  return 0;
}

int
hc_head_decode (compact_state_t *state, hc_head_t *head) {
  uint64_t fork, length;
  if (compact_decode_uint(state, &fork) < 0) return -1;
  if (compact_decode_uint(state, &length) < 0) return -1;
  head->fork = (uint64_t) fork;
  head->length = (uint64_t) length;
  if (compact_decode_fixed32(state, head->root_hash) < 0) return -1;

  uint8_t *sig_slice = NULL;
  size_t sig_len = 0;
  if (compact_decode_uint8array(state, &sig_slice, &sig_len) < 0) return -1;
  if (sig_len > 0) {
    head->signature.buffer = malloc(sig_len);
    if (head->signature.buffer == NULL) return -1;
    memcpy(head->signature.buffer, sig_slice, sig_len);
    head->signature.len = sig_len;
  } else {
    head->signature.buffer = NULL;
    head->signature.len = 0;
  }

  uint64_t flags = 0;
  if (state->start < state->end) {
    if (compact_decode_uint(state, &flags) < 0) return -1;
  }

  if (flags & 1) {
    if (compact_decode_uint64(state, &head->timestamp) < 0) return -1;
  } else {
    head->timestamp = 0;
  }

  return 0;
}

static int
preencode_signer (compact_state_t *state, const hc_signer_t *s) {
  compact_preencode_uint(state, (uint64_t) s->signature);
  compact_preencode_fixed32(state, s->namespace);
  return compact_preencode_fixed32(state, s->public_key);
}

static int
encode_signer (compact_state_t *state, const hc_signer_t *s) {
  compact_encode_uint(state, (uint64_t) s->signature);
  compact_encode_fixed32(state, s->namespace);
  return compact_encode_fixed32(state, s->public_key);
}

static int
decode_signer (compact_state_t *state, hc_signer_t *s) {
  uint64_t sig;
  if (compact_decode_uint(state, &sig) < 0) return -1;
  s->signature = (hc_signature_func_t) sig;
  if (compact_decode_fixed32(state, s->namespace) < 0) return -1;
  return compact_decode_fixed32(state, s->public_key);
}

static int
preencode_signer_array (compact_state_t *state, const hc_signer_array_t *arr) {
  compact_preencode_uint(state, (uint64_t) arr->length);
  for (size_t i = 0; i < arr->length; i++) {
    if (preencode_signer(state, &arr->buffers[i]) < 0) return -1;
  }
  return 0;
}

static int
encode_signer_array (compact_state_t *state, const hc_signer_array_t *arr) {
  compact_encode_uint(state, (uint64_t) arr->length);
  for (size_t i = 0; i < arr->length; i++) {
    if (encode_signer(state, &arr->buffers[i]) < 0) return -1;
  }
  return 0;
}

static int
decode_signer_array (compact_state_t *state, hc_signer_array_t *arr) {
  uint64_t len;
  if (compact_decode_uint(state, &len) < 0) return -1;
  arr->length = (size_t) len;
  arr->capacity = (size_t) len;
  arr->buffers = len > 0 ? malloc(len * sizeof(hc_signer_t)) : NULL;
  if (len > 0 && arr->buffers == NULL) return -1;
  for (size_t i = 0; i < arr->length; i++) {
    if (decode_signer(state, &arr->buffers[i]) < 0) return -1;
  }
  return 0;
}

static int
preencode_linked (compact_state_t *state, const hc_hash_array_t *arr) {
  compact_preencode_uint(state, (uint64_t) arr->length);
  for (size_t i = 0; i < arr->length; i++) {
    compact_preencode_fixed32(state, arr->buffers[i]);
  }
  return 0;
}

static int
encode_linked (compact_state_t *state, const hc_hash_array_t *arr) {
  compact_encode_uint(state, (uint64_t) arr->length);
  for (size_t i = 0; i < arr->length; i++) {
    compact_encode_fixed32(state, arr->buffers[i]);
  }
  return 0;
}

static int
decode_linked (compact_state_t *state, hc_hash_array_t *arr) {
  uint64_t len;
  if (compact_decode_uint(state, &len) < 0) return -1;
  arr->length = (size_t) len;
  arr->capacity = (size_t) len;
  arr->buffers = len > 0 ? malloc(len * sizeof(hc_hash_t)) : NULL;
  if (len > 0 && arr->buffers == NULL) return -1;
  for (size_t i = 0; i < arr->length; i++) {
    if (compact_decode_fixed32(state, arr->buffers[i]) < 0) return -1;
  }
  return 0;
}

static int
decode_v0 (compact_state_t *state, hc_manifest_t *m, uint64_t hash_func) {
  uint64_t type;
  if (compact_decode_uint(state, &type) < 0) return -1;
  if (type > 2) return -1;

  m->version = 0;
  m->hash = (hc_hash_func_t) hash_func;
  m->allow_patch = false;
  m->quorum = 0;
  hc__array_init(&m->signers);
  m->prologue = NULL;
  hc__array_init(&m->linked);
  m->user_data.buffer = NULL;
  m->user_data.len = 0;

  if (type == 0) {
    m->prologue = malloc(sizeof(hc_prologue_t));
    if (m->prologue == NULL) return -1;
    m->prologue->length = 0;
    return compact_decode_fixed32(state, m->prologue->hash);
  }

  if (type == 1) {
    m->quorum = 1;
    if (hc__array_grow(&m->signers, 1) < 0) return -1;
    m->signers.length = 1;
    return decode_signer(state, &m->signers.buffers[0]);
  }

  // type == 2
  uint64_t flags;
  if (compact_decode_uint(state, &flags) < 0) return -1;
  m->allow_patch = (flags & 1) != 0;
  uint64_t quorum;
  if (compact_decode_uint(state, &quorum) < 0) return -1;
  m->quorum = (uint32_t) quorum;
  return decode_signer_array(state, &m->signers);
}

int
hc_manifest_preencode (compact_state_t *state, const hc_manifest_t *m) {
  compact_preencode_uint(state, (uint64_t) m->version);

  if (m->version == 0) return -1; // encode v0 not supported

  uint8_t flags = 0;
  if (m->allow_patch) flags |= HC_MANIFEST_PATCH;
  if (m->prologue) flags |= HC_MANIFEST_PROLOGUE;
  if (m->linked.length > 0) flags |= HC_MANIFEST_LINKED;
  if (m->user_data.buffer) flags |= HC_MANIFEST_USER_DATA;

  compact_preencode_uint(state, (uint64_t) flags);
  compact_preencode_uint(state, (uint64_t) m->hash);
  compact_preencode_uint(state, (uint64_t) m->quorum);
  if (preencode_signer_array(state, &m->signers) < 0) return -1;
  if (m->prologue) {
    compact_preencode_fixed32(state, m->prologue->hash);
    compact_preencode_uint(state, (uint64_t) m->prologue->length);
  }
  if (m->linked.length > 0) {
    if (preencode_linked(state, &m->linked) < 0) return -1;
  }
  if (m->user_data.buffer) {
    compact_preencode_uint8array(state, m->user_data.buffer, m->user_data.len);
  }
  return 0;
}

int
hc_manifest_encode (compact_state_t *state, const hc_manifest_t *m) {
  compact_encode_uint(state, (uint64_t) m->version);

  if (m->version == 0) return -1; // encode v0 not supported

  uint8_t flags = 0;
  if (m->allow_patch) flags |= HC_MANIFEST_PATCH;
  if (m->prologue) flags |= HC_MANIFEST_PROLOGUE;
  if (m->linked.length > 0) flags |= HC_MANIFEST_LINKED;
  if (m->user_data.buffer) flags |= HC_MANIFEST_USER_DATA;

  compact_encode_uint(state, (uint64_t) flags);
  compact_encode_uint(state, (uint64_t) m->hash);
  compact_encode_uint(state, (uint64_t) m->quorum);
  if (encode_signer_array(state, &m->signers) < 0) return -1;
  if (m->prologue) {
    compact_encode_fixed32(state, m->prologue->hash);
    compact_encode_uint(state, (uint64_t) m->prologue->length);
  }
  if (m->linked.length > 0) {
    if (encode_linked(state, &m->linked) < 0) return -1;
  }
  if (m->user_data.buffer) {
    compact_encode_uint8array(state, m->user_data.buffer, m->user_data.len);
  }
  return 0;
}

int
hc_store_head_preencode (compact_state_t *state, const struct hc_store_head_s *h) {
  compact_preencode_uint(state, 2);
  compact_preencode_uint(state, h->cores);
  compact_preencode_uint(state, h->datas);
  compact_preencode_uint(state, h->groups);
  uint8_t flags = 0;
  if (h->has_seed) flags |= HC_STORE_HEAD_SEED;
  if (h->has_default_discovery_key) flags |= HC_STORE_HEAD_DEFAULT_DISCOVERY_KEY;
  compact_preencode_uint(state, (uint64_t) flags);
  if (h->has_seed) compact_preencode_fixed32(state, h->seed);
  if (h->has_default_discovery_key) compact_preencode_fixed32(state, h->default_discovery_key);
  return 0;
}

int
hc_store_head_encode (compact_state_t *state, const struct hc_store_head_s *h) {
  compact_encode_uint(state, 2);
  compact_encode_uint(state, h->cores);
  compact_encode_uint(state, h->datas);
  compact_encode_uint(state, h->groups);
  uint8_t flags = 0;
  if (h->has_seed) flags |= HC_STORE_HEAD_SEED;
  if (h->has_default_discovery_key) flags |= HC_STORE_HEAD_DEFAULT_DISCOVERY_KEY;
  compact_encode_uint(state, (uint64_t) flags);
  if (h->has_seed) compact_encode_fixed32(state, h->seed);
  if (h->has_default_discovery_key) compact_encode_fixed32(state, h->default_discovery_key);
  return 0;
}

int
hc_store_head_decode (compact_state_t *state, struct hc_store_head_s *h) {
  uint64_t version;
  if (compact_decode_uint(state, &version) < 0) return -1;
  if (version > 2) return -1;

  if (version == 1) {
    uint64_t flags;
    if (compact_decode_uint(state, &flags) < 0) return -1;
    h->groups = 0;
    if (flags & 0x01) {
      uint64_t cores, datas;
      if (compact_decode_uint(state, &cores) < 0) return -1;
      if (compact_decode_uint(state, &datas) < 0) return -1;
      h->cores = (uint64_t) cores;
      h->datas = (uint64_t) datas;
    } else {
      h->cores = 0;
      h->datas = 0;
    }
    h->has_seed = (flags & 0x02) != 0;
    if (h->has_seed && compact_decode_fixed32(state, h->seed) < 0) return -1;
    h->has_default_discovery_key = (flags & 0x04) != 0;
    if (h->has_default_discovery_key && compact_decode_fixed32(state, h->default_discovery_key) < 0) return -1;
    return 0;
  }

  uint64_t cores, datas, groups, flags;
  if (compact_decode_uint(state, &cores) < 0) return -1;
  if (compact_decode_uint(state, &datas) < 0) return -1;
  if (compact_decode_uint(state, &groups) < 0) return -1;
  if (compact_decode_uint(state, &flags) < 0) return -1;
  h->cores = (uint64_t) cores;
  h->datas = (uint64_t) datas;
  h->groups = (uint64_t) groups;
  h->has_seed = (flags & HC_STORE_HEAD_SEED) != 0;
  if (h->has_seed && compact_decode_fixed32(state, h->seed) < 0) return -1;
  h->has_default_discovery_key = (flags & HC_STORE_HEAD_DEFAULT_DISCOVERY_KEY) != 0;
  if (h->has_default_discovery_key && compact_decode_fixed32(state, h->default_discovery_key) < 0) return -1;
  return 0;
}

int
hc_store_core_preencode (compact_state_t *state, uint64_t core_ptr, uint64_t data_ptr) {
  compact_preencode_uint(state, 1); // version
  compact_preencode_uint(state, core_ptr);
  compact_preencode_uint(state, data_ptr);
  compact_preencode_uint(state, 0); // flags (no alias)
  return 0;
}

int
hc_store_core_encode (compact_state_t *state, uint64_t core_ptr, uint64_t data_ptr) {
  compact_encode_uint(state, 1); // version
  compact_encode_uint(state, core_ptr);
  compact_encode_uint(state, data_ptr);
  compact_encode_uint(state, 0); // flags (no alias)
  return 0;
}

int
hc_store_core_decode (compact_state_t *state, uint64_t *core_ptr, uint64_t *data_ptr) {
  uint64_t version, cp, dp, flags;
  if (compact_decode_uint(state, &version) < 0) return -1;
  if (version != 1) return -1;
  if (compact_decode_uint(state, &cp) < 0) return -1;
  if (compact_decode_uint(state, &dp) < 0) return -1;
  if (compact_decode_uint(state, &flags) < 0) return -1;
  *core_ptr = (uint64_t) cp;
  *data_ptr = (uint64_t) dp;
  return 0;
}

int
hc_manifest_decode (compact_state_t *state, hc_manifest_t *m) {
  uint64_t version;
  if (compact_decode_uint(state, &version) < 0) return -1;

  if (version > 2) return -1;

  if (version == 0) {
    uint64_t hash_func;
    if (compact_decode_uint(state, &hash_func) < 0) return -1;
    return decode_v0(state, m, hash_func);
  }

  uint64_t flags;
  if (compact_decode_uint(state, &flags) < 0) return -1;
  uint64_t hash_func;
  if (compact_decode_uint(state, &hash_func) < 0) return -1;
  uint64_t quorum;
  if (compact_decode_uint(state, &quorum) < 0) return -1;

  m->version = (uint32_t) version;
  m->hash = (hc_hash_func_t) hash_func;
  m->allow_patch = (flags & HC_MANIFEST_PATCH) != 0;
  m->quorum = (uint32_t) quorum;

  if (decode_signer_array(state, &m->signers) < 0) return -1;

  if (flags & HC_MANIFEST_PROLOGUE) {
    m->prologue = malloc(sizeof(hc_prologue_t));
    if (m->prologue == NULL) return -1;
    if (compact_decode_fixed32(state, m->prologue->hash) < 0) return -1;
    uint64_t plen;
    if (compact_decode_uint(state, &plen) < 0) return -1;
    m->prologue->length = (uint64_t) plen;
  } else {
    m->prologue = NULL;
  }

  if (flags & HC_MANIFEST_LINKED) {
    if (decode_linked(state, &m->linked) < 0) return -1;
  } else {
    hc__array_init(&m->linked);
  }

  if (flags & HC_MANIFEST_USER_DATA) {
    if (compact_decode_uint8array(state, &m->user_data.buffer, &m->user_data.len) < 0) return -1;
  } else {
    m->user_data.buffer = NULL;
    m->user_data.len = 0;
  }

  return 0;
}
