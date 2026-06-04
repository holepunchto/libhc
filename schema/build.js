const CHyperschema = require('hyperschema-c')

const schema = CHyperschema.from(__dirname)
const hc = schema.namespace('hc')

hc.register({
  name: 'tree-node',
  fields: [
    { name: 'index', type: 'uint', required: true },
    { name: 'size', type: 'uint', required: true },
    { name: 'hash', type: 'fixed32', required: true }
  ]
})

hc.register({
  name: 'head',
  fields: [
    { name: 'fork', type: 'uint', required: true },
    { name: 'length', type: 'uint', required: true },
    { name: 'root-hash', type: 'fixed32', required: true },
    { name: 'signature', type: 'buffer', required: true },
    // uint64 (fixed 8 bytes), optional — gated by the struct's flags byte.
    { name: 'timestamp', type: 'uint64', required: false }
  ]
})

hc.register({
  name: 'store-head',
  fields: [
    { name: 'cores', type: 'uint', required: true },
    { name: 'datas', type: 'uint', required: true },
    { name: 'groups', type: 'uint', required: true },
    // Optionals gate on the flags byte: seed = bit 0, default-discovery-key
    // = bit 1, matching the hand-written HC_STORE_HEAD_* flags.
    { name: 'seed', type: 'fixed32', required: false },
    { name: 'default-discovery-key', type: 'fixed32', required: false }
  ]
})

hc.register({
  name: 'store-core',
  fields: [
    { name: 'core-ptr', type: 'uint', required: true },
    { name: 'data-ptr', type: 'uint', required: true }
  ]
})

// compact: encoded inline (no length frame), matching the canonical hypercore
// manifest encoding that the core key hashes over. hash/signature funcs encode
// as a uint (one value each today); callers assign the HC_*_FUNC_* constants
// kept in manifest.h.
hc.register({
  name: 'signer',
  compact: true,
  fields: [
    { name: 'signature', type: 'uint', required: true },
    { name: 'namespace', type: 'fixed32', required: true },
    { name: 'public-key', type: 'fixed32', required: true }
  ]
})

hc.register({
  name: 'prologue',
  compact: true,
  fields: [
    { name: 'hash', type: 'fixed32', required: true },
    { name: 'length', type: 'uint', required: true }
  ]
})

hc.register({
  name: 'manifest',
  fields: [
    { name: 'version', type: 'uint', required: true },
    { name: 'allow-patch', type: 'bool', required: true },
    { name: 'hash', type: 'uint', required: true },
    { name: 'quorum', type: 'uint', required: true },
    { name: 'signers', type: '@hc/signer', array: true, required: true },
    { name: 'prologue', type: '@hc/prologue', required: false },
    { name: 'linked', type: 'fixed32', array: true, required: false },
    { name: 'user-data', type: 'buffer', required: false }
  ]
})

// Writes to __dirname (the directory passed to CHyperschema.from above).
CHyperschema.toDisk(schema)
