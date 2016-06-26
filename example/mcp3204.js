'use strict';

// Reads the data of an MCP3204 A-D converter on SPI channel 0, MCP channels 0-3.

const pigpiod = require('../lib/pigpiod.js');
let   pi;
let   a2dValue;

if((pi = pigpiod.pigpio_start()) < 0) {
  throw new Error('Failed to pigpiod.pidpio_start()');
}

a2dValue = pigpiod.mcp3204(pi, 0, 0);
console.log(a2dValue);

a2dValue = pigpiod.mcp3204(pi, 0, 1);
console.log(a2dValue);

a2dValue = pigpiod.mcp3204(pi, 0, 2);
console.log(a2dValue);

a2dValue = pigpiod.mcp3204(pi, 0, 3);
console.log(a2dValue);

pigpiod.pigpio_stop(pi);
