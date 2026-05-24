#ifndef HC_SCHEMA_H
#define HC_SCHEMA_H

#include <compact.h>

#include "head.h"
#include "manifest.h"
#include "merkle_tree.h"

// Forward decl to avoid a cycle through store.h (which now includes db.h
// for hc__db_store_t).
struct hc_store_head_s;

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

int
hc_store_head_preencode (compact_state_t *state, const struct hc_store_head_s *head);
int
hc_store_head_encode (compact_state_t *state, const struct hc_store_head_s *head);
int
hc_store_head_decode (compact_state_t *state, struct hc_store_head_s *head);

int
hc_store_core_preencode (compact_state_t *state, uint64_t core_ptr, uint64_t data_ptr);
int
hc_store_core_encode (compact_state_t *state, uint64_t core_ptr, uint64_t data_ptr);
int
hc_store_core_decode (compact_state_t *state, uint64_t *core_ptr, uint64_t *data_ptr);

// Upper bounds on compact-encoded store records.
#define HC_STORE_HEAD_MAX_SIZE 128
#define HC_STORE_CORE_MAX_SIZE 32

#ifdef __cplusplus
}
#endif

#endif // HC_SCHEMA_H
