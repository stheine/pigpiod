#!/usr/bin/env node
'use strict';

/* eslint-disable no-console */
/* eslint-disable no-process-exit */

// Reads the data of a DHT22 sensor
// The DHT sensor is connected to GPIO 18.

const pigpiod = require('../lib/pigpiod.js');

const pi = pigpiod.pigpio_start();

if(pi < 0) {
  throw new Error('Failed to pigpiod.pidpio_start()');
}

const interval = setInterval(() => {
  pigpiod.dht22(pi, 18).then(dht22Data => {
    if(dht22Data.status) {
      console.log('failed: status=', dht22Data.status);
    } else {
      console.log(dht22Data);
    }
  });
}, 3000);

setTimeout(() => {
  clearInterval(interval);
  pigpiod.pigpio_stop(pi);
  process.exit();
}, 3000000);
