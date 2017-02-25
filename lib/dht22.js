'use strict';

// Reads the data of a DHT22 sensor, synchronous

/* eslint-disable no-bitwise */

const pigpiod = require('../lib/bindings.js');

const DHT_GOOD         = 0;
const DHT_BAD_CHECKSUM = 1;
const DHT_BAD_DATA     = 2;
const DHT_TIMEOUT      = 3;

const dht22 = function(pi, gpio) {
  return new Promise(resolve => {
    pigpiod.dht22_get(pi, gpio, (temperature, humidity, status) => {
      resolve({
        temperature: Math.round(temperature * 10) / 10,
        humidity:    Math.round(humidity * 10) / 10,
        status});
    });
  });
};

module.exports = {
  DHT_GOOD,
  DHT_BAD_CHECKSUM,
  DHT_BAD_DATA,
  DHT_TIMEOUT,
  dht22
};
