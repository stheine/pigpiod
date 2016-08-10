#!/usr/bin/env node
'use strict';

/* eslint-disable no-console */

const pigpiod  = require('../lib/pigpiod.js');

const pi = pigpiod.pigpio_start();

if(pi < 0) {
  throw new Error('Failed to pigpiod.pidpio_start()');
}

const serialHandle = pigpiod.serial_open(pi, '/dev/ttyAMA0', 4800, 0);

pigpiod.serial_write_byte(pi, serialHandle, 0x04);

setInterval(() => {
  console.log('.');

  const dataAvailable = pigpiod.serial_data_available(pi, serialHandle);

  if(dataAvailable) {
    console.log('data!');

    console.log(pigpiod.serial_read_byte(pi, serialHandle));

    pigpiod.serial_close(pi, serialHandle);
    pigpiod.pigpio_stop(pi);
  }
}, 10);
