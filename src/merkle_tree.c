#include <stdlib.h>
#include <string.h>

#include <flattree.h>

#include "hc/core.h"
#include "hc/crypto.h"
#include "hc/merkle_tree.h"

int
hc_merkle_tree_batch_init (hc_merkle_tree_batch_t *batch, hc_core_t *core) {
  batch->core = core;
  batch->roots = NULL;
  batch->roots_len = 0;
  batch->updated = NULL;
  batch->updated_len = 0;
  batch->length = 0;
  batch->byte_length = 0;
  return 0;
}

void
hc_merkle_tree_batch_destroy (hc_merkle_tree_batch_t *batch) {
  free(batch->roots);
  free(batch->updated);
}

int
hc_merkle_tree_append (hc_merkle_tree_batch_t *batch, const hc_buf_t *buffers, size_t count) {
  if (count == 0) return 0;

  // Lazy fork from core on the first append call.
  if (batch->roots == NULL) {
    size_t cap = batch->core->roots_len + 2 * count;
    if (cap > HC_MERKLE_TREE_MAX_ROOTS) cap = HC_MERKLE_TREE_MAX_ROOTS;
    batch->roots = malloc(cap * sizeof(*batch->roots));
    if (batch->roots == NULL) return -1;
    batch->roots_len = batch->core->roots_len;
    if (batch->core->roots_len > 0) {
      memcpy(batch->roots, batch->core->roots, batch->core->roots_len * sizeof(*batch->roots));
    }
    batch->length = batch->core->length;
    batch->byte_length = batch->core->byte_length;
  }

  // Grow updated to fit the worst case (count leaves + count parents).
  size_t updated_needed = batch->updated_len + 2 * count;
  hc_merkle_tree_node_t *new_updated = realloc(batch->updated, updated_needed * sizeof(*new_updated));
  if (new_updated == NULL) return -1;
  batch->updated = new_updated;

  for (size_t i = 0; i < count; i++) {
    if (batch->roots_len >= HC_MERKLE_TREE_MAX_ROOTS) return -1;

    flat_tree_iterator_t ite;
    uint64_t head = batch->length * 2;
    flat_tree_iterator_init(&ite, head);

    hc_merkle_tree_node_t leaf;
    leaf.index = head;
    leaf.size = buffers[i].len;
    hc_crypto_data(leaf.hash, buffers[i].buffer, buffers[i].len);

    batch->length++;
    batch->byte_length += buffers[i].len;

    batch->roots[batch->roots_len++] = leaf;
    batch->updated[batch->updated_len++] = leaf;

    while (batch->roots_len > 1) {
      hc_merkle_tree_node_t *a = &batch->roots[batch->roots_len - 1];
      hc_merkle_tree_node_t *b = &batch->roots[batch->roots_len - 2];

      uint64_t sib = flat_tree_iterator_sibling(&ite);
      if (sib != b->index) {
        flat_tree_iterator_sibling(&ite); // toggle back
        break;
      }

      hc_merkle_tree_node_t parent;
      parent.index = flat_tree_iterator_parent(&ite);
      parent.size = a->size + b->size;
      hc_crypto_parent(parent.hash, (const hc_crypto_node_t *) a, (const hc_crypto_node_t *) b);

      batch->roots_len -= 2;
      batch->roots[batch->roots_len++] = parent;
      batch->updated[batch->updated_len++] = parent;
    }
  }

  return 0;
}
