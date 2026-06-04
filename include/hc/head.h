#ifndef HC_HEAD_H
#define HC_HEAD_H

#include <hc_schema.h>

#ifdef __cplusplus
extern "C" {
#endif

// Upper bound on a compact-encoded head: uint(fork) + uint(length) +
// fixed32(rootHash) + uint8array(signature, max 64) + uint(flags) +
// uint64(timestamp) = 9 + 9 + 32 + (1 + 64) + 1 + 8 = 124.
#define HC_HEAD_MAX_SIZE 128

#ifdef __cplusplus
}
#endif

#endif // HC_HEAD_H
