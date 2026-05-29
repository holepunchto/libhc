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

// Writes to __dirname (the directory passed to CHyperschema.from above).
CHyperschema.toDisk(schema)
