#include <string.h>

#include "hc/core.h"
#include "hc/db.h"
#include "hc/store.h"

int
hc_store_init (hc_store_t *store, const char *path, uv_loop_t *loop) {
  int err = hc__db_init(&store->db, path, loop);
  if (err < 0) return err;
  memset(&store->head, 0, sizeof(store->head));

  hc__db_store_read_t read;
  hc__db_store_read_init(&read, &store->db);

  err = hc__db_store_read_get_head(&read, &store->head);
  if (err < 0) {
    hc__db_store_read_destroy(&read);
    return err;
  }
  err = hc__db_store_read_flush(&read);
  hc__db_store_read_destroy(&read);
  return err;
}

void
hc_store_destroy (hc_store_t *store) {
  hc__db_destroy(&store->db);
}

int
hc_store_create (hc_store_t *store, struct hc_core_s *core, const hc_hash_t key, const hc_hash_t discovery_key) {
  uint64_t core_ptr = store->head.cores;
  uint64_t data_ptr = store->head.datas;
  store->head.cores++;
  store->head.datas++;

  hc__db_store_write_t write;
  hc__db_store_write_init(&write, &store->db);

  int err = 0;
  err = hc__db_store_write_set_head(&write, &store->head);
  if (err < 0) goto fail;
  err = hc__db_store_write_put_core(&write, discovery_key, core_ptr, data_ptr);
  if (err < 0) goto fail;
  err = hc__db_store_write_flush(&write);
  if (err < 0) goto fail;

  hc__db_store_write_destroy(&write);
  return hc_core_init(core, core_ptr, data_ptr, &store->db, key, discovery_key);

fail:
  hc__db_store_write_destroy(&write);
  store->head.cores--;
  store->head.datas--;
  return err;
}

int
hc_store_get (hc_store_t *store, struct hc_core_s *core, const hc_hash_t key, const hc_hash_t discovery_key) {
  hc_store_core_t entry = {0};

  hc__db_store_read_t read;
  hc__db_store_read_init(&read, &store->db);

  int err = 0;
  err = hc__db_store_read_get_core(&read, discovery_key, &entry);
  if (err < 0) {
    hc__db_store_read_destroy(&read);
    return err;
  }
  err = hc__db_store_read_flush(&read);
  hc__db_store_read_destroy(&read);
  if (err < 0) return err;

  if (!entry.found) return -1;

  err = hc_core_init(core, entry.core_ptr, entry.data_ptr, &store->db, key, discovery_key);
  if (err < 0) return err;
  return hc_core_load(core);
}
