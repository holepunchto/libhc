#include <assert.h>
#include <string.h>

#include <flattree.h>
#include <uv.h>

#include "hc/core.h"
#include "hc/db.h"
#include "tmp.h"

#define N 2000

int
main () {
  char dir[2048];
  assert(hc_test_mkdtemp(dir, sizeof(dir), "libhc-core") == 0);

  hc__db_t db;
  assert(hc__db_init(&db, dir, uv_default_loop()) == 0);

  hc_core_t core;
  assert(hc_core_init(&core, 0, 0, &db, (const uint8_t[32]){0}, (const uint8_t[32]){0}) == 0);

  uint8_t block_data[64];
  memset(block_data, 0xcd, sizeof(block_data));

  hc_buf_t block = {.len = sizeof(block_data), .buffer = block_data};

  for (size_t i = 0; i < N; i++) {
    assert(hc_core_append(&core, &block, 1) == 0);
  }

  assert(core.length == N);
  assert(core.byte_length == N * sizeof(block_data));

  uint64_t expected[64];
  int nroots = flat_tree_full_roots(N * 2, expected);
  assert((size_t) nroots == core.roots.length);

  uint64_t roots_byte_length = 0;
  for (int i = 0; i < nroots; i++) {
    assert(core.roots.buffers[i].index == expected[i]);
    roots_byte_length += core.roots.buffers[i].size;
  }
  assert(roots_byte_length == core.byte_length);

  hc_core_destroy(&core);
  hc__db_destroy(&db);
  hc_test_rmdir(dir);
  return 0;
}
