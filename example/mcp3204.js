#!/usr/bin/env node
'use strict';

/* eslint-disable no-console */

// Reads data of an MCP3204 A-D converter on SPI channel 0, MCP channels 0-3.

const pigpiod = require('../lib/pigpiod.js');

let   a2dValue;

const pi = pigpiod.pigpio_start();

if(pi < 0) {
  throw new Error('Failed to pigpiod.pidpio_start()');
}

const spi = pigpiod.spi_open(pi, 0, 500000, 0);

if(spi < 0) {
  throw new Error('Failed to pigpiod.spi_open()');
}

a2dValue = pigpiod.mcp3204(pi, spi, 0);
console.log('#0', a2dValue);

a2dValue = pigpiod.mcp3204(pi, spi, 1);
console.log('#1', a2dValue);

a2dValue = pigpiod.mcp3204(pi, spi, 2);
console.log('#2', a2dValue);

a2dValue = pigpiod.mcp3204(pi, spi, 3);
console.log('#3', a2dValue);

pigpiod.spi_close(pi, spi);
pigpiod.pigpio_stop(pi);
