#include <stdlib.h>
#include <string.h>

#include <flattree.h>

#include "hc/core.h"
#include "hc/crypto.h"
#include "hc/db.h"
#include "hc/merkle_tree.h"

int
hc_merkle_tree_append (hc_core_upgrade_t *upgrade, hc__db_core_write_t *write, const hc_buf_t *buffers, size_t count) {
  if (count == 0) return 0;

  // Lazy fork from core on the first append call.
  if (upgrade->roots == NULL) {
    size_t cap = upgrade->core->roots_len + 2 * count;
    if (cap > HC_MERKLE_TREE_MAX_ROOTS) cap = HC_MERKLE_TREE_MAX_ROOTS;
    upgrade->roots = malloc(cap * sizeof(*upgrade->roots));
    if (upgrade->roots == NULL) return -1;
    upgrade->roots_len = upgrade->core->roots_len;
    if (upgrade->core->roots_len > 0) {
      memcpy(upgrade->roots, upgrade->core->roots, upgrade->core->roots_len * sizeof(*upgrade->roots));
    }
    upgrade->length = upgrade->core->length;
    upgrade->byte_length = upgrade->core->byte_length;
  }

  if (hc__db_core_write_ensure_tree_nodes(write, 2 * count) < 0) return -1;
  if (hc__db_core_write_ensure_blocks(write, count) < 0) return -1;

  for (size_t i = 0; i < count; i++) {
    if (upgrade->roots_len >= HC_MERKLE_TREE_MAX_ROOTS) return -1;

    flat_tree_iterator_t ite;
    uint64_t head = upgrade->length * 2;
    flat_tree_iterator_init(&ite, head);

    hc_merkle_tree_node_t leaf;
    leaf.index = head;
    leaf.size = buffers[i].len;
    hc_crypto_data(leaf.hash, buffers[i].buffer, buffers[i].len);

    upgrade->length++;
    upgrade->byte_length += buffers[i].len;

    upgrade->roots[upgrade->roots_len++] = leaf;
    if (hc__db_core_write_tree_node(write, &leaf) < 0) return -1;
    if (hc__db_core_write_block(write, upgrade->length - 1, buffers[i]) < 0) return -1;

    while (upgrade->roots_len > 1) {
      hc_merkle_tree_node_t *a = &upgrade->roots[upgrade->roots_len - 1];
      hc_merkle_tree_node_t *b = &upgrade->roots[upgrade->roots_len - 2];

      uint64_t sib = flat_tree_iterator_sibling(&ite);
      if (sib != b->index) {
        flat_tree_iterator_sibling(&ite); // toggle back
        break;
      }

      hc_merkle_tree_node_t parent;
      parent.index = flat_tree_iterator_parent(&ite);
      parent.size = a->size + b->size;
      hc_crypto_parent(parent.hash, (const hc_crypto_node_t *) a, (const hc_crypto_node_t *) b);

      upgrade->roots_len -= 2;
      upgrade->roots[upgrade->roots_len++] = parent;
      if (hc__db_core_write_tree_node(write, &parent) < 0) return -1;
    }
  }

  return 0;
}
