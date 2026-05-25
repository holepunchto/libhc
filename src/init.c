#include <sodium.h>

#include "hc.h"
#include "hc/hashes.h"

int
hc_init (void) {
  // sodium_init returns 0 on success, 1 if already initialised, < 0 on
  // failure. Normalise to 0 / -1.
  if (sodium_init() < 0) return -1;

  // sodium_init must run first — namespace hashes use BLAKE2b through
  // libsodium.
  hc__hashes_init();

  return 0;
}
