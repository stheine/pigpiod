#!/usr/bin/env node
'use strict';

// Reads the data of a DHT22 sensor
// The DHT sensor is connected to GPIO 18.

const moment  = require('moment');
const pigpiod = require('../lib/pigpiod.js');
let   pi;

if((pi = pigpiod.pigpio_start()) < 0) {
  throw new Error('Failed to pigpiod.pidpio_start()');
}

console.log('start: ', moment().format());

setInterval(() => {
  console.log('startdht: ', moment().format('HH:mm:ss.SSS'));
  const dht22Data = pigpiod.dht22Sync(pi, 18);
  console.log('stopdht:  ', moment().format('HH:mm:ss.SSS'));
  console.log(dht22Data);
}, 3000); // do not run more often
