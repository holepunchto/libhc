#ifndef HC_DB_H
#define HC_DB_H

#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>

#include <rocksdb.h>
#include <uv.h>

#include "array.h"
#include "buffer.h"
#include "keys.h"
#include "merkle_tree.h"
#include "schema.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct hc_store_head_s {
  uint64_t cores;
  uint64_t datas;
  uint64_t groups;
  uint8_t has_seed;
  hc_hash_t seed;
  uint8_t has_default_discovery_key;
  hc_hash_t default_discovery_key;
} hc_store_head_t;

// Result type for store-by-discovery-key lookups. found is 0 if the entry
// was not present in the kv.
typedef struct hc_store_core_s {
  uint8_t found;
  uint64_t core_ptr;
  uint64_t data_ptr;
} hc_store_core_t;

// Bundle of a rocksdb handle and the active column family. Owned by an
// hc_store_t; borrowed (by pointer) by per-core db handles. The store
// only ever talks to one (rocks, cf) combo, so they travel together.
typedef struct hc__db_s hc__db_t;

struct hc__db_s {
  rocksdb_t rocks;
  rocksdb_column_family_t *cf;
};

// Opens the rocksdb at `path` synchronously and stashes a handle to the
// `corestore` column family — same name and tuning as the JS
// hypercore-storage uses, so on-disk layout is interoperable.
//
// `loop` is used by librocksdb's worker machinery; in sync mode (cb ==
// NULL) it isn't actually serviced but is still required by the API.
static inline int
hc__db_init (hc__db_t *db, const char *path, uv_loop_t *loop) {
  if (path == NULL) return -1;

  rocksdb_options_t opts = {
    .create_if_missing = true,
    .create_missing_column_families = true,
  };

  // Mirror hypercore-storage/lib/index.js createColumnFamily: blob files
  // for large values, 8KB table blocks, index/filter blocks cached.
  rocksdb_column_family_options_t cf_opts = {
    .version = 5,
    .compaction_style = rocksdb_level_compaction,
    .enable_blob_files = true,
    .min_blob_size = 4096,
    .blob_file_size = 256ULL * 1024 * 1024,
    .enable_blob_garbage_collection = true,
    .block_size = 8 * 1024,
    .cache_index_and_filter_blocks = true,
    .format_version = 6,
  };

  rocksdb_column_family_descriptor_t descs[2] = {
    rocksdb_column_family_descriptor("default", NULL),
    rocksdb_column_family_descriptor("corestore", &cf_opts),
  };
  rocksdb_column_family_t *handles[2] = {NULL, NULL};

  rocksdb_open_t req;
  int rc = rocksdb_open(loop, &db->rocks, &req, path, &opts, descs, handles, 2, NULL, NULL);
  int err = (rc < 0 || req.error != NULL) ? -1 : 0;
  rocksdb_open_cleanup(&req);
  if (err < 0) return err;

  // We only operate on the corestore CF. The default CF must be opened
  // (rocksdb requires every existing CF to appear in the descriptor
  // list), but we don't use it — destroy the handle right away.
  // rocksdb_column_family_destroy frees the handle, not the data.
  rocksdb_column_family_destroy(&db->rocks, handles[0]);
  db->cf = handles[1];

  return 0;
}

static inline void
hc__db_destroy (hc__db_t *db) {
  rocksdb_column_family_destroy(&db->rocks, db->cf);

  rocksdb_close_t req;
  rocksdb_close(&db->rocks, &req, NULL, NULL);
  rocksdb_close_cleanup(&req);
}

typedef struct hc__db_core_s hc__db_core_t;

struct hc__db_core_s {
  hc__db_t *db;
  uint64_t core_ptr;
  uint64_t data_ptr;
};

static inline int
hc__db_core_init (hc__db_core_t *core, uint64_t core_ptr, uint64_t data_ptr, hc__db_t *db) {
  core->db = db;
  core->core_ptr = core_ptr;
  core->data_ptr = data_ptr;
  return 0;
}

