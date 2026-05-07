#include <assert.h>
#include <stdlib.h>
#include <string.h>

#include <compact.h>

#include "hc/keys.h"
#include "hc/schema.h"
#include "hc/storage.h"

// Encodes a tree-node value into a freshly malloc'd buffer. Caller frees
// after the write batch flushes.
static hc_buf_t
encode_tree_node (const hc_merkle_tree_node_t *node) {
  compact_state_t s = {0, 0, NULL};
  hc_schema_preencode_tree_node(&s, node);
  s.buffer = malloc(s.end);
  hc_schema_encode_tree_node(&s, node);
  return (hc_buf_t){s.start, s.buffer};
}

// Decode a value buffer (populated by hc_read_batch_flush) into a node.
static void
decode_tree_node (hc_buf_t value, hc_merkle_tree_node_t *out) {
  compact_state_t s = {0, value.len, value.buffer};
  hc_schema_decode_tree_node(&s, out);
}

int
main () {
  hc_storage_core_t storage;
  assert(hc_storage_core_init(&storage, 0, 0) == 0);

  // Put a tree node.
  hc_merkle_tree_node_t node = {.index = 7, .size = 42};
  memset(node.hash, 0xab, sizeof(node.hash));

  {
    hc_write_batch_t write;
    hc_write_batch_init(&write, &storage.kv, 4);
    hc_small_key_t k0;
    hc_key_core_tree(&k0, storage.data_ptr, 7);
    hc_buf_t v0 = encode_tree_node(&node);
    assert(hc_write_batch_put(&write, k0.buf, v0) == 0);
    assert(hc_write_batch_flush(&write) == 0);
    hc_write_batch_destroy(&write);
    free(v0.buffer);
  }

  // Read it back.
  {
    hc_read_batch_t read;
    hc_read_batch_init(&read, &storage.kv, 4);
    hc_small_key_t k0;
    hc_key_core_tree(&k0, storage.data_ptr, 7);
    hc_buf_t v0;
    assert(hc_read_batch_get(&read, k0.buf, &v0) == 0);
    assert(hc_read_batch_flush(&read) == 0);

    assert(v0.buffer != NULL);
    hc_merkle_tree_node_t got = {0};
    decode_tree_node(v0, &got);
    free(v0.buffer);
    hc_read_batch_destroy(&read);

    assert(got.index == 7);
    assert(got.size == 42);
    assert(memcmp(got.hash, node.hash, sizeof(node.hash)) == 0);
  }

  // Multiple ops in one batch.
  hc_merkle_tree_node_t a = {.index = 100, .size = 1};
  hc_merkle_tree_node_t b = {.index = 200, .size = 2};
  memset(a.hash, 0x11, sizeof(a.hash));
  memset(b.hash, 0x22, sizeof(b.hash));

  {
    hc_write_batch_t write;
    hc_write_batch_init(&write, &storage.kv, 4);
    hc_small_key_t ka, kb;
    hc_key_core_tree(&ka, storage.data_ptr, 100);
    hc_key_core_tree(&kb, storage.data_ptr, 200);
    hc_buf_t va = encode_tree_node(&a);
    hc_buf_t vb = encode_tree_node(&b);
    assert(hc_write_batch_put(&write, ka.buf, va) == 0);
    assert(hc_write_batch_put(&write, kb.buf, vb) == 0);
    assert(hc_write_batch_flush(&write) == 0);
    hc_write_batch_destroy(&write);
    free(va.buffer);
    free(vb.buffer);
  }

  {
    hc_read_batch_t read;
    hc_read_batch_init(&read, &storage.kv, 4);
    hc_small_key_t ka, kb;
    hc_key_core_tree(&ka, storage.data_ptr, 100);
    hc_key_core_tree(&kb, storage.data_ptr, 200);
    hc_buf_t va, vb;
    assert(hc_read_batch_get(&read, ka.buf, &va) == 0);
    assert(hc_read_batch_get(&read, kb.buf, &vb) == 0);
    assert(hc_read_batch_flush(&read) == 0);

    assert(va.buffer != NULL && vb.buffer != NULL);
    hc_merkle_tree_node_t got_a = {0};
    hc_merkle_tree_node_t got_b = {0};
    decode_tree_node(va, &got_a);
    decode_tree_node(vb, &got_b);
    free(va.buffer);
    free(vb.buffer);
    hc_read_batch_destroy(&read);

    assert(got_a.index == 100 && got_a.size == 1);
    assert(got_b.index == 200 && got_b.size == 2);
    assert(memcmp(got_a.hash, a.hash, sizeof(a.hash)) == 0);
    assert(memcmp(got_b.hash, b.hash, sizeof(b.hash)) == 0);
  }

  // Delete one, verify the other survives and the deleted one returns NULL.
  {
    hc_write_batch_t write;
    hc_write_batch_init(&write, &storage.kv, 4);
    hc_small_key_t kdel;
    hc_key_core_tree(&kdel, storage.data_ptr, 100);
    assert(hc_write_batch_del(&write, kdel.buf) == 0);
    assert(hc_write_batch_flush(&write) == 0);
    hc_write_batch_destroy(&write);
  }

  {
    hc_read_batch_t read;
    hc_read_batch_init(&read, &storage.kv, 4);
    hc_small_key_t ka, kb;
    hc_key_core_tree(&ka, storage.data_ptr, 100);
    hc_key_core_tree(&kb, storage.data_ptr, 200);
    hc_buf_t va = {0};
    hc_buf_t vb = {0};
    hc_read_batch_get(&read, ka.buf, &va);
    hc_read_batch_get(&read, kb.buf, &vb);
    assert(hc_read_batch_flush(&read) == 0);

    assert(va.buffer == NULL); // deleted
    assert(vb.buffer != NULL); // still present
    hc_merkle_tree_node_t got_b = {0};
    decode_tree_node(vb, &got_b);
    free(vb.buffer);
    hc_read_batch_destroy(&read);

    assert(got_b.index == 200 && got_b.size == 2);
  }

  hc_storage_core_destroy(&storage);
  return 0;
}
