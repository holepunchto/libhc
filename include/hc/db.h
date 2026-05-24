#ifndef HC_DB_H
#define HC_DB_H

#include <stddef.h>
#include <stdint.h>

#include <kv.h>

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

typedef struct hc__db_store_s hc__db_store_t;

struct hc__db_store_s {
  kv_t kv;
};

static inline int
hc__db_store_init (hc__db_store_t *db) {
  kv_init(&db->kv);
  return 0;
}

static inline void
hc__db_store_destroy (hc__db_store_t *db) {
  kv_destroy(&db->kv);
}

typedef struct hc__db_core_s hc__db_core_t;

struct hc__db_core_s {
  kv_t *kv;
  uint64_t core_ptr;
  uint64_t data_ptr;
};

static inline int
hc__db_core_init (hc__db_core_t *db, uint64_t core_ptr, uint64_t data_ptr, kv_t *kv) {
  db->kv = kv;
  db->core_ptr = core_ptr;
  db->data_ptr = data_ptr;
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
  kv_write_batch_t kv_batch;
  kv_write_batch_init(&kv_batch, write->db->kv, write->tree_nodes.length + write->blocks.length + write->deletes.length + (write->has_head ? 1 : 0));

  for (size_t i = 0; i < write->tree_nodes.length; i++) {
    hc__db_tree_node_kv_t *kv = &write->tree_nodes.buffers[i];
    if (kv_write_batch_put(&kv_batch, kv->key.buf.buffer, kv->key.buf.len, kv->value.buffer, kv->value.len) < 0) {
      kv_write_batch_destroy(&kv_batch);
      return -1;
    }
  }

  for (size_t i = 0; i < write->blocks.length; i++) {
    hc__db_block_kv_t *kv = &write->blocks.buffers[i];
    if (kv_write_batch_put(&kv_batch, kv->key.buf.buffer, kv->key.buf.len, kv->value.buffer, kv->value.len) < 0) {
      kv_write_batch_destroy(&kv_batch);
      return -1;
    }
  }

  for (size_t i = 0; i < write->deletes.length; i++) {
    hc_small_key_t *key = &write->deletes.buffers[i];
    if (kv_write_batch_del(&kv_batch, key->buf.buffer, key->buf.len) < 0) {
      kv_write_batch_destroy(&kv_batch);
      return -1;
    }
  }

  if (write->has_head) {
    hc__db_core_head_kv_t *kv = &write->head;
    if (kv_write_batch_put(&kv_batch, kv->key.buf.buffer, kv->key.buf.len, kv->value.buffer, kv->value.len) < 0) {
      kv_write_batch_destroy(&kv_batch);
      return -1;
    }
  }

  int err = kv_write_batch_flush(&kv_batch);
  kv_write_batch_destroy(&kv_batch);
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
  kv_read_batch_t kv_batch;
  kv_read_batch_init(&kv_batch, read->db->kv, read->small_reads.length);

  for (size_t i = 0; i < read->small_reads.length; i++) {
    hc__db_small_read_t *e = &read->small_reads.buffers[i];
    if (kv_read_batch_get(&kv_batch, e->key.buf.buffer, e->key.buf.len, &e->value.buffer, &e->value.len) < 0) {
      kv_read_batch_destroy(&kv_batch);
      return -1;
    }
  }

  int err = kv_read_batch_flush(&kv_batch);
  kv_read_batch_destroy(&kv_batch);
  if (err < 0) return err;

  for (size_t i = 0; i < read->small_reads.length; i++) {
    hc__db_small_read_t *e = &read->small_reads.buffers[i];
    if (e->value.buffer == NULL) continue;
    if (e->type == HC__DB_READ_TREE_NODE) {
      compact_state_t state = {0, e->value.len, e->value.buffer};
      hc_tree_node_decode(&state, (hc_merkle_tree_node_t *) e->result);
      free(e->value.buffer);
    } else if (e->type == HC__DB_READ_BLOCK) {
      *(hc_buf_t *) e->result = e->value;
    }
  }

  return 0;
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
  hc__db_store_t *db;
  hc__db_store_head_kv_t head;
  uint8_t has_head;
  HC__ARRAY(hc__db_store_core_kv_t) cores;
};

static inline void
hc__db_store_write_init (hc__db_store_write_t *write, hc__db_store_t *db) {
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
  kv_write_batch_t kv_batch;
  kv_write_batch_init(&kv_batch, &write->db->kv, write->cores.length + (write->has_head ? 1 : 0));

  if (write->has_head) {
    hc__db_store_head_kv_t *kv = &write->head;
    if (kv_write_batch_put(&kv_batch, kv->key.buf.buffer, kv->key.buf.len, kv->value.buffer, kv->value.len) < 0) {
      kv_write_batch_destroy(&kv_batch);
      return -1;
    }
  }

  for (size_t i = 0; i < write->cores.length; i++) {
    hc__db_store_core_kv_t *kv = &write->cores.buffers[i];
    if (kv_write_batch_put(&kv_batch, kv->key.buf.buffer, kv->key.buf.len, kv->value.buffer, kv->value.len) < 0) {
      kv_write_batch_destroy(&kv_batch);
      return -1;
    }
  }

  int err = kv_write_batch_flush(&kv_batch);
  kv_write_batch_destroy(&kv_batch);
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
  hc__db_store_t *db;
  HC__ARRAY(hc__db_store_small_read_t) small_reads;
} hc__db_store_read_t;

static inline void
hc__db_store_read_init (hc__db_store_read_t *read, hc__db_store_t *db) {
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
  kv_read_batch_t kv_batch;
  kv_read_batch_init(&kv_batch, &read->db->kv, read->small_reads.length);

  for (size_t i = 0; i < read->small_reads.length; i++) {
    hc__db_store_small_read_t *e = &read->small_reads.buffers[i];
    if (kv_read_batch_get(&kv_batch, e->key.buf.buffer, e->key.buf.len, &e->value.buffer, &e->value.len) < 0) {
      kv_read_batch_destroy(&kv_batch);
      return -1;
    }
  }

  int err = kv_read_batch_flush(&kv_batch);
  kv_read_batch_destroy(&kv_batch);
  if (err < 0) return err;

  for (size_t i = 0; i < read->small_reads.length; i++) {
    hc__db_store_small_read_t *e = &read->small_reads.buffers[i];
    if (e->value.buffer == NULL) continue;
    if (e->type == HC__DB_STORE_READ_HEAD) {
      compact_state_t state = {0, e->value.len, e->value.buffer};
      hc_store_head_decode(&state, (hc_store_head_t *) e->result);
    } else if (e->type == HC__DB_STORE_READ_CORE) {
      compact_state_t state = {0, e->value.len, e->value.buffer};
      hc_store_core_t *out = (hc_store_core_t *) e->result;
      if (hc_store_core_decode(&state, &out->core_ptr, &out->data_ptr) == 0) {
        out->found = 1;
      }
    }
    free(e->value.buffer);
  }

  return 0;
}

#ifdef __cplusplus
}
#endif

#endif // HC_DB_H
