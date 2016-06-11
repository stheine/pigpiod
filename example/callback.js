'use strict';

const pigpiod  = require('bindings')('pigpiod.node');

const GPIO_WIND            = 25   // Pin 22 / GPIO25 - Windmelder

const RISING_EDGE  = 0;
const FALLING_EDGE = 1;
const EITHER_EDGE  = 2;


let pi;

if((pi = pigpiod.pigpio_start()) < 0) {
  throw new Error('Failed to pigpiod.pidpio_start()');
}

let windCounter = 0;
if(pigpiod.callback(pi, GPIO_WIND, FALLING_EDGE, (gpio, level, tick) => {
  console.log(`Interrupt GPIO_WIND ${gpio}/${level}/${tick}`);
  windCounter++;
}) < 0) {
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

  pigpiod.pigpio_stop(pi);

  process.exit();
  // TODO why is not going down automatically?
  // Probably somehow related to the registered pigpiod.callback()?!!?
}, 10000);
