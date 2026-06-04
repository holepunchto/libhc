#ifndef HC_ENCODINGS_H
#define HC_ENCODINGS_H

#include <compact.h>

#include "manifest.h"

#ifdef __cplusplus
extern "C" {
#endif

int
hc_manifest_preencode (compact_state_t *state, const hc_manifest_t *manifest);
int
hc_manifest_encode (compact_state_t *state, const hc_manifest_t *manifest);
int
hc_manifest_decode (compact_state_t *state, hc_manifest_t *manifest);

int
hc_store_core_preencode (compact_state_t *state, uint64_t core_ptr, uint64_t data_ptr);
int
hc_store_core_encode (compact_state_t *state, uint64_t core_ptr, uint64_t data_ptr);
int
hc_store_core_decode (compact_state_t *state, uint64_t *core_ptr, uint64_t *data_ptr);

// Upper bounds on compact-encoded store records.
#define HC_STORE_HEAD_MAX_SIZE 128
#define HC_STORE_CORE_MAX_SIZE 32

#ifdef __cplusplus
}
#endif

#endif // HC_ENCODINGS_H
