#include <stdlib.h>
#include <string.h>

#include <compact.h>
#include <kv.h>

#include "hc/core.h"
#include "hc/schema.h"
#include "hc/store.h"
#include "encodings/keys.h"

int
hc_store_init (hc_store_t *store) {
  kv_init(&store->kv);
  memset(&store->head, 0, sizeof(store->head));

  compact_state_t ks = {0, 0, NULL};
  hc_key_preencode_store_head(&ks);
  uint8_t kd[HC_KEY_MAX_SIZE];
  compact_state_t ke = {0, ks.end, kd};
  hc_key_encode_store_head(&ke);

  kv_read_batch_t read;
  kv_read_batch_init(&read, &store->kv, 1);
  uint8_t *val = NULL;
  size_t val_len = 0;
  if (kv_read_batch_get(&read, kd, ke.start, &val, &val_len) < 0) {
    kv_read_batch_destroy(&read);
    return -1;
  }
  if (kv_read_batch_flush(&read) < 0) {
    kv_read_batch_destroy(&read);
    return -1;
  }
  kv_read_batch_destroy(&read);

  if (val == NULL) return 0; // fresh store

  compact_state_t vs = {0, val_len, val};
  int err = hc_store_head_decode(&vs, &store->head);
  free(val);
  return err;
}

void
hc_store_destroy (hc_store_t *store) {
  kv_destroy(&store->kv);
}

static int
flush_store_head (hc_store_t *store) {
  compact_state_t ks = {0, 0, NULL};
  hc_key_preencode_store_head(&ks);
  uint8_t kd[HC_KEY_MAX_SIZE];
  compact_state_t ke = {0, ks.end, kd};
  hc_key_encode_store_head(&ke);

  compact_state_t vs = {0, 0, NULL};
  hc_store_head_preencode(&vs, &store->head);
  uint8_t *vd = malloc(vs.end);
  if (vd == NULL) return -1;
  compact_state_t ve = {0, vs.end, vd};
  hc_store_head_encode(&ve, &store->head);

  kv_write_batch_t write;
  kv_write_batch_init(&write, &store->kv, 1);
  int err = kv_write_batch_put(&write, kd, ke.start, vd, ve.start);
  if (err == 0) err = kv_write_batch_flush(&write);
  kv_write_batch_destroy(&write);
  free(vd);
  return err;
}

int
hc_store_create (hc_store_t *store, struct hc_core_s *core, const hc_hash_t key, const hc_hash_t discovery_key) {
  uint64_t core_ptr = store->head.cores;
  uint64_t data_ptr = store->head.datas;

  compact_state_t cks = {0, 0, NULL};
  hc_key_preencode_store_core(&cks, discovery_key);
  uint8_t ckd[HC_KEY_MAX_SIZE];
  compact_state_t cke = {0, cks.end, ckd};
  hc_key_encode_store_core(&cke, discovery_key);

  compact_state_t cvs = {0, 0, NULL};
  hc_store_core_preencode(&cvs, core_ptr, data_ptr);
  uint8_t *cvd = malloc(cvs.end);
  if (cvd == NULL) return -1;
  compact_state_t cve = {0, cvs.end, cvd};
  hc_store_core_encode(&cve, core_ptr, data_ptr);

  store->head.cores++;
  store->head.datas++;

  compact_state_t hks = {0, 0, NULL};
  hc_key_preencode_store_head(&hks);
  uint8_t hkd[HC_KEY_MAX_SIZE];
  compact_state_t hke = {0, hks.end, hkd};
  hc_key_encode_store_head(&hke);

  compact_state_t hvs = {0, 0, NULL};
  hc_store_head_preencode(&hvs, &store->head);
  uint8_t *hvd = malloc(hvs.end);
  if (hvd == NULL) {
    store->head.cores--;
    store->head.datas--;
    free(cvd);
    return -1;
  }
  compact_state_t hve = {0, hvs.end, hvd};
  hc_store_head_encode(&hve, &store->head);

  kv_write_batch_t write;
  kv_write_batch_init(&write, &store->kv, 2);
  int err = kv_write_batch_put(&write, hkd, hke.start, hvd, hve.start);
  if (err == 0) err = kv_write_batch_put(&write, ckd, cke.start, cvd, cve.start);
  if (err == 0) err = kv_write_batch_flush(&write);
  kv_write_batch_destroy(&write);
  free(hvd);
  free(cvd);

  if (err < 0) {
    store->head.cores--;
    store->head.datas--;
    return err;
  }

  return hc_core_init(core, core_ptr, data_ptr, key, discovery_key);
}

int
hc_store_get (hc_store_t *store, struct hc_core_s *core, const hc_hash_t key, const hc_hash_t discovery_key) {
  compact_state_t cks = {0, 0, NULL};
  hc_key_preencode_store_core(&cks, discovery_key);
  uint8_t ckd[HC_KEY_MAX_SIZE];
  compact_state_t cke = {0, cks.end, ckd};
  hc_key_encode_store_core(&cke, discovery_key);

  kv_read_batch_t read;
  kv_read_batch_init(&read, &store->kv, 1);
  uint8_t *val = NULL;
  size_t val_len = 0;
  if (kv_read_batch_get(&read, ckd, cke.start, &val, &val_len) < 0) {
    kv_read_batch_destroy(&read);
    return -1;
  }
  if (kv_read_batch_flush(&read) < 0) {
    kv_read_batch_destroy(&read);
    return -1;
  }
  kv_read_batch_destroy(&read);

  if (val == NULL) return -1; // not found

  uint64_t core_ptr, data_ptr;
  compact_state_t vs = {0, val_len, val};
  int err = hc_store_core_decode(&vs, &core_ptr, &data_ptr);
  free(val);
  if (err < 0) return err;

  return hc_core_init(core, core_ptr, data_ptr, key, discovery_key);
}