static inline void
hc__db_core_destroy (hc__db_core_t *db) {
  (void) db;
}

// Stack-allocatable key+value pair for a tree-node kv record.
typedef struct {
  hc_small_key_t key;
  hc_buf_t value;
  uint8_t value_data[HC_MERKLE_TREE_NODE_MAX_SIZE];
} hc__db_tree_node_kv_t;

// Block kv record. Value is variable-size and caller-owned; the bytes must
// remain valid until the write is flushed.
typedef struct {
  hc_small_key_t key;
  hc_buf_t value;
} hc__db_block_kv_t;

typedef struct {
  hc_small_key_t key;
  hc_buf_t value;
  uint8_t value_data[HC_HEAD_MAX_SIZE];
} hc__db_core_head_kv_t;

typedef struct hc__db_core_write_s hc__db_core_write_t;
struct hc__db_core_write_s {
  hc__db_core_t *db;
  HC__ARRAY(hc__db_tree_node_kv_t) tree_nodes;
  HC__ARRAY(hc__db_block_kv_t) blocks;
  HC__ARRAY(hc_small_key_t) deletes;
  hc__db_core_head_kv_t head;
  uint8_t has_head;
};

static inline void
hc__db_core_write_init (hc__db_core_write_t *write, hc__db_core_t *db) {
  write->db = db;
  hc__array_init(&write->tree_nodes);
  hc__array_init(&write->blocks);
  hc__array_init(&write->deletes);
  write->has_head = 0;
}

static inline int
hc__db_core_write_ensure_tree_nodes (hc__db_core_write_t *write, size_t capacity) {
  return hc__array_grow(&write->tree_nodes, write->tree_nodes.length + capacity);
}

static inline int
hc__db_core_write_ensure_blocks (hc__db_core_write_t *write, size_t capacity) {
  return hc__array_grow(&write->blocks, write->blocks.length + capacity);
}

static inline int
hc__db_core_write_ensure_deletes (hc__db_core_write_t *write, size_t capacity) {
  return hc__array_grow(&write->deletes, write->deletes.length + capacity);
}

static inline void
hc__db_core_write_destroy (hc__db_core_write_t *write) {
  hc__array_destroy(&write->tree_nodes);
  hc__array_destroy(&write->blocks);
  hc__array_destroy(&write->deletes);
}

static inline int
hc__db_core_write_flush (hc__db_core_write_t *write) {
  size_t total = write->tree_nodes.length + write->blocks.length + write->deletes.length + (write->has_head ? 1 : 0);
  if (total == 0) return 0;

  rocksdb_write_t *writes = (rocksdb_write_t *) malloc(total * sizeof(rocksdb_write_t));
  if (writes == NULL) return -1;

  size_t i = 0;

  for (size_t j = 0; j < write->tree_nodes.length; j++) {
    hc__db_tree_node_kv_t *kv = &write->tree_nodes.buffers[j];
    writes[i].type = rocksdb_put;
    writes[i].column_family = write->db->db->cf;
    writes[i].key = rocksdb_slice_init((const char *) kv->key.buf.buffer, kv->key.buf.len);
    writes[i].value = rocksdb_slice_init((const char *) kv->value.buffer, kv->value.len);
    i++;
  }

  for (size_t j = 0; j < write->blocks.length; j++) {
    hc__db_block_kv_t *kv = &write->blocks.buffers[j];
    writes[i].type = rocksdb_put;
    writes[i].column_family = write->db->db->cf;
    writes[i].key = rocksdb_slice_init((const char *) kv->key.buf.buffer, kv->key.buf.len);
    writes[i].value = rocksdb_slice_init((const char *) kv->value.buffer, kv->value.len);
    i++;
  }

  for (size_t j = 0; j < write->deletes.length; j++) {
    hc_small_key_t *key = &write->deletes.buffers[j];
    writes[i].type = rocksdb_delete;
    writes[i].column_family = write->db->db->cf;
    writes[i].key = rocksdb_slice_init((const char *) key->buf.buffer, key->buf.len);
    writes[i].value = rocksdb_slice_empty();
    i++;
  }

  if (write->has_head) {
    hc__db_core_head_kv_t *kv = &write->head;
    writes[i].type = rocksdb_put;
    writes[i].column_family = write->db->db->cf;
    writes[i].key = rocksdb_slice_init((const char *) kv->key.buf.buffer, kv->key.buf.len);
    writes[i].value = rocksdb_slice_init((const char *) kv->value.buffer, kv->value.len);
    i++;
  }

  rocksdb_write_batch_t batch;
  int rc = rocksdb_write(&write->db->db->rocks, &batch, writes, total, NULL, NULL);
  int err = (rc < 0 || batch.error != NULL) ? -1 : 0;
  rocksdb_write_cleanup(&batch);
  free(writes);
  return err;
}

