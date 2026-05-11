#include <assert.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include <compact.h>

#include "hc/manifest.h"
#include "hc/schema.h"

// Expected bytes produced by the JS hypercore messages.manifest codec for this
// exact manifest object (verified against the live JS implementation).
static const uint8_t expected[] = {
  0x01, // version=1
  0x00, // flags=0 (no patch, no prologue, no linked, no userData)
  0x00, // hash=blake2b
  0x01, // quorum=1
  0x01, // signers.length=1
  0x00, // signature=ed25519
  // namespace (32 bytes)
  65, 68, 238, 165, 49, 228, 131, 213, 78, 12, 20, 244, 202, 104, 224, 100,
  79, 53, 83, 67, 255, 111, 203, 15, 0, 82, 0, 225, 44, 215, 71, 203,
  // publicKey (32 bytes)
  151, 21, 96, 60, 4, 102, 108, 2, 49, 177, 37, 35, 250, 78, 134, 54,
  41, 139, 12, 152, 134, 19, 251, 37, 63, 214, 175, 138, 18, 82, 17, 224,
};

int
main () {
  hc_signer_t signer = {
    .signature = HC_SIGNATURE_FUNC_ED25519,
    .namespace = {65, 68, 238, 165, 49, 228, 131, 213, 78, 12, 20, 244, 202, 104, 224, 100, 79, 53, 83, 67, 255, 111, 203, 15, 0, 82, 0, 225, 44, 215, 71, 203},
    .public_key = {151, 21, 96, 60, 4, 102, 108, 2, 49, 177, 37, 35, 250, 78, 134, 54, 41, 139, 12, 152, 134, 19, 251, 37, 63, 214, 175, 138, 18, 82, 17, 224},
  };

  hc_manifest_t m = {
    .version = 1,
    .hash = HC_HASH_FUNC_BLAKE2B,
    .allow_patch = false,
    .quorum = 1,
    .signers = {.buffers = &signer, .length = 1, .capacity = 1},
    .prologue = NULL,
    .linked = {.buffers = NULL, .length = 0, .capacity = 0},
    .user_data = {.buffer = NULL, .len = 0},
  };

  // Preencode to determine size.
  compact_state_t state = {0, 0, NULL};
  assert(hc_schema_preencode_manifest(&state, &m) == 0);
  assert(state.end == sizeof(expected));

  // Encode into a buffer.
  uint8_t buf[sizeof(expected)];
  state = (compact_state_t){0, sizeof(buf), buf};
  assert(hc_schema_encode_manifest(&state, &m) == 0);
  assert(state.start == sizeof(expected));
  assert(memcmp(buf, expected, sizeof(expected)) == 0);

  // Decode back and check fields.
  hc_manifest_t got = {0};
  state = (compact_state_t){0, sizeof(expected), (uint8_t *) expected};
  assert(hc_schema_decode_manifest(&state, &got) == 0);

  assert(got.version == 1);
  assert(got.hash == HC_HASH_FUNC_BLAKE2B);
  assert(got.allow_patch == false);
  assert(got.quorum == 1);
  assert(got.signers.length == 1);
  assert(got.signers.buffers[0].signature == HC_SIGNATURE_FUNC_ED25519);
  assert(memcmp(got.signers.buffers[0].namespace, signer.namespace, 32) == 0);
  assert(memcmp(got.signers.buffers[0].public_key, signer.public_key, 32) == 0);
  assert(got.prologue == NULL);
  assert(got.linked.length == 0);
  assert(got.user_data.buffer == NULL);

  free(got.signers.buffers);
  return 0;
}
