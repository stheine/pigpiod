'use strict';

// Reads the data of an MCP3204 A-D converter

/* xeslint-disable max-statements */
/* xeslint-disable complexity */
/* xeslint-disable sort-vars */
/* xeslint-disable no-cond-assign */
/* xeslint-disable no-console */
/* xeslint-disable no-bitwise */

// const moment  = require('moment');
const pigpiod = require('../lib/bindings.js');

const mcp3204 = function(pi, spiChannel, mcpChannel) {
  const txBuf = Buffer.allocUnsafe(3);
  const rxBuf = Buffer.allocUnsafe(3);
  let   a2dValue;

  const mcpSpiHandle = pigpiod.spi_open(pi, spiChannel, 500000, 0);

  // Configuring the MCP3204 serial communication.
  // We are sending 24 bits / 3 bytes of data.
  // This includes 12 bits of placeholders for the result we're receiving.
  //
  // Byte 0:
  //   Bit 7: 0 to not initiate communication yet
  //   Bit 6: 0 to not initiate communication yet
  //   Bit 5: 0 to not initiate communication yet
  //   Bit 4: 0 to not initiate communication yet
  //   Bit 3: 0 to not initiate communication yet
  //   Bit 2: 1 Start communication
  //   Bit 1: 1 to indicate single-ended mode (0 would be differential)
  //   Bit 0: 0 not used in MCP3204 (would be used in MCP3208 to allow more channels)
  // Byte 1:
  //   Bit 7: high-bit for channel selection
  //   Bit 6: low-bit for channel selection
  //   Bit 5: placeholder for output. Undefined. Sample taken.
  //   Bit 4: placeholder for output. Null bit.
  //   Bit 3: placeholder for output. Output bit 11
  //   Bit 2: placeholder for output. Output bit 10
  //   Bit 1: placeholder for output. Output bit  9
  //   Bit 0: placeholder for output. Output bit  8
  // Byte 2:
  //   Bit 7: placeholder for output. Output bit  7
  //   Bit 6: placeholder for output. Output bit  6
  //   Bit 5: placeholder for output. Output bit  5
  //   Bit 4: placeholder for output. Output bit  4
  //   Bit 3: placeholder for output. Output bit  3
  //   Bit 2: placeholder for output. Output bit  2
  //   Bit 1: placeholder for output. Output bit  1
  //   Bit 0: placeholder for output. Output bit  0
  txBuf.writeUInt8(parseInt('00000110', 2), 0);
  switch(mcpChannel) {
    case 0:
      txBuf.writeUInt8(parseInt('00000000', 2), 1); // channel 0
      break;

    case 1:
      txBuf.writeUInt8(parseInt('01000000', 2), 1); // channel 1
      break;

    case 2:
      txBuf.writeUInt8(parseInt('10000000', 2), 1); // channel 2
      break;

    case 3:
      txBuf.writeUInt8(parseInt('11000000', 2), 1); // channel 3
      break;

    default:
      throw new Error(`Unhandled MCP Channel ${mcpChannel}`);
  }
  txBuf.writeUInt8(parseInt('00000000', 2), 2);

  // Send the 3 bytes of data, and receive 3 bytes of data.
  pigpiod.spi_xfer(pi, mcpSpiHandle, txBuf, rxBuf, 3);

  // Extract only the output bits 0-11 from the received data.
  a2dValue  = (rxBuf.readUInt8(1) << 8) & 0b111100000000; // first 4 bit
  a2dValue |=  rxBuf.readUInt8(2)       & 0b000011111111; // last  8 bit

  // console.log(`Channel ${mcpChannel}, a2dValue = ${a2dValue}`);

  pigpiod.spi_close(pi, mcpSpiHandle);

  return a2dValue;
}



module.exports = {
  mcp3204
};
