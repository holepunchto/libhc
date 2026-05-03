#ifndef HC_MERKLE_TREE_H
#define HC_MERKLE_TREE_H

#include <stdint.h>

#include "crypto.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
  uint64_t index;
  uint64_t size;
  uint8_t hash[HC_CRYPTO_HASH_SIZE];
} hc_merkle_tree_node_t;

#ifdef __cplusplus
}
#endif

#endif // HC_MERKLE_TREE_H
