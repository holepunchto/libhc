#ifndef HC_MERKLE_TREE_H
#define HC_MERKLE_TREE_H

#include <stddef.h>
#include <stdint.h>

#include <hc_schema.h>

#include "array.h"
#include "buffer.h"
#include "crypto.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef HC__ARRAY(hc_tree_node_t) hc_tree_node_array_t;

// A merkle tree over a uint64-indexed leaf space has at most 64 peaks (one
// per bit position).
#define HC_MERKLE_TREE_MAX_ROOTS 64

// Upper bound on a compact-encoded tree-node value:
// compact_uint(index) + compact_uint(size) + fixed32(hash) = 9 + 9 + 32.
#define HC_MERKLE_TREE_NODE_MAX_SIZE 50

// Forward decls to avoid include cycles.
struct hc_core_upgrade_s;
struct hc__db_core_write_s;

// Appends `count` leaves. Mutates `upgrade->roots`, `upgrade->length`, and
// `upgrade->byte_length`. Writes updated tree nodes and block data to `write`.
int
hc_merkle_tree_append (struct hc_core_upgrade_s *upgrade, struct hc__db_core_write_s *write, const hc_buf_t *buffers, size_t count);

#ifdef __cplusplus
}
#endif

#endif // HC_MERKLE_TREE_H
