#ifndef HC_STORE_H
#define HC_STORE_H

#include <stdint.h>

#include "crypto.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
  uint64_t cores;
  uint64_t datas;
  uint64_t groups;
  uint8_t has_seed;
  hc_hash_t seed;
  uint8_t has_default_discovery_key;
  hc_hash_t default_discovery_key;
} hc_store_head_t;

#ifdef __cplusplus
}
#endif

#endif // HC_STORE_H
