#ifndef HC_SCHEMA_H
#define HC_SCHEMA_H

#include <compact.h>

#include "merkle_tree.h"

#ifdef __cplusplus
extern "C" {
#endif

// Compact encoders for the value side of every kv record. Same two-pass
// preencode/encode/decode style as libcompact.

int
hc_schema_preencode_tree_node (compact_state_t *state, const hc_merkle_tree_node_t *node);
int
hc_schema_encode_tree_node (compact_state_t *state, const hc_merkle_tree_node_t *node);
int
hc_schema_decode_tree_node (compact_state_t *state, hc_merkle_tree_node_t *node);

#ifdef __cplusplus
}
#endif

#endif // HC_SCHEMA_H
