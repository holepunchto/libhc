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
  if (upgrade->roots.buffers == NULL) {
    size_t cap = upgrade->core->roots.length + 2 * count;
    if (cap > HC_MERKLE_TREE_MAX_ROOTS) cap = HC_MERKLE_TREE_MAX_ROOTS;
    if (hc__array_grow(&upgrade->roots, cap) < 0) return -1;
    upgrade->roots.length = upgrade->core->roots.length;
    if (upgrade->core->roots.length > 0) {
      memcpy(upgrade->roots.buffers, upgrade->core->roots.buffers, upgrade->core->roots.length * sizeof(*upgrade->roots.buffers));
    }
    upgrade->length = upgrade->core->length;
    upgrade->byte_length = upgrade->core->byte_length;
  }

  if (hc__db_core_write_ensure_tree_nodes(write, 2 * count) < 0) return -1;

  for (size_t i = 0; i < count; i++) {
    if (upgrade->roots.length >= HC_MERKLE_TREE_MAX_ROOTS) return -1;

    flat_tree_iterator_t ite;
    uint64_t head = upgrade->length * 2;
    flat_tree_iterator_init(&ite, head);

    hc_tree_node_t leaf;
    leaf.index = head;
    leaf.size = buffers[i].len;
    hc_crypto_data(leaf.hash, buffers[i].buffer, buffers[i].len);

    upgrade->length++;
    upgrade->byte_length += buffers[i].len;

    upgrade->roots.buffers[upgrade->roots.length++] = leaf;
    if (hc__db_core_write_tree_node(write, &leaf) < 0) return -1;

    while (upgrade->roots.length > 1) {
      hc_tree_node_t *a = &upgrade->roots.buffers[upgrade->roots.length - 1];
      hc_tree_node_t *b = &upgrade->roots.buffers[upgrade->roots.length - 2];

      uint64_t sib = flat_tree_iterator_sibling(&ite);
      if (sib != b->index) {
        flat_tree_iterator_sibling(&ite); // toggle back
        break;
      }

      hc_tree_node_t parent;
      parent.index = flat_tree_iterator_parent(&ite);
      parent.size = a->size + b->size;
      hc_crypto_parent(parent.hash, (const hc_crypto_node_t *) a, (const hc_crypto_node_t *) b);

      upgrade->roots.length -= 2;
      upgrade->roots.buffers[upgrade->roots.length++] = parent;
      if (hc__db_core_write_tree_node(write, &parent) < 0) return -1;
    }
  }

  return 0;
}
