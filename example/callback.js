'use strict';

const pigpiod  = require('bindings')('pigpiod.node');

const GPIO_WIND            = 25   // Pin 22 / GPIO25 - Windmelder

const RISING_EDGE  = 0;
const FALLING_EDGE = 1;
const EITHER_EDGE  = 2;



let pi;
let callbackId;
let windCounter = 0;

if((pi = pigpiod.pigpio_start()) < 0) {
  throw new Error('Failed to pigpiod.pidpio_start()');
}

const gpioWindCallback = function(gpio, level, tick) {
  console.log(`Interrupt GPIO_WIND ${gpio}/${level}/${tick}`);
  windCounter++;
};

if((callbackId =
  pigpiod.callback(pi, GPIO_WIND, FALLING_EDGE, gpioWindCallback)) < 0
) {
  throw new Error('Failed to pigpiod.callback()');
}


// Trigger automatic interrupts, even if no wind.
pigpiod.set_watchdog(pi, GPIO_WIND, 500); // 1000

const intervalObject = setInterval(() => {
  console.log(`windCounter = ${windCounter}`);
  windCounter = 0;
}, 1000);

setTimeout(() => {
  clearInterval(intervalObject);

  pigpiod.set_watchdog(pi, GPIO_WIND, 0);
  pigpiod.callback_cancel(callbackId);
  pigpiod.pigpio_stop(pi);

  // TODO why is not going down automatically?
//  process.exit();
}, 5000);
