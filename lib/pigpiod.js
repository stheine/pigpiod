'use strict';

const pigpiod = require('./bindings');
const dht22   = require('./dht22');
const mcp3204 = require('./mcp3204');

module.exports = Object.assign({}, pigpiod, dht22, mcp3204);
