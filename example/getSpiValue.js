'use strict';

const pigpiod  = require('bindings')('pigpiod.node');

const mcp3204SpiChannel  = 0  // SPI Channel 0
let   pi;

if((pi = pigpiod.pigpio_start()) < 0) {
  throw new Error('Failed to pigpiod.pidpio_start()');
}

const txBuf = Buffer.allocUnsafe(3);
const rxBuf = Buffer.allocUnsafe(3);
let spiResult;
let a2dVal;

const mcpSpiHandle = pigpiod.spi_open(pi, mcp3204SpiChannel, 500000, 0);

// Read value from Channel 0
txBuf.writeInt8(parseInt('00000110', 2), 0);
txBuf.writeInt8(parseInt('00000000', 2), 1); // channel 0
txBuf.writeInt8(parseInt('00000000', 2), 2);
spiResult = pigpiod.spi_xfer(pi, mcpSpiHandle, txBuf, rxBuf, 3);
a2dVal  = (rxBuf.readInt8(1) << 8) & 0b111100000000; // first 4 bit
a2dVal |=  rxBuf.readInt8(2)       & 0b000011111111; // last  8 bit
console.log(`Channel 0, a2dVal = ${a2dVal}`);

// Read value from Channel 1
txBuf.writeInt8(parseInt('00000110', 2), 0);
txBuf.writeInt8(parseInt('01000000', 2), 1); // channel 1
txBuf.writeInt8(parseInt('00000000', 2), 2);
spiResult = pigpiod.spi_xfer(pi, mcpSpiHandle, txBuf, rxBuf, 3);
a2dVal  = (rxBuf.readInt8(1) << 8) & 0b111100000000; // first 4 bit
a2dVal |=  rxBuf.readInt8(2)       & 0b000011111111; // last  8 bit
console.log(`Channel 1, a2dVal = ${a2dVal}`);

pigpiod.spi_close(pi, mcpSpiHandle);
pigpiod.pigpio_stop(pi);
