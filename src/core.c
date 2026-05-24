#include <stdlib.h>
#include <string.h>

#include <flattree.h>

#include "hc/core.h"
#include "hc/db.h"
#include "hc/merkle_tree.h"

int
hc_core_init (hc_core_t *core, uint64_t core_ptr, uint64_t data_ptr, hc__db_t *db, const hc_hash_t key, const hc_hash_t discovery_key) {
  hc__db_core_init(&core->db, core_ptr, data_ptr, db);
  memcpy(core->key, key, sizeof(hc_hash_t));
  memcpy(core->discovery_key, discovery_key, sizeof(hc_hash_t));
  core->manifest = NULL;
  hc__array_init(&core->roots);
  core->fork = 0;
  core->length = 0;
  core->byte_length = 0;
  return 0;
}

void
hc_core_destroy (hc_core_t *core) {
  if (core->manifest) {
    hc_manifest_destroy(core->manifest);
    free(core->manifest);
    core->manifest = NULL;
  }
  hc__array_destroy(&core->roots);
  hc__db_core_destroy(&core->db);
}

static int
hc_core_append_work (hc_core_upgrade_t *upgrade, hc__db_core_write_t *write, const hc_buf_t *buffers, size_t count) {
  int err = 0;

  err = hc_merkle_tree_append(upgrade, write, buffers, count);
  if (err < 0) return err;

  err = hc__db_core_write_ensure_blocks(write, count);
  if (err < 0) return err;

  for (size_t i = 0; i < count; i++) {
    err = hc__db_core_write_block(write, upgrade->core->length + i, buffers[i]);
    if (err < 0) return err;
  }

  hc_head_t head = {
    .fork = upgrade->core->fork,
    .length = upgrade->length,
    .signature = {0, NULL},
    .timestamp = 0,
  };
  hc_crypto_tree(head.root_hash, (const hc_crypto_node_t *) upgrade->roots.buffers, upgrade->roots.length);

  err = hc__db_core_write_head(write, &head);
  if (err < 0) return err;

  return hc__db_core_write_flush(write);
}

int
hc_core_append (hc_core_t *core, const hc_buf_t *buffers, size_t count) {
  hc_core_upgrade_t upgrade;
  hc__db_core_write_t write;

  hc_core_upgrade_init(&upgrade, core);
  hc__db_core_write_init(&write, &core->db);

  int err = hc_core_append_work(&upgrade, &write, buffers, count);

  if (err == 0) hc_core_commit(&upgrade);

  hc__db_core_write_destroy(&write);
  hc_core_upgrade_destroy(&upgrade);
  return err;
}

int
hc_core_checkout (hc_core_t *core, uint64_t length) {
  // TODO: load roots for `length` from storage and recompute byte_length.
  core->length = length;
  return 0;
}

int
hc_core_load (hc_core_t *core) {
  hc_small_key_t head_key;
  hc_key_core_head(&head_key, core->db.data_ptr);

  rocksdb_read_t read;
  read.type = rocksdb_get;
  read.column_family = core->db.db->cf;
  read.key = rocksdb_slice_init((const char *) head_key.buf.buffer, head_key.buf.len);
  read.value = rocksdb_slice_empty();

  rocksdb_read_batch_t batch;
  int err = rocksdb_read(&core->db.db->rocks, &batch, &read, 1, NULL, NULL);
  if (err == 0 && batch.errors != NULL && batch.errors[0] != NULL) err = -1;

  if (err < 0) {
    if (read.value.data) rocksdb_slice_destroy(&read.value);
    rocksdb_read_cleanup(&batch);
    return err;
  }

  if (read.value.data == NULL) {
    rocksdb_read_cleanup(&batch);
    return 0;
  }

  hc_head_t head = {0};
  compact_state_t vs = {0, read.value.len, (uint8_t *) (uintptr_t) read.value.data};
  err = hc_head_decode(&vs, &head);
  rocksdb_slice_destroy(&read.value);
  rocksdb_read_cleanup(&batch);
  if (err < 0) return err;

  core->fork = head.fork;
  core->length = head.length;
  hc_head_destroy(&head);

  if (core->length == 0) return 0;

  uint64_t root_indices[64];
  int nroots = flat_tree_full_roots(core->length * 2, root_indices);
  if (nroots < 0) return -1;

  if (hc__array_grow(&core->roots, (size_t) nroots) < 0) return -1;

  hc__db_core_read_t db_read;
  hc__db_core_read_init(&db_read, &core->db);

  for (int i = 0; i < nroots; i++) {
    if (hc__db_core_read_get_tree_node(&db_read, root_indices[i], &core->roots.buffers[i]) < 0) {
      hc__db_core_read_destroy(&db_read);
      return -1;
    }
  }

  err = hc__db_core_read_flush(&db_read);
  hc__db_core_read_destroy(&db_read);
  if (err < 0) return err;

  core->roots.length = (size_t) nroots;

  core->byte_length = 0;
  for (int i = 0; i < nroots; i++) {
    core->byte_length += core->roots.buffers[i].size;
  }

  return 0;
}

int
hc_core_upgrade_init (hc_core_upgrade_t *upgrade, hc_core_t *core) {
  upgrade->core = core;
  hc__array_init(&upgrade->roots);
  upgrade->length = 0;
  upgrade->byte_length = 0;
  return 0;
}

void
hc_core_upgrade_destroy (hc_core_upgrade_t *upgrade) {
  hc__array_destroy(&upgrade->roots);
}

void
hc_core_commit (hc_core_upgrade_t *upgrade) {
  hc_core_t *core = upgrade->core;
  hc__array_destroy(&core->roots);
  core->roots = upgrade->roots;
  core->length = upgrade->length;
  core->byte_length = upgrade->byte_length;

  hc__array_init(&upgrade->roots);
  upgrade->length = 0;
  upgrade->byte_length = 0;
}
