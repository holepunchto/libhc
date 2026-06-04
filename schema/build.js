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

// Writes to __dirname (the directory passed to CHyperschema.from above).
CHyperschema.toDisk(schema)