static inline int
hc__db_core_write_head (hc__db_core_write_t *write, const hc_head_t *head) {
  hc_key_core_head(&write->head.key, write->db->data_ptr);
  compact_state_t state = {0, sizeof(write->head.value_data), write->head.value_data};
  hc_head_encode(&state, head);
  write->head.value.buffer = write->head.value_data;
  write->head.value.len = state.start;
  write->has_head = 1;
  return 0;
}

static inline int
hc__db_core_write_tree_node (hc__db_core_write_t *write, const hc_merkle_tree_node_t *node) {
  if (hc__array_grow(&write->tree_nodes, write->tree_nodes.length + 1) < 0) return -1;

  hc__db_tree_node_kv_t *kv = &write->tree_nodes.buffers[write->tree_nodes.length++];
  hc_key_core_tree(&kv->key, write->db->data_ptr, node->index);

  compact_state_t state = {0, sizeof(kv->value_data), kv->value_data};
  hc_tree_node_encode(&state, node);
  kv->value.buffer = kv->value_data;
  kv->value.len = state.start;

  return 0;
}

// The bytes referenced by `value` must remain valid until hc__db_core_write_flush.
static inline int
hc__db_core_write_block (hc__db_core_write_t *write, uint64_t index, hc_buf_t value) {
  if (hc__array_grow(&write->blocks, write->blocks.length + 1) < 0) return -1;

  hc__db_block_kv_t *kv = &write->blocks.buffers[write->blocks.length++];
  hc_key_core_block(&kv->key, write->db->data_ptr, index);
  kv->value = value;

  return 0;
}

static inline int
hc__db_core_write_del_tree_node (hc__db_core_write_t *write, uint64_t index) {
  if (hc__array_grow(&write->deletes, write->deletes.length + 1) < 0) return -1;
  hc_key_core_tree(&write->deletes.buffers[write->deletes.length++], write->db->data_ptr, index);
  return 0;
}

static inline int
hc__db_core_write_del_block (hc__db_core_write_t *write, uint64_t index) {
  if (hc__array_grow(&write->deletes, write->deletes.length + 1) < 0) return -1;
  hc_key_core_block(&write->deletes.buffers[write->deletes.length++], write->db->data_ptr, index);
  return 0;
}

typedef enum {
  HC__DB_READ_TREE_NODE,
  HC__DB_READ_BLOCK,
} hc__db_read_type_t;

// Queued read entry: key is stack-allocated, value is filled after flush,
// result is a caller-owned pointer where the decoded output will go.
typedef struct {
  hc__db_read_type_t type;
  hc_small_key_t key;
  hc_buf_t value;
  void *result;
} hc__db_small_read_t;

typedef struct {
  hc__db_core_t *db;
  HC__ARRAY(hc__db_small_read_t) small_reads;
} hc__db_core_read_t;

static inline void
hc__db_core_read_init (hc__db_core_read_t *read, hc__db_core_t *db) {
  read->db = db;
  hc__array_init(&read->small_reads);
}

