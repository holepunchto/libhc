# libhc

Native workhorse for Hypercore.

> AI assisted

## API

See `include/hc.h` and the sub-headers it includes.

## Hello world

```c
#include <string.h>

#include <uv.h>

#include "hc.h"
#include "hc/core.h"
#include "hc/crypto.h"
#include "hc/hashes.h"
#include "hc/manifest.h"
#include "hc/store.h"

int
main () {
  hc_init();

  hc_store_t store;
  hc_store_init(&store, "/tmp/libhc-hello", uv_default_loop());

  hc_crypto_keypair_t kp;
  hc_crypto_keypair(&kp, NULL);

  hc_manifest_t manifest;
  hc_manifest_init_single_signer(&manifest, &kp);

  hc_hash_t key, discovery_key;
  hc_hashes_manifest(key, &manifest);
  hc_crypto_discovery_key(discovery_key, key);

  hc_core_t core;
  hc_store_create(&store, &core, key, discovery_key);

  const char *msg = "hello world";
  hc_buf_t block = { .len = strlen(msg), .buffer = (uint8_t *) msg };
  hc_core_append(&core, &block, 1);

  hc_core_destroy(&core);
  hc_manifest_destroy(&manifest);
  hc_store_destroy(&store);
  return 0;
}
```

## License

Apache-2.0
