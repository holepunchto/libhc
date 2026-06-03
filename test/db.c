#include <assert.h>
#include <stdlib.h>
#include <string.h>

#include <rocksdb.h>
#include <uv.h>

#include "hc/db.h"
#include "hc/keys.h"
#include "tmp.h"


static void
decode_tree_node (uint8_t *data, size_t len, hc_tree_node_t *out) {
  compact_state_t s = {0, len, data};
  hc_tree_node_decode(&s, out);
}

// Read a single key directly via rocksdb. Returns the value (caller frees
// via rocksdb_slice_destroy) or {NULL, 0} if not found.
static rocksdb_slice_t
read_key (hc__db_core_t *db, const uint8_t *key, size_t key_len) {
  rocksdb_read_t r;
  r.type = rocksdb_get;
  r.column_family = db->db->cf;
  r.key = rocksdb_slice_init((const char *) key, key_len);
  r.value = rocksdb_slice_empty();

  rocksdb_read_batch_t batch;
  assert(rocksdb_read(&db->db->rocks, &batch, &r, 1, NULL, NULL) == 0);
  rocksdb_read_cleanup(&batch);
  return r.value;
}

int
main () {
  char dir[2048];
  assert(hc_test_mkdtemp(dir, sizeof(dir), "libhc-db") == 0);

  hc__db_t store;
  assert(hc__db_init(&store, dir, uv_default_loop()) == 0);

  hc__db_core_t db;
  assert(hc__db_core_init(&db, 0, 0, &store) == 0);

  // Write a single tree node via hc__db_core_write_t.
  hc_tree_node_t node = {.index = 7, .size = 42};
  memset(node.hash, 0xab, sizeof(node.hash));

  {
    hc__db_core_write_t write;
    hc__db_core_write_init(&write, &db);
    assert(hc__db_core_write_tree_node(&write, &node) == 0);
    assert(hc__db_core_write_flush(&write) == 0);
    hc__db_core_write_destroy(&write);
  }

  // Read it back directly via rocksdb.
  {
    hc_small_key_t k0;
    hc_key_core_tree(&k0, db.data_ptr, 7);

    rocksdb_slice_t v0 = read_key(&db, k0.buf.buffer, k0.buf.len);
    assert(v0.data != NULL);

    hc_tree_node_t got = {0};
    decode_tree_node((uint8_t *) (uintptr_t) v0.data, v0.len, &got);
    rocksdb_slice_destroy(&v0);

    assert(got.index == 7);
    assert(got.size == 42);
    assert(memcmp(got.hash, node.hash, sizeof(node.hash)) == 0);
  }

  // Write multiple tree nodes in one batch.
  hc_tree_node_t a = {.index = 100, .size = 1};
  hc_tree_node_t b = {.index = 200, .size = 2};
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

    rocksdb_slice_t va = read_key(&db, ka.buf.buffer, ka.buf.len);
    rocksdb_slice_t vb = read_key(&db, kb.buf.buffer, kb.buf.len);
    assert(va.data != NULL && vb.data != NULL);

    hc_tree_node_t got_a = {0}, got_b = {0};
    decode_tree_node((uint8_t *) (uintptr_t) va.data, va.len, &got_a);
    decode_tree_node((uint8_t *) (uintptr_t) vb.data, vb.len, &got_b);
    rocksdb_slice_destroy(&va);
    rocksdb_slice_destroy(&vb);

    assert(got_a.index == 100 && got_a.size == 1);
    assert(got_b.index == 200 && got_b.size == 2);
    assert(memcmp(got_a.hash, a.hash, sizeof(a.hash)) == 0);
    assert(memcmp(got_b.hash, b.hash, sizeof(b.hash)) == 0);
  }

  hc__db_core_destroy(&db);
  hc__db_destroy(&store);
  hc_test_rmdir(dir);
  return 0;
}
