#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#include "hc/core.h"
#include "hc/db.h"
#include "hc/merkle_tree.h"

#define N 100000

static uint64_t
now_ns () {
  struct timespec ts;
  clock_gettime(CLOCK_MONOTONIC, &ts);
  return (uint64_t) ts.tv_sec * 1000000000ULL + ts.tv_nsec;
}

int
main () {
  hc_core_t core;
  assert(hc_core_init(&core, 0, 0, (const uint8_t[32]){0}, (const uint8_t[32]){0}) == 0);

  uint8_t block_data[1024];
  memset(block_data, 0xcd, sizeof(block_data));

  hc_buf_t block = {.len = sizeof(block_data), .buffer = block_data};

  uint64_t t0 = now_ns();

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

  uint64_t elapsed_ns = now_ns() - t0;
  double elapsed_ms = elapsed_ns / 1e6;
  double per_block_us = elapsed_ns / 1e3 / N;

  printf("appended %d blocks in %.2f ms (%.2f us/block)\n", N, elapsed_ms, per_block_us);

  hc_core_destroy(&core);
  return 0;
}
