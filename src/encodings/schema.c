#include <stdint.h>
#include <stdlib.h>

#include <compact.h>

#include "hc/schema.h"

#define HC_MANIFEST_PATCH     0x01
#define HC_MANIFEST_PROLOGUE  0x02
#define HC_MANIFEST_LINKED    0x04
#define HC_MANIFEST_USER_DATA 0x08

int
hc_tree_node_preencode (compact_state_t *state, const hc_merkle_tree_node_t *node) {
  compact_preencode_uint(state, node->index);
  compact_preencode_uint(state, node->size);
  return compact_preencode_fixed32(state, node->hash);
}

int
hc_tree_node_encode (compact_state_t *state, const hc_merkle_tree_node_t *node) {
  compact_encode_uint(state, node->index);
  compact_encode_uint(state, node->size);
  return compact_encode_fixed32(state, node->hash);
}

int
hc_tree_node_decode (compact_state_t *state, hc_merkle_tree_node_t *node) {
  uintmax_t index;
  uintmax_t size;
  compact_decode_uint(state, &index);
  compact_decode_uint(state, &size);
  node->index = (uint64_t) index;
  node->size = (uint64_t) size;
  return compact_decode_fixed32(state, node->hash);
}

int
hc_head_preencode (compact_state_t *state, const hc_head_t *head) {
  compact_preencode_uint(state, head->fork);
  compact_preencode_uint(state, head->length);
  compact_preencode_fixed32(state, head->root_hash);
  return compact_preencode_uint8array(state, head->signature.buffer, head->signature.len);
}

int
hc_head_encode (compact_state_t *state, const hc_head_t *head) {
  compact_encode_uint(state, head->fork);
  compact_encode_uint(state, head->length);
  compact_encode_fixed32(state, head->root_hash);
  return compact_encode_uint8array(state, head->signature.buffer, head->signature.len);
}

int
hc_head_decode (compact_state_t *state, hc_head_t *head) {
  uintmax_t fork, length;
  if (compact_decode_uint(state, &fork) < 0) return -1;
  if (compact_decode_uint(state, &length) < 0) return -1;
  head->fork = (uint64_t) fork;
  head->length = (uint64_t) length;
  if (compact_decode_fixed32(state, head->root_hash) < 0) return -1;
  return compact_decode_uint8array(state, &head->signature.buffer, &head->signature.len);
}

static int
preencode_signer (compact_state_t *state, const hc_signer_t *s) {
  compact_preencode_uint(state, (uintmax_t) s->signature);
  compact_preencode_fixed32(state, s->namespace);
  return compact_preencode_fixed32(state, s->public_key);
}

static int
encode_signer (compact_state_t *state, const hc_signer_t *s) {
  compact_encode_uint(state, (uintmax_t) s->signature);
  compact_encode_fixed32(state, s->namespace);
  return compact_encode_fixed32(state, s->public_key);
}

static int
decode_signer (compact_state_t *state, hc_signer_t *s) {
  uintmax_t sig;
  if (compact_decode_uint(state, &sig) < 0) return -1;
  s->signature = (hc_signature_func_t) sig;
  if (compact_decode_fixed32(state, s->namespace) < 0) return -1;
  return compact_decode_fixed32(state, s->public_key);
}

static int
preencode_signer_array (compact_state_t *state, const hc_signer_array_t *arr) {
  compact_preencode_uint(state, (uintmax_t) arr->length);
  for (size_t i = 0; i < arr->length; i++) {
    if (preencode_signer(state, &arr->buffers[i]) < 0) return -1;
  }
  return 0;
}

static int
encode_signer_array (compact_state_t *state, const hc_signer_array_t *arr) {
  compact_encode_uint(state, (uintmax_t) arr->length);
  for (size_t i = 0; i < arr->length; i++) {
    if (encode_signer(state, &arr->buffers[i]) < 0) return -1;
  }
  return 0;
}

static int
decode_signer_array (compact_state_t *state, hc_signer_array_t *arr) {
  uintmax_t len;
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
  compact_preencode_uint(state, (uintmax_t) arr->length);
  for (size_t i = 0; i < arr->length; i++) {
    compact_preencode_fixed32(state, arr->buffers[i]);
  }
  return 0;
}

static int
encode_linked (compact_state_t *state, const hc_hash_array_t *arr) {
  compact_encode_uint(state, (uintmax_t) arr->length);
  for (size_t i = 0; i < arr->length; i++) {
    compact_encode_fixed32(state, arr->buffers[i]);
  }
  return 0;
}

static int
decode_linked (compact_state_t *state, hc_hash_array_t *arr) {
  uintmax_t len;
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
decode_v0 (compact_state_t *state, hc_manifest_t *m, uintmax_t hash_func) {
  uintmax_t type;
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
  uintmax_t flags;
  if (compact_decode_uint(state, &flags) < 0) return -1;
  m->allow_patch = (flags & 1) != 0;
  uintmax_t quorum;
  if (compact_decode_uint(state, &quorum) < 0) return -1;
  m->quorum = (uint32_t) quorum;
  return decode_signer_array(state, &m->signers);
}

int
hc_manifest_preencode (compact_state_t *state, const hc_manifest_t *m) {
  compact_preencode_uint(state, (uintmax_t) m->version);

  if (m->version == 0) return -1; // encode v0 not supported

  uint8_t flags = 0;
  if (m->allow_patch) flags |= HC_MANIFEST_PATCH;
  if (m->prologue) flags |= HC_MANIFEST_PROLOGUE;
  if (m->linked.length > 0) flags |= HC_MANIFEST_LINKED;
  if (m->user_data.buffer) flags |= HC_MANIFEST_USER_DATA;

  compact_preencode_uint(state, (uintmax_t) flags);
  compact_preencode_uint(state, (uintmax_t) m->hash);
  compact_preencode_uint(state, (uintmax_t) m->quorum);
  if (preencode_signer_array(state, &m->signers) < 0) return -1;
  if (m->prologue) {
    compact_preencode_fixed32(state, m->prologue->hash);
    compact_preencode_uint(state, (uintmax_t) m->prologue->length);
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
  compact_encode_uint(state, (uintmax_t) m->version);

  if (m->version == 0) return -1; // encode v0 not supported

  uint8_t flags = 0;
  if (m->allow_patch) flags |= HC_MANIFEST_PATCH;
  if (m->prologue) flags |= HC_MANIFEST_PROLOGUE;
  if (m->linked.length > 0) flags |= HC_MANIFEST_LINKED;
  if (m->user_data.buffer) flags |= HC_MANIFEST_USER_DATA;

  compact_encode_uint(state, (uintmax_t) flags);
  compact_encode_uint(state, (uintmax_t) m->hash);
  compact_encode_uint(state, (uintmax_t) m->quorum);
  if (encode_signer_array(state, &m->signers) < 0) return -1;
  if (m->prologue) {
    compact_encode_fixed32(state, m->prologue->hash);
    compact_encode_uint(state, (uintmax_t) m->prologue->length);
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
hc_manifest_decode (compact_state_t *state, hc_manifest_t *m) {
  uintmax_t version;
  if (compact_decode_uint(state, &version) < 0) return -1;

  if (version > 2) return -1;

  if (version == 0) {
    uintmax_t hash_func;
    if (compact_decode_uint(state, &hash_func) < 0) return -1;
    return decode_v0(state, m, hash_func);
  }

  uintmax_t flags;
  if (compact_decode_uint(state, &flags) < 0) return -1;
  uintmax_t hash_func;
  if (compact_decode_uint(state, &hash_func) < 0) return -1;
  uintmax_t quorum;
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
    uintmax_t plen;
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
