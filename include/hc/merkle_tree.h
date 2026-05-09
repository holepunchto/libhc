#ifndef HC_MERKLE_TREE_H
#define HC_MERKLE_TREE_H

#include <stddef.h>
#include <stdint.h>

#include "buffer.h"
#include "crypto.h"
#include "keys.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
  uint64_t index;
  uint64_t size;
  uint8_t hash[HC_CRYPTO_HASH_SIZE];
} hc_merkle_tree_node_t;

// A merkle tree over a uint64-indexed leaf space has at most 64 peaks (one
// per bit position).
#define HC_MERKLE_TREE_MAX_ROOTS 64

// Upper bound on a compact-encoded tree-node value:
// compact_uint(index) + compact_uint(size) + fixed32(hash) = 9 + 9 + 32.
#define HC_MERKLE_TREE_NODE_MAX_SIZE 50

// Stack-allocatable carrier for an encoded tree-node value. The hc_buf_t
// header is the first member so &node->buf is interconvertible with a
// pointer to the struct.
typedef struct {
  hc_buf_t buf;
  uint8_t data[HC_MERKLE_TREE_NODE_MAX_SIZE];
} hc__tree_node_buffer_t;

typedef struct {
  hc__tree_node_buffer_t *buffers;
  size_t length;
  size_t capacity;
} hc__tree_node_buffer_array_t;

// Stack-allocatable key+value pair for a tree-node kv record.
typedef struct {
  hc_small_key_t key;
  hc__tree_node_buffer_t value;
} hc__tree_node_kv_t;

// Forward decl to avoid an include cycle with hc/core.h.
struct hc_core_s;

// Holds the result of a merkle tree mutation pending commit:
//   - roots: the new peak set
//   - updated: tree nodes created/changed during the mutation
//   - length/byte_length: the new core lengths
// Initialised via hc_merkle_tree_batch_init, destroyed via the matching
// destroy. The roots/length/byte_length fields are forked from the core
// on the first append call.
typedef struct {
  struct hc_core_s *core;
  hc_merkle_tree_node_t *roots;
  size_t roots_len;
  hc_merkle_tree_node_t *updated;
  size_t updated_len;
  uint64_t length;
  uint64_t byte_length;
} hc_merkle_tree_batch_t;

// Forks the core's tree state into `batch`: copies `roots`, `length`, and
// `byte_length` from the core. `updated` stays NULL/0; appends fill it in.
int
hc_merkle_tree_batch_init (hc_merkle_tree_batch_t *batch, struct hc_core_s *core);

void
hc_merkle_tree_batch_destroy (hc_merkle_tree_batch_t *batch);

// Appends `count` leaves into the batch. Mutates `batch->roots`,
// `batch->length`, `batch->byte_length`, and writes new/changed nodes to
// `batch->updated`.
int
hc_merkle_tree_append (hc_merkle_tree_batch_t *batch, const hc_buf_t *buffers, size_t count);

#ifdef __cplusplus
}
#endif

#endif // HC_MERKLE_TREE_H