static inline int
hc__db_core_read_ensure_tree_nodes (hc__db_core_read_t *read, size_t capacity) {
  return hc__array_grow(&read->small_reads, read->small_reads.length + capacity);
}

static inline int
hc__db_core_read_ensure_blocks (hc__db_core_read_t *read, size_t capacity) {
  return hc__array_grow(&read->small_reads, read->small_reads.length + capacity);
}

static inline void
hc__db_core_read_destroy (hc__db_core_read_t *read) {
  hc__array_destroy(&read->small_reads);
}

static inline int
hc__db_core_read_flush (hc__db_core_read_t *read) {
  size_t total = read->small_reads.length;
  if (total == 0) return 0;

  rocksdb_read_t *reads = (rocksdb_read_t *) malloc(total * sizeof(rocksdb_read_t));
  if (reads == NULL) return -1;

  for (size_t i = 0; i < total; i++) {
    hc__db_small_read_t *e = &read->small_reads.buffers[i];
    reads[i].type = rocksdb_get;
    reads[i].column_family = read->db->db->cf;
    reads[i].key = rocksdb_slice_init((const char *) e->key.buf.buffer, e->key.buf.len);
    reads[i].value = rocksdb_slice_empty();
  }

  rocksdb_read_batch_t batch;
  int rc = rocksdb_read(&read->db->db->rocks, &batch, reads, total, NULL, NULL);
  int err = (rc < 0) ? -1 : 0;

  for (size_t i = 0; i < total; i++) {
    hc__db_small_read_t *e = &read->small_reads.buffers[i];

    if (err == 0 && batch.errors != NULL && batch.errors[i] != NULL) err = -1;

    if (reads[i].value.data == NULL) continue;

    // Take ownership of the rocksdb-allocated bytes. const_cast is safe
    // here — librocksdb itself does it inside rocksdb_slice_destroy.
    e->value.buffer = (uint8_t *) (uintptr_t) reads[i].value.data;
    e->value.len = reads[i].value.len;
    reads[i].value.data = NULL;
    reads[i].value.len = 0;

    if (err < 0) {
      free(e->value.buffer);
      e->value.buffer = NULL;
      e->value.len = 0;
      continue;
    }

    if (e->type == HC__DB_READ_TREE_NODE) {
      compact_state_t state = {0, e->value.len, e->value.buffer};
      hc_tree_node_decode(&state, (hc_merkle_tree_node_t *) e->result);
      free(e->value.buffer);
    } else if (e->type == HC__DB_READ_BLOCK) {
      *(hc_buf_t *) e->result = e->value;
    }
  }

  rocksdb_read_cleanup(&batch);
  free(reads);
  return err;
}

static inline int
hc__db_core_read_get_tree_node (hc__db_core_read_t *read, uint64_t index, hc_merkle_tree_node_t *result) {
  if (hc__array_grow(&read->small_reads, read->small_reads.length + 1) < 0) return -1;
  hc__db_small_read_t *e = &read->small_reads.buffers[read->small_reads.length++];
  e->type = HC__DB_READ_TREE_NODE;
  e->value.buffer = NULL;
  e->value.len = 0;
  e->result = result;
  hc_key_core_tree(&e->key, read->db->data_ptr, index);
  return 0;
}

static inline int
hc__db_core_read_get_block (hc__db_core_read_t *read, uint64_t index, hc_buf_t *result) {
  if (hc__array_grow(&read->small_reads, read->small_reads.length + 1) < 0) return -1;
  hc__db_small_read_t *e = &read->small_reads.buffers[read->small_reads.length++];
  e->type = HC__DB_READ_BLOCK;
  e->value.buffer = NULL;
  e->value.len = 0;
  e->result = result;
  hc_key_core_block(&e->key, read->db->data_ptr, index);
  return 0;
}

// ===== store-level batches =====

