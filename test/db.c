#include <assert.h>
#include <stdlib.h>
#include <string.h>

#include <kv.h>

#include "hc/db.h"
#include "hc/keys.h"
#include "hc/schema.h"

static void
decode_tree_node (hc_buf_t value, hc_merkle_tree_node_t *out) {
  compact_state_t s = {0, value.len, value.buffer};
  hc_schema_decode_tree_node(&s, out);
}

int
main () {
  hc__db_core_t db;
  assert(hc__db_core_init(&db, 0, 0) == 0);

  // Write a single tree node via hc__db_core_write_t.
  hc_merkle_tree_node_t node = {.index = 7, .size = 42};
  memset(node.hash, 0xab, sizeof(node.hash));

  {
    hc__db_core_write_t write;
    hc__db_core_write_init(&write, &db);
    assert(hc__db_core_write_tree_node(&write, &node) == 0);
    assert(hc__db_core_write_flush(&write) == 0);
    hc__db_core_write_destroy(&write);
  }

  // Read it back directly via kv.
  {
    hc_small_key_t k0;
    hc_key_core_tree(&k0, db.data_ptr, 7);

    kv_read_batch_t read;
    kv_read_batch_init(&read, &db.kv, 1);
    hc_buf_t v0 = {0};
    assert(kv_read_batch_get(&read, k0.buf.buffer, k0.buf.len, &v0.buffer, &v0.len) == 0);
    assert(kv_read_batch_flush(&read) == 0);

    assert(v0.buffer != NULL);
    hc_merkle_tree_node_t got = {0};
    decode_tree_node(v0, &got);
    free(v0.buffer);
    kv_read_batch_destroy(&read);

    assert(got.index == 7);
    assert(got.size == 42);
    assert(memcmp(got.hash, node.hash, sizeof(node.hash)) == 0);
  }

  // Write multiple tree nodes in one batch.
  hc_merkle_tree_node_t a = {.index = 100, .size = 1};
  hc_merkle_tree_node_t b = {.index = 200, .size = 2};
  memset(a.hash, 0x11, sizeof(a.hash));
  memset(b.hash, 0x22, sizeof(b.hash));

  {
    hc__db_core_write_t write;
    hc__db_core_write_init(&write, &db);
    assert(hc__db_core_write_tree_node(&write, &a) == 0);
    assert(hc__db_core_write_tree_node(&write, &b) == 0);
    assert(hc__db_core_write_flush(&write) == 0);
    hc__db_core_write_destroy(&write);
  }

  {
    hc_small_key_t ka, kb;
    hc_key_core_tree(&ka, db.data_ptr, 100);
    hc_key_core_tree(&kb, db.data_ptr, 200);

    kv_read_batch_t read;
    kv_read_batch_init(&read, &db.kv, 2);
    hc_buf_t va = {0}, vb = {0};
    assert(kv_read_batch_get(&read, ka.buf.buffer, ka.buf.len, &va.buffer, &va.len) == 0);
    assert(kv_read_batch_get(&read, kb.buf.buffer, kb.buf.len, &vb.buffer, &vb.len) == 0);
    assert(kv_read_batch_flush(&read) == 0);

    assert(va.buffer != NULL && vb.buffer != NULL);
    hc_merkle_tree_node_t got_a = {0}, got_b = {0};
    decode_tree_node(va, &got_a);
    decode_tree_node(vb, &got_b);
    free(va.buffer);
    free(vb.buffer);
    kv_read_batch_destroy(&read);

    assert(got_a.index == 100 && got_a.size == 1);
    assert(got_b.index == 200 && got_b.size == 2);
    assert(memcmp(got_a.hash, a.hash, sizeof(a.hash)) == 0);
    assert(memcmp(got_b.hash, b.hash, sizeof(b.hash)) == 0);
  }

  hc__db_core_destroy(&db);
  return 0;
}
