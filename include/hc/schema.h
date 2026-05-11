#ifndef HC_SCHEMA_H
#define HC_SCHEMA_H

#include <compact.h>

#include "head.h"
#include "manifest.h"
#include "merkle_tree.h"

#ifdef __cplusplus
extern "C" {
#endif

int
hc_tree_node_preencode (compact_state_t *state, const hc_merkle_tree_node_t *node);
int
hc_tree_node_encode (compact_state_t *state, const hc_merkle_tree_node_t *node);
int
hc_tree_node_decode (compact_state_t *state, hc_merkle_tree_node_t *node);

int
hc_head_preencode (compact_state_t *state, const hc_head_t *head);
int
hc_head_encode (compact_state_t *state, const hc_head_t *head);
int
hc_head_decode (compact_state_t *state, hc_head_t *head);

int
hc_manifest_preencode (compact_state_t *state, const hc_manifest_t *manifest);
int
hc_manifest_encode (compact_state_t *state, const hc_manifest_t *manifest);
int
hc_manifest_decode (compact_state_t *state, hc_manifest_t *manifest);

#ifdef __cplusplus
}
#endif

#endif // HC_SCHEMA_H
