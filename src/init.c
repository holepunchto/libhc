#include <sodium.h>

#include "hc.h"
#include "hc/hashes.h"

static int initialized = 0;

int
hc_init (void) {
  if (initialized) return 1;

  if (sodium_init() < 0) return -1;

  // sodium_init must run first — namespace hashes use BLAKE2b through
  // libsodium.
  hc__hashes_init();

  initialized = 1;
  return 0;
}
