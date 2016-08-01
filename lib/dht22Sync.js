'use strict';

// Reads the data of a DHT22 sensor, synchronous

const pigpiod = require('../lib/bindings.js');

const DHT_GOOD         = 0;
const DHT_BAD_CHECKSUM = 1;
const DHT_BAD_DATA     = 2;
const DHT_TIMEOUT      = 3;

const dht22Sync = function(pi, gpio) {
  const buf = Buffer.allocUnsafe(8);
  const dht22Data = {};

  pigpiod.dht22_get(pi, 18, buf);

  //    0      1      2      3      4
  // |0    7|8   15|16  23|24  31|32  39|
  // +------+------+------+------+------+
  // | chk  |  humidityLE |temperatureLE|
  // +------+------+------+------+------+
  const chksum =
    (buf.readUInt8(1) +
     buf.readUInt8(2) +
     buf.readUInt8(3) +
     buf.readUInt8(4))
    & 0xff;
  let valid = false;
  if(chksum === buf.readUInt8(0)) {
    valid = true;

    const temperature = buf.readInt16LE(1) / 10;
    if((temperature < -40) || (temperature > 80)) {
      valid = false;
      dht22Data.status = DHT_BAD_DATA;
    }

    const humidity = buf.readInt16LE(3) / 10;
    if((humidity < 0) || (humidity > 100)) {
      valid = false;
      dht22Data.status = DHT_BAD_DATA;
    }

    if(chksum === 0 &&
       temperature === 0 &&
       humidity === 0
    ) {
      valid = false;
      dht22Data.status = DHT_BAD_DATA;
    }

    if(valid) {
      dht22Data.temperature = temperature;
      dht22Data.humidity    = humidity;
      dht22Data.status      = DHT_GOOD;
    }
  } else {
    dht22Data.status = DHT_BAD_CHECKSUM;
  }

  return dht22Data;
};

module.exports = {
  DHT_GOOD,
  DHT_BAD_CHECKSUM,
  DHT_BAD_DATA,
  DHT_TIMEOUT,
  dht22Sync
};
