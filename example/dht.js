'use strict';

// Reads the data of a DHT22 sensor
// The DHT sensor is connected to GPIO 18.

const pigpiod = require('../lib/pigpiod.js');
let   pi;

if((pi = pigpiod.pigpio_start()) < 0) {
  throw new Error('Failed to pigpiod.pidpio_start()');
}

pigpiod.dht(pi, 18).then(result => {
  if(result.status) {
    console.log(result.status);
  } else {
    console.log(result);
  }

  pigpiod.pigpio_stop(pi);

  process.exit(result.status);
});
