'use strict';

const pigpiod   = require('./bindings');
const dht       = require('./dht');
// const dht22   =   require('./dht22');
const dht22Sync = require('./dht22Sync');
const mcp3204   = require('./mcp3204');

module.exports = Object.assign({}, pigpiod, dht, dht22Sync, mcp3204);
