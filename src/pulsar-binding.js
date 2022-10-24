const path = require('path');
const binary = require('@mapbox/node-pre-gyp');

const bindingPath = binary.find(path.resolve(path.join(__dirname, '../package.json')));
// eslint-disable-next-line import/no-dynamic-require
const binding = require(bindingPath);
// eslint-disable-next-line no-multi-assign
module.exports = exports = binding;