typedef struct {
  hc_small_key_t key;
  hc_buf_t value;
  uint8_t value_data[HC_STORE_HEAD_MAX_SIZE];
} hc__db_store_head_kv_t;

typedef struct {
  hc_small_key_t key;
  hc_buf_t value;
  uint8_t value_data[HC_STORE_CORE_MAX_SIZE];
} hc__db_store_core_kv_t;

typedef struct hc__db_store_write_s hc__db_store_write_t;
struct hc__db_store_write_s {
  hc__db_t *db;
  hc__db_store_head_kv_t head;
  uint8_t has_head;
  HC__ARRAY(hc__db_store_core_kv_t) cores;
};

static inline void
hc__db_store_write_init (hc__db_store_write_t *write, hc__db_t *db) {
  write->db = db;
  write->has_head = 0;
  hc__array_init(&write->cores);
}

static inline void
hc__db_store_write_destroy (hc__db_store_write_t *write) {
  hc__array_destroy(&write->cores);
}

static inline int
hc__db_store_write_set_head (hc__db_store_write_t *write, const hc_store_head_t *head) {
  hc_key_store_head(&write->head.key);
  compact_state_t state = {0, sizeof(write->head.value_data), write->head.value_data};
  hc_store_head_encode(&state, head);
  write->head.value.buffer = write->head.value_data;
  write->head.value.len = state.start;
  write->has_head = 1;
  return 0;
}

static inline int
hc__db_store_write_put_core (hc__db_store_write_t *write, const hc_hash_t discovery_key, uint64_t core_ptr, uint64_t data_ptr) {
  if (hc__array_grow(&write->cores, write->cores.length + 1) < 0) return -1;
  hc__db_store_core_kv_t *kv = &write->cores.buffers[write->cores.length++];
  hc_key_store_core(&kv->key, discovery_key);
  compact_state_t state = {0, sizeof(kv->value_data), kv->value_data};
  hc_store_core_encode(&state, core_ptr, data_ptr);
  kv->value.buffer = kv->value_data;
  kv->value.len = state.start;
  return 0;
}

static inline int
hc__db_store_write_flush (hc__db_store_write_t *write) {
  size_t total = write->cores.length + (write->has_head ? 1 : 0);
  if (total == 0) return 0;

  rocksdb_write_t *writes = (rocksdb_write_t *) malloc(total * sizeof(rocksdb_write_t));
  if (writes == NULL) return -1;

  size_t i = 0;

  if (write->has_head) {
    hc__db_store_head_kv_t *kv = &write->head;
    writes[i].type = rocksdb_put;
    writes[i].column_family = write->db->cf;
    writes[i].key = rocksdb_slice_init((const char *) kv->key.buf.buffer, kv->key.buf.len);
    writes[i].value = rocksdb_slice_init((const char *) kv->value.buffer, kv->value.len);
    i++;
  }

  for (size_t j = 0; j < write->cores.length; j++) {
    hc__db_store_core_kv_t *kv = &write->cores.buffers[j];
    writes[i].type = rocksdb_put;
    writes[i].column_family = write->db->cf;
    writes[i].key = rocksdb_slice_init((const char *) kv->key.buf.buffer, kv->key.buf.len);
    writes[i].value = rocksdb_slice_init((const char *) kv->value.buffer, kv->value.len);
    i++;
  }

  rocksdb_write_batch_t batch;
  int rc = rocksdb_write(&write->db->rocks, &batch, writes, total, NULL, NULL);
  int err = (rc < 0 || batch.error != NULL) ? -1 : 0;
  rocksdb_write_cleanup(&batch);
  free(writes);
  return err;
}

typedef enum {
  HC__DB_STORE_READ_HEAD,
  HC__DB_STORE_READ_CORE,
} hc__db_store_read_type_t;

typedef struct {
  hc__db_store_read_type_t type;
  hc_small_key_t key;
  hc_buf_t value;
  void *result;
} hc__db_store_small_read_t;

