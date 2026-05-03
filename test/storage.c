#include <assert.h>
#include <string.h>

#include "hc/storage.h"

int
main () {
  hc_storage_core_t storage;
  assert(hc_storage_core_init(&storage, 0, 0) == 0);

  // Put a tree node, flush.
  hc_merkle_tree_node_t node = {.index = 7, .size = 42};
  memset(node.hash, 0xab, sizeof(node.hash));

  hc_storage_core_write_t write;
  assert(hc_storage_core_write(&storage, &write, -1) == 0);
  assert(hc_storage_core_write_put_tree_node(&write, 7, &node) == 0);
  assert(hc_storage_core_write_flush(&write) == 0);

  // Read it back, flush, verify.
  hc_storage_core_read_t read;
  assert(hc_storage_core_read(&storage, &read, -1) == 0);
  hc_merkle_tree_node_t got = {0};
  assert(hc_storage_core_read_get_tree_node(&read, 7, &got) == 0);
  assert(hc_storage_core_read_flush(&read) == 0);
  assert(got.index == 7);
  assert(got.size == 42);
  assert(memcmp(got.hash, node.hash, sizeof(node.hash)) == 0);

  // Multiple ops in one batch.
  hc_merkle_tree_node_t a = {.index = 100, .size = 1};
  hc_merkle_tree_node_t b = {.index = 200, .size = 2};
  memset(a.hash, 0x11, sizeof(a.hash));
  memset(b.hash, 0x22, sizeof(b.hash));

  assert(hc_storage_core_write(&storage, &write, -1) == 0);
  assert(hc_storage_core_write_put_tree_node(&write, 100, &a) == 0);
  assert(hc_storage_core_write_put_tree_node(&write, 200, &b) == 0);
  assert(hc_storage_core_write_flush(&write) == 0);

  assert(hc_storage_core_read(&storage, &read, -1) == 0);
  hc_merkle_tree_node_t got_a = {0};
  hc_merkle_tree_node_t got_b = {0};
  assert(hc_storage_core_read_get_tree_node(&read, 100, &got_a) == 0);
  assert(hc_storage_core_read_get_tree_node(&read, 200, &got_b) == 0);
  assert(hc_storage_core_read_flush(&read) == 0);
  assert(got_a.index == 100 && got_a.size == 1);
  assert(got_b.index == 200 && got_b.size == 2);
  assert(memcmp(got_a.hash, a.hash, sizeof(a.hash)) == 0);
  assert(memcmp(got_b.hash, b.hash, sizeof(b.hash)) == 0);

  // Delete one, verify the other survives.
  assert(hc_storage_core_write(&storage, &write, -1) == 0);
  assert(hc_storage_core_write_delete_tree_node(&write, 100) == 0);
  assert(hc_storage_core_write_flush(&write) == 0);

  assert(hc_storage_core_read(&storage, &read, -1) == 0);
  hc_merkle_tree_node_t after_a = {0};
  hc_merkle_tree_node_t after_b = {0};
  assert(hc_storage_core_read_get_tree_node(&read, 100, &after_a) == 0);
  assert(hc_storage_core_read_get_tree_node(&read, 200, &after_b) == 0);
  assert(hc_storage_core_read_flush(&read) == 0);
  // 100 was deleted; out buffer remains zeroed.
  assert(after_a.index == 0 && after_a.size == 0);
  // 200 still present.
  assert(after_b.index == 200 && after_b.size == 2);

  hc_storage_core_destroy(&storage);
  return 0;
}
