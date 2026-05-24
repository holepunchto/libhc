#include <assert.h>
#include <stdio.h>
#include <string.h>

#include <uv.h>

#include "hc/core.h"
#include "hc/db.h"
#include "../test/tmp.h"

#define N 100000

static uint64_t
now_ns () {
  uv_timespec64_t ts;
  uv_clock_gettime(UV_CLOCK_MONOTONIC, &ts);
  return (uint64_t) ts.tv_sec * 1000000000ULL + (uint64_t) ts.tv_nsec;
}

int
main () {
  char dir[2048];
  assert(hc_test_mkdtemp(dir, sizeof(dir), "libhc-bench-append") == 0);

  hc__db_t db;
  assert(hc__db_init(&db, dir, uv_default_loop()) == 0);

  hc_core_t core;
  assert(hc_core_init(&core, 0, 0, &db, (const uint8_t[32]){0}, (const uint8_t[32]){0}) == 0);

  uint8_t block_data[1024];
  memset(block_data, 0xcd, sizeof(block_data));

  hc_buf_t block = {.len = sizeof(block_data), .buffer = block_data};

  uint64_t t0 = now_ns();

  for (size_t i = 0; i < N; i++) {
    assert(hc_core_append(&core, &block, 1) == 0);
  }

  uint64_t elapsed_ns = now_ns() - t0;
  double elapsed_ms = elapsed_ns / 1e6;
  double per_block_us = elapsed_ns / 1e3 / N;

  printf("appended %d blocks in %.2f ms (%.2f us/block)\n", N, elapsed_ms, per_block_us);

  hc_core_destroy(&core);
  hc__db_destroy(&db);
  hc_test_rmdir(dir);
  return 0;
}
