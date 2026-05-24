#ifndef HC_HEAD_H
#define HC_HEAD_H

#include <stdlib.h>
#include <stdint.h>

#include "buffer.h"
#include "crypto.h"

#ifdef __cplusplus
extern "C" {
#endif

// Upper bound on a compact-encoded head: uint(fork) + uint(length) +
// fixed32(rootHash) + uint8array(signature, max 64) + uint(flags) +
// uint64(timestamp) = 9 + 9 + 32 + (1 + 64) + 1 + 8 = 124.
#define HC_HEAD_MAX_SIZE 128

typedef struct {
  uint64_t fork;
  uint64_t length;
  hc_hash_t root_hash;
  hc_buf_t signature; // caller-owned; free signature.buffer after use
  uint64_t timestamp; // 0 if not present
} hc_head_t;

static inline void
hc_head_destroy (hc_head_t *head) {
  free(head->signature.buffer);
  head->signature.buffer = NULL;
  head->signature.len = 0;
}

#ifdef __cplusplus
}
#endif

#endif // HC_HEAD_H
