'use strict';

const pigpiod = require('./bindings');
const dht     = require('./dht');

module.exports = Object.assign({}, pigpiod, dht);
