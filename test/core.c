#include <assert.h>
#include <string.h>

#include <flattree.h>

#include "hc/core.h"
#include "hc/db.h"
#include "hc/merkle_tree.h"

#define N 2000

int
main () {
  hc_core_t core;
  assert(hc_core_init(&core, 0, 0) == 0);

  uint8_t block_data[64];
  memset(block_data, 0xcd, sizeof(block_data));

  hc_buf_t block = {.len = sizeof(block_data), .buffer = block_data};

  for (size_t i = 0; i < N; i++) {
    hc_core_upgrade_t upgrade;
    hc__db_core_write_t write;

    hc_core_upgrade_init(&upgrade, &core);
    hc__db_core_write_init(&write, &core.db);

    assert(hc_merkle_tree_append(&upgrade, &write, &block, 1) == 0);
    assert(hc__db_core_write_flush(&write) == 0);

    hc_core_commit(&upgrade);

    hc__db_core_write_destroy(&write);
    hc_core_upgrade_destroy(&upgrade);
  }

  assert(core.length == N);
  assert(core.byte_length == N * sizeof(block_data));

  uint64_t expected[64];
  int nroots = flat_tree_full_roots(N * 2, expected);
  assert((size_t) nroots == core.roots.length);
  for (int i = 0; i < nroots; i++) {
    assert(core.roots.buffers[i].index == expected[i]);
  }

  hc_core_destroy(&core);
  return 0;
}
