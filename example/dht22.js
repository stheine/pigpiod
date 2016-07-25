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
  pigpiod.dht22(pi, 18).then(result => {
    if(result.status) {
      console.log('failed: status=', result.status);
    } else {
      console.log('finished: ', moment().format());
      console.log(result);
    }
  })
  .catch(err => {
    console.log(err);
    pigpiod.pigpio_stop(pi);
    process.exit(1);
  });
}, 10000); // do not run more often
