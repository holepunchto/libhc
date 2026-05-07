#include <stdlib.h>

#include "hc/core.h"

int
hc_core_init (hc_core_t *core, uint64_t core_ptr, uint64_t data_ptr) {
  hc_storage_core_init(&core->storage, core_ptr, data_ptr);
  core->roots = NULL;
  core->roots_len = 0;
  core->length = 0;
  core->byte_length = 0;
  return 0;
}

void
hc_core_destroy (hc_core_t *core) {
  free(core->roots);
  hc_storage_core_destroy(&core->storage);
}

int
hc_core_checkout (hc_core_t *core, uint64_t length) {
  // TODO: load roots for `length` from storage and recompute byte_length.
  core->length = length;
  return 0;
}
