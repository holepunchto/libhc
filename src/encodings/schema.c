#include <stdint.h>

#include <compact.h>

#include "schema.h"

int
hc_schema_preencode_tree_node (compact_state_t *state, const hc_merkle_tree_node_t *node) {
  compact_preencode_uint(state, node->index);
  compact_preencode_uint(state, node->size);
  return compact_preencode_fixed32(state, node->hash);
}

int
hc_schema_encode_tree_node (compact_state_t *state, const hc_merkle_tree_node_t *node) {
  compact_encode_uint(state, node->index);
  compact_encode_uint(state, node->size);
  return compact_encode_fixed32(state, node->hash);
}

int
hc_schema_decode_tree_node (compact_state_t *state, hc_merkle_tree_node_t *node) {
  uintmax_t index;
  uintmax_t size;
  compact_decode_uint(state, &index);
  compact_decode_uint(state, &size);
  node->index = (uint64_t) index;
  node->size = (uint64_t) size;
  return compact_decode_fixed32(state, node->hash);
}
