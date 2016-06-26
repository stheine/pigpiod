'use strict';

const pigpiod = require('./bindings');
const dht     = require('./dht');
const mcp3204 = require('./mcp3204');

module.exports = Object.assign({}, pigpiod, dht, mcp3204);
