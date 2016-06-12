'use strict';

const pigpiod = require('../pigpiod.js');

const GPIO_WIND            = 25; // Pin 22 / GPIO25 - Windmelder

const GPIO_TASTER_RUNTER   = 22; // GPIO22, Pin15 - Input  - Taster runter
const GPIO_TASTER_HOCH     = 27; // GPIO27, Pin13 - Input  - Taster hoch
const GPIO_JALOUSIE_RUNTER =  4; // GPIO4,  Pin7  - Output - Jalousie runter
const GPIO_JALOUSIE_HOCH   = 17; // GPIO17, Pin11 - Output - Jalousie hoch

const MCP3204_SPI_CHANNEL  = 0;  // SPI Channel 0



let pi;

if((pi = pigpiod.pigpio_start()) < 0) {
  throw new Error('Failed to pigpiod.pidpio_start()');
}

console.log(`GPIO_WIND = ${pigpiod.get_mode(pi, GPIO_WIND)}`);
console.log(`GPIO_TASTER_RUNTER = ${pigpiod.get_mode(pi, GPIO_TASTER_RUNTER)}`);
console.log(`GPIO_TASTER_HOCH = ${pigpiod.get_mode(pi, GPIO_TASTER_HOCH)}`);
console.log(`GPIO_JALOUSIE_RUNTER = ${pigpiod.get_mode(pi, GPIO_JALOUSIE_RUNTER)}`);
console.log(`GPIO_JALOUSIE_HOCH = ${pigpiod.get_mode(pi, GPIO_JALOUSIE_HOCH)}`);

pigpiod.pigpio_stop(pi);
