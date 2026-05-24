# libhc

Native workhorse for Hypercore.

> AI assisted

## API

See `include/hc.h` and the sub-headers it includes.

## Hello world

```c
#include <string.h>

#include "hc/core.h"
#include "hc/crypto.h"
#include "hc/store.h"

int
main () {
  hc_store_t store;
  hc_store_init(&store);

  hc_crypto_keypair_t kp;
  hc_crypto_keypair(&kp, NULL);

  hc_hash_t discovery_key;
  hc_crypto_discovery_key(discovery_key, kp.public_key);

  hc_core_t core;
  hc_store_create(&store, &core, kp.public_key, discovery_key);

  const char *msg = "hello world";
  hc_buf_t block = { .len = strlen(msg), .buffer = (uint8_t *) msg };
  hc_core_append(&core, &block, 1);

  hc_core_destroy(&core);
  hc_store_destroy(&store);
  return 0;
}
```

## License

Apache-2.0
