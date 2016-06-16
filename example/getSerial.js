'use strict';

const pigpiod  = require('../lib/pigpiod.js');

let   pi;

if((pi = pigpiod.pigpio_start()) < 0) {
  throw new Error('Failed to pigpiod.pidpio_start()');
}

// const txBuf = Buffer.allocUnsafe(3);
// const rxBuf = Buffer.allocUnsafe(3);
// let spiResult;
// let a2dVal;

const serialHandle = pigpiod.serial_open(pi, "/dev/ttyAMA0", 4800, 0);

pigpiod.serial_write_byte(pi, serialHandle, 0x04); // Terminate previous connection

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

/*
setInterval(() => {
  console.log('.');
  pigpiod.serial_write_byte(pi, serialHandle, 0x16);
  pigpiod.serial_write_byte(pi, serialHandle, 0x0);
  pigpiod.serial_write_byte(pi, serialHandle, 0x0);

  const dataAvailable = pigpiod.serial_data_available(pi, serialHandle);
  if(dataAvailable) {
    console.log('data!');

    console.log(pigpiod.serial_read_byte(pi, serialHandle));

    pigpiod.serial_close(pi, serialHandle);
    pigpiod.pigpio_stop(pi);
  }
}, 10);
*/


/*
setTimeout(() => {
  const dataAvailable = pigpiod.serial_data_available(pi, serialHandle);

  for(let i = 0; i < dataAvailable; i++) {
    const readByte = pigpiod.serial_read_byte(pi, serialHandle);

    console.log(readByte);
  }

  pigpiod.serial_close(pi, serialHandle);
  pigpiod.pigpio_stop(pi);
}, 100);
*/
