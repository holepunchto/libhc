#include <sodium.h>

#include "hc.h"

int
hc_init (void) {
  // sodium_init returns 0 on success, 1 if already initialised, < 0 on
  // failure. Normalise to 0 / -1.
  return sodium_init() < 0 ? -1 : 0;
}