typedef struct {
  hc__db_t *db;
  HC__ARRAY(hc__db_store_small_read_t) small_reads;
} hc__db_store_read_t;

static inline void
hc__db_store_read_init (hc__db_store_read_t *read, hc__db_t *db) {
  read->db = db;
  hc__array_init(&read->small_reads);
}

static inline void
hc__db_store_read_destroy (hc__db_store_read_t *read) {
  hc__array_destroy(&read->small_reads);
}

// Result is populated to a zero-initialized head on miss (i.e. fresh store).
static inline int
hc__db_store_read_get_head (hc__db_store_read_t *read, hc_store_head_t *result) {
  if (hc__array_grow(&read->small_reads, read->small_reads.length + 1) < 0) return -1;
  hc__db_store_small_read_t *e = &read->small_reads.buffers[read->small_reads.length++];
  e->type = HC__DB_STORE_READ_HEAD;
  e->value.buffer = NULL;
  e->value.len = 0;
  e->result = result;
  hc_key_store_head(&e->key);
  return 0;
}

// Result.found is set to 0 on miss, 1 on hit (with core_ptr/data_ptr populated).
static inline int
hc__db_store_read_get_core (hc__db_store_read_t *read, const hc_hash_t discovery_key, hc_store_core_t *result) {
  if (hc__array_grow(&read->small_reads, read->small_reads.length + 1) < 0) return -1;
  hc__db_store_small_read_t *e = &read->small_reads.buffers[read->small_reads.length++];
  e->type = HC__DB_STORE_READ_CORE;
  e->value.buffer = NULL;
  e->value.len = 0;
  e->result = result;
  result->found = 0;
  hc_key_store_core(&e->key, discovery_key);
  return 0;
}

static inline int
hc__db_store_read_flush (hc__db_store_read_t *read) {
  size_t total = read->small_reads.length;
  if (total == 0) return 0;

  rocksdb_read_t *reads = (rocksdb_read_t *) malloc(total * sizeof(rocksdb_read_t));
  if (reads == NULL) return -1;

  for (size_t i = 0; i < total; i++) {
    hc__db_store_small_read_t *e = &read->small_reads.buffers[i];
    reads[i].type = rocksdb_get;
    reads[i].column_family = read->db->cf;
    reads[i].key = rocksdb_slice_init((const char *) e->key.buf.buffer, e->key.buf.len);
    reads[i].value = rocksdb_slice_empty();
  }

  rocksdb_read_batch_t batch;
  int rc = rocksdb_read(&read->db->rocks, &batch, reads, total, NULL, NULL);
  int err = (rc < 0) ? -1 : 0;

  for (size_t i = 0; i < total; i++) {
    hc__db_store_small_read_t *e = &read->small_reads.buffers[i];

    if (err == 0 && batch.errors != NULL && batch.errors[i] != NULL) err = -1;

    if (reads[i].value.data == NULL) continue;

    uint8_t *bytes = (uint8_t *) (uintptr_t) reads[i].value.data;
    size_t len = reads[i].value.len;
    reads[i].value.data = NULL;
    reads[i].value.len = 0;

    if (err < 0) {
      free(bytes);
      continue;
    }

    if (e->type == HC__DB_STORE_READ_HEAD) {
      compact_state_t state = {0, len, bytes};
      hc_store_head_decode(&state, (hc_store_head_t *) e->result);
    } else if (e->type == HC__DB_STORE_READ_CORE) {
      compact_state_t state = {0, len, bytes};
      hc_store_core_t *out = (hc_store_core_t *) e->result;
      if (hc_store_core_decode(&state, &out->core_ptr, &out->data_ptr) == 0) {
        out->found = 1;
      }
    }
    free(bytes);
  }

  rocksdb_read_cleanup(&batch);
  free(reads);
  return err;
}

#ifdef __cplusplus
}
#endif

#endif // HC_DB_H
