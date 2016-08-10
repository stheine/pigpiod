#!/usr/bin/env node
'use strict';

/* eslint-disable no-console */
/* eslint-disable no-process-exit */

const pigpiod = require('../lib/pigpiod.js');

const GPIO_WIND = 25; // Pin 22 / GPIO25 - Windsensor



let windCounter = 0;

const pi = pigpiod.pigpio_start();

if(pi < 0) {
  throw new Error('Failed to pigpiod.pidpio_start()');
}

const gpioWindCallback = function(gpio, level, tick) {
  console.log(`Interrupt GPIO_WIND ${gpio}/${level}/${tick}`);
  windCounter++;
};

const callbackId =
  pigpiod.callback(pi, GPIO_WIND, pigpiod.FALLING_EDGE, gpioWindCallback);

if(callbackId < 0) {
  throw new Error('Failed to pigpiod.callback()');
}


// Trigger automatic interrupts, even if no wind.
pigpiod.set_watchdog(pi, GPIO_WIND, 500);

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
  process.exit();
}, 5000);
