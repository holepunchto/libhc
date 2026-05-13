#include <stdlib.h>
#include <string.h>

#include "hc/core.h"
#include "hc/db.h"
#include "hc/merkle_tree.h"

int
hc_core_init (hc_core_t *core, uint64_t core_ptr, uint64_t data_ptr, const hc_hash_t key, const hc_hash_t discovery_key) {
  hc__db_core_init(&core->db, core_ptr, data_ptr);
  memcpy(core->key, key, sizeof(hc_hash_t));
  memcpy(core->discovery_key, discovery_key, sizeof(hc_hash_t));
  core->manifest = NULL;
  hc__array_init(&core->roots);
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
  int rc;

  if ((rc = hc_merkle_tree_append(upgrade, write, buffers, count)) < 0) return rc;
  if ((rc = hc__db_core_write_ensure_blocks(write, count)) < 0) return rc;

  for (size_t i = 0; i < count; i++) {
    if ((rc = hc__db_core_write_block(write, upgrade->core->length + i, buffers[i])) < 0) return rc;
  }

  return hc__db_core_write_flush(write);
}

int
hc_core_append (hc_core_t *core, const hc_buf_t *buffers, size_t count) {
  hc_core_upgrade_t upgrade;
  hc__db_core_write_t write;

  hc_core_upgrade_init(&upgrade, core);
  hc__db_core_write_init(&write, &core->db);

  int rc = hc_core_append_work(&upgrade, &write, buffers, count);

  if (rc == 0) hc_core_commit(&upgrade);

  hc__db_core_write_destroy(&write);
  hc_core_upgrade_destroy(&upgrade);
  return rc;
}

int
hc_core_checkout (hc_core_t *core, uint64_t length) {
  // TODO: load roots for `length` from storage and recompute byte_length.
  core->length = length;
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
