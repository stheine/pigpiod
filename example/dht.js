'use strict';

/* eslint-disable max-statements */
/* eslint-disable complexity */
/* eslint-disable sort-vars */
/* eslint-disable no-cond-assign */
/* eslint-disable no-console */
/* eslint-disable no-bitwise */

const moment  = require('moment');
const pigpiod = require('../pigpiod.js');

const GPIO_DHT                  = 18;

const DHT_GOOD                  = 0;
const DHT_TIMEOUT               = 1;
const DHT_BAD_CHECKSUM          = 2;
const DHT_TEMP_OUT_OF_RANGE     = 3;
const DHT_HUMIDITY_OUT_OF_RANGE = 4;



const dhtData = {
  reading:     null,
  edgeLen:     null,
  lastTick:    null,
  receiving:   null,
  numBits:     null,
  bits:        [],
  temperature: null,
  humidity:    null,
  status:      null,
  timestamp:   null
};



const intrGpioDht = function(gpio, level, tick) {
  let chksum;
  let temperature;
  let humidity;
  let valid;

  dhtData.edgeLen  = tick - dhtData.lastTick;
  dhtData.lastTick = tick;

  if(dhtData.edgeLen > 10000) {
    // Init. The first interrupt will be a long egde, triggering.
    dhtData.receiving = true;
    dhtData.numBits   = -2; // Ignoriere die first two edges.
  } else if(dhtData.receiving) {
    dhtData.numBits++;
    if(dhtData.numBits >= 1) {
      if((dhtData.edgeLen >= 60) && (dhtData.edgeLen <= 100)) {
        /* 0 bit */
      } else if((dhtData.edgeLen > 100) && (dhtData.edgeLen <= 150)) {
        /* 1 bit */
        dhtData.bits[dhtData.numBits - 1] = 1;
      } else {
        /* invalid bit */
        dhtData.receiving = false;
      }

      if(dhtData.receiving && dhtData.numBits === 40
      ) {
        // Decode data
        //        +-------+
        // Temp C | 0-50  |
        //        +-------+
        // RH%    | 20-90 |
        //        +-------+
        //
        //    0      1      2      3      4
        // |0    7|8   15|16  23|24  31|32  39|
        // +------+------+------+------+------+
        // |   humidity  | temperature |  chk |
        // +------+------+------+------+------+
        chksum =
          (parseInt(dhtData.bits.slice( 0,  8).join(''), 2) +
           parseInt(dhtData.bits.slice( 8, 16).join(''), 2) +
           parseInt(dhtData.bits.slice(16, 24).join(''), 2) +
           parseInt(dhtData.bits.slice(24, 32).join(''), 2))
          & 0xff;
        valid = false;
        if(chksum === parseInt(dhtData.bits.slice(32, 40).join(''), 2)) {
          valid = true;

          temperature =
            parseInt(dhtData.bits.slice(16, 32).join(''), 2) / 10;
          if((temperature < -40) || (temperature > 80)) {
            valid = false;
            dhtData.status = DHT_TEMP_OUT_OF_RANGE;
          }

          humidity =
            parseInt(dhtData.bits.slice(0, 16).join(''), 2) / 10;
          if((humidity < 0) || (humidity > 100)) {
            valid = false;
            dhtData.status = DHT_HUMIDITY_OUT_OF_RANGE;
          }

          if(valid) {
            dhtData.temperature = temperature;
            dhtData.humidity    = humidity;
            dhtData.status      = DHT_GOOD;
            dhtData.timestamp   = moment();
          }
        } else {
          dhtData.status = DHT_BAD_CHECKSUM;
        }

        dhtData.reading = false;
      }
    }
  }
};

const dht = function(pi) {
  let   callbackId;
  let   i;
  const dhtResult = {
    timestamp:   null,
    status:      null,
    temperature: null,
    humidity:    null
  };

  dhtData.numBits   = 0;
  dhtData.receiving = false;
  dhtData.reading   = true;
  dhtData.lastTick  = pigpiod.get_current_tick(pi),
  dhtData.bits[39]  = 0;
  dhtData.bits.fill(0);

  return new Promise((resolveDht, rejectDht) => {
    if((callbackId =
      pigpiod.callback(pi, GPIO_DHT, pigpiod.RISING_EDGE, intrGpioDht)) < 0
    ) {
      throw new Error('Failed to pigpiod.callback()');
    }

    pigpiod.gpio_write(pi, GPIO_DHT, 0);
    pigpiod.set_mode(pi, GPIO_DHT, pigpiod.PI_INPUT);
    setTimeout(() => {
      new Promise(resolveReading => {
        /* timeout if reading does not stop */
        let   timeoutCheckNum = 5;
        const timeoutCheckInterval = setInterval(() => {
          if(dhtData.reading) {
            // Still reading
            timeoutCheckNum--;
            if(!timeoutCheckNum) {
              clearInterval(timeoutCheckInterval);
              return resolveReading();
            }
          } else {
            clearInterval(timeoutCheckInterval);
            return resolveReading();
          }
        }, 50);
      })
      .then(() => {
        if(dhtData.reading) {
          // Reading wasn't finished
          dhtResult.timestamp   = moment();
          dhtResult.status      = DHT_TIMEOUT;
          dhtResult.temperature = -1.0;
          dhtResult.humidity    = -1.0;
        } else {
          dhtResult.timestamp   = dhtData.timestamp;
          dhtResult.status      = dhtData.status;
          dhtResult.temperature = dhtData.temperature;
          dhtResult.humidity    = dhtData.humidity;
        }

        pigpiod.callback_cancel(callbackId);

        pigpiod.set_mode(pi, GPIO_DHT, pigpiod.PI_OUTPUT);
        pigpiod.gpio_write(pi, GPIO_DHT, 1);

        return resolveDht(dhtResult);
      });
    }, 18);
  });
};



let pi;

if((pi = pigpiod.pigpio_start()) < 0) {
  throw new Error('Failed to pigpiod.pidpio_start()');
}

dht(pi).then(result => {
  if(result.status) {
    console.log(result.status);
  } else {
    console.log(result);
  }

  pigpiod.pigpio_stop(pi);

  process.exit(result.status);
});
