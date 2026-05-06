#ifndef HC_ENCODINGS_SCHEMA_H
#define HC_ENCODINGS_SCHEMA_H

#include <compact.h>

#include "hc/merkle_tree.h"

// Compact encoders for the value side of every kv record. Same two-pass
// preencode/encode/decode style as libcompact.

int
hc_schema_preencode_tree_node (compact_state_t *state, const hc_merkle_tree_node_t *node);
int
hc_schema_encode_tree_node (compact_state_t *state, const hc_merkle_tree_node_t *node);
int
hc_schema_decode_tree_node (compact_state_t *state, hc_merkle_tree_node_t *node);

#endif // HC_ENCODINGS_SCHEMA_H
