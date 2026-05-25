#ifndef HC_H
#define HC_H

#include "hc/array.h"
#include "hc/buffer.h"
#include "hc/core.h"
#include "hc/crypto.h"
#include "hc/head.h"
#include "hc/keys.h"
#include "hc/merkle_tree.h"
#include "hc/manifest.h"
#include "hc/schema.h"
#include "hc/db.h"

#ifdef __cplusplus
extern "C" {
#endif

// Initialise process-global state used by libhc. Currently this just
// runs sodium_init; idempotent and cheap to call multiple times. Should
// be called once before any other libhc function. Returns 0 on success,
// < 0 on failure.
int
hc_init (void);

#ifdef __cplusplus
}
#endif

#endif // HC_H
