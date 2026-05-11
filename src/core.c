#include "hc/core.h"

int
hc_core_init (hc_core_t *core, uint64_t core_ptr, uint64_t data_ptr) {
  hc__db_core_init(&core->db, core_ptr, data_ptr);
  hc__array_init(&core->roots);
  core->length = 0;
  core->byte_length = 0;
  return 0;
}

void
hc_core_destroy (hc_core_t *core) {
  hc__array_destroy(&core->roots);
  hc__db_core_destroy(&core->db);
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
