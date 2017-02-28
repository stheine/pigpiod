# pigpiod

Node.js interface for **pigpiod** on the Raspberry Pi Zero, 1, 2, or 3.

## Contents

## Features

## Installation

#### Step 1

This step can be skipped on Raspbian Jessie 2016-05-10 or newer as it includes the pigpio C library and pigpiod.

The pigpio package is based on the
[pigpio C library](https://github.com/joan2937/pigpio) so the C library needs to be installed first. Version V41 or higher of the pigpio C library is
required. It can be installed with the following commands:

```
wget abyz.co.uk/rpi/pigpio/pigpio.zip
unzip pigpio.zip
cd PIGPIO
make
sudo make install
```

#### Step 2

```
npm install pigpio
```

I have developed and tested on Node.js 6.2.0, so it should be working ok here. It might very well work on the earlier versions of Node.js.

## Usage

```
const pigpiod = require('pigpiod');

let pi  = pigpiod.pigpio_start();
let spi = pigpiod.spi_open(pi, 0); // SPI channel 0

// read A-D converter value on MCP3204 on SPI channel 0, MCP channel 0
const a2dValue = pigpiod.mcp3204(pi, spi, 0);
console.log(`a2dValue = ${a2dValue}`);

// DHT22 (this is a C-based implementation reading the sensor data)
dhtResult = pigpiod.dht22(pi, 18); // read DHT22 sensor on port 18
console.log(dhtResult);

pigpiod.spi_close(pi, spi);
pigpiod.pigpio_stop(pi);
```

## Standard pigpiod API

Following APIs are already implemented. See for details: http://abyz.co.uk/rpi/pigpio/pdif2.html

Where not explicitly stated, the parameters are the same.

The error handling is different, though: Instead of returning an error code, an exception is thrown.

| | ESSENTIAL | |
| --- | --- | --- |
| [x] | pigpio_start | Connects to a pigpio daemon |
| [x] | pigpio_stop | Disconnects from a pigpio daemon |

| | BEGINNER | |
| --- | --- | --- |
| [x] | set_mode | Set a GPIO mode |
| [x] | get_mode | Get a GPIO mode |
| [x] | set_pull_up_down | Set/clear GPIO pull up/down resistor |
| [x] | gpio_read | Read a GPIO |
| [x] | gpio_write | Write a GPIO |
| [ ] | set_PWM_dutycycle | Start/stop PWM pulses on a GPIO |
| [ ] | get_PWM_dutycycle | Get the PWM dutycycle in use on a GPIO |
| [ ] | set_servo_pulsewidth | Start/stop servo pulses on a GPIO |
| [ ] | get_servo_pulsewidth | Get the servo pulsewidth in use on a GPIO |
| [x] | callback | Create GPIO level change callback |
| [ ] | callback_ex | Create GPIO level change callback |
| [x] | callback_cancel | Cancel a callback |
| [ ] | wait_for_edge | Wait for GPIO level change |

| | INTERMEDIATE | |
| --- | --- | --- |
| [ ] | gpio_trigger | Send a trigger pulse to a GPIO. |
| [x] | set_watchdog | Set a watchdog on a GPIO. |
| [ ] | set_PWM_range | Configure PWM range for a GPIO |
| [ ] | get_PWM_range | Get configured PWM range for a GPIO |
| [ ] | set_PWM_frequency | Configure PWM frequency for a GPIO |
| [ ] | get_PWM_frequency | Get configured PWM frequency for a GPIO |
| [ ] | read_bank_1 | Read all GPIO in bank 1 |
| [ ] | read_bank_2 | Read all GPIO in bank 2 |
| [ ] | clear_bank_1 | Clear selected GPIO in bank 1 |
| [ ] | clear_bank_2 | Clear selected GPIO in bank 2 |
| [ ] | set_bank_1 | Set selected GPIO in bank 1 |
| [ ] | set_bank_2 | Set selected GPIO in bank 2 |
| [ ] | start_thread | Start a new thread |
| [ ] | stop_thread | Stop a previously started thread |

| | ADVANCED | |
| --- | --- | --- |
| [ ] | get_PWM_real_range | Get underlying PWM range for a GPIO |
| [ ] | notify_open | Request a notification handle |
| [ ] | notify_begin | Start notifications for selected GPIO |
| [ ] | notify_pause | Pause notifications |
| [ ] | notify_close | Close a notification |
| [ ] | bb_serial_read_open | Opens a GPIO for bit bang serial reads |
| [ ] | bb_serial_read | Reads bit bang serial data from a GPIO |
| [ ] | bb_serial_read_close | Closes a GPIO for bit bang serial reads |
| [ ] | bb_serial_invert | Invert serial logic (1 invert, 0 normal) |
| [ ] | hardware_clock | Start hardware clock on supported GPIO |
| [ ] | hardware_PWM | Start hardware PWM on supported GPIO |
| [x] | set_glitch_filter | Set a glitch filter on a GPIO |
| [x] | set_noise_filter | Set a noise filter on a GPIO |

| | SCRIPTS | |
| --- | --- | --- |
| [ ] | store_script | Store a script |
| [ ] | run_script | Run a stored script |
| [ ] | script_status | Get script status and parameters |
| [ ] | stop_script | Stop a running script |
| [ ] | delete_script | Delete a stored script |

| | WAVES | |
| --- | --- | --- |
| [ ] | wave_clear | Deletes all waveforms |
| [ ] | wave_add_new | Starts a new waveform |
| [ ] | wave_add_generic | Adds a series of pulses to the waveform |
| [ ] | wave_add_serial | Adds serial data to the waveform |
| [ ] | wave_create | Creates a waveform from added data |
| [ ] | wave_delete | Deletes one or more waveforms |
| [ ] | wave_send_once | Transmits a waveform once |
| [ ] | wave_send_repeat | Transmits a waveform repeatedly |
| [ ] | wave_send_using_mode | Transmits a waveform in the chosen mode |
| [ ] | wave_chain | Transmits a chain of waveforms |
| [ ] | wave_tx_at | Returns the current transmitting waveform |
| [ ] | wave_tx_busy | Checks to see if the waveform has ended |
| [ ] | wave_tx_stop | Aborts the current waveform |
| [ ] | wave_get_micros | Length in microseconds of the current waveform |
| [ ] | wave_get_high_micros | Length of longest waveform so far |
| [ ] | wave_get_max_micros | Absolute maximum allowed micros |
| [ ] | wave_get_pulses | Length in pulses of the current waveform |
| [ ] | wave_get_high_pulses | Length of longest waveform so far |
| [ ] | wave_get_max_pulses | Absolute maximum allowed pulses |
| [ ] | wave_get_cbs | Length in cbs of the current waveform |
| [ ] | wave_get_high_cbs | Length of longest waveform so far |
| [ ] | wave_get_max_cbs | Absolute maximum allowed cbs |

| | I2C | |
| --- | --- | --- |
| [ ] | i2c_open | Opens an I2C device |
| [ ] | i2c_close | Closes an I2C device |
| [ ] | i2c_write_quick | smbus write quick |
| [ ] | i2c_write_byte | smbus write byte |
| [ ] | i2c_read_byte | smbus read byte |
| [ ] | i2c_write_byte_data | smbus write byte data |
| [ ] | i2c_write_word_data | smbus write word data |
| [ ] | i2c_read_byte_data | smbus read byte data |
| [ ] | i2c_read_word_data | smbus read word data |
| [ ] | i2c_process_call | smbus process call |
| [ ] | i2c_write_block_data | smbus write block data |
| [ ] | i2c_read_block_data | smbus read block data |
| [ ] | i2c_block_process_call | smbus block process call |
| [ ] | i2c_write_i2c_block_data | smbus write I2C block data |
| [ ] | i2c_read_i2c_block_data | smbus read I2C block data |
| [ ] | i2c_read_device | Reads the raw I2C device |
| [ ] | i2c_write_device | Writes the raw I2C device |
| [ ] | i2c_zip | Performs multiple I2C transactions |
| [ ] | bb_i2c_open | Opens GPIO for bit banging I2C |
| [ ] | bb_i2c_close | Closes GPIO for bit banging I2C |
| [ ] | bb_i2c_zip | Performs multiple bit banged I2C transactions |

| | SPI | |
| --- | --- | --- |
| [x] | spi_open | Opens a SPI device |
| [x] | spi_close | Closes a SPI device |
| [ ] | spi_read | Reads bytes from a SPI device |
| [ ] | spi_write | Writes bytes to a SPI device |
| [x] | spi_xfer | Transfers bytes with a SPI device |

| | SERIAL |
| --- | --- | --- |
| [x] | serial_open | Opens a serial device (/dev/tty*) |
| [x] | serial_close | Closes a serial device |
| [x] | serial_write_byte | Writes a byte to a serial device |
| [x] | serial_read_byte | Reads a byte from a serial device |
| [x] | serial_write | Writes bytes to a serial device |
| [x] | serial_read | Reads bytes from a serial device |
| [x] | serial_data_available | Returns number of bytes ready to be read |

| | CUSTOM | |
| --- | --- | --- |
| [ ] | custom_1 | User custom function 1 |
| [ ] | custom_2 | User custom function 2 |

| | UTILITIES | |
| --- | --- | --- |
| [x] | get_current_tick | Get current tick (microseconds) |
| [x] | get_hardware_revision | Get hardware revision |
| [x] | get_pigpio_version | Get the pigpio version |
| [ ] | pigpiod_if_version | Get the pigpiod_if2 version |
| [ ] | pigpio_error | Get a text description of an error code. |
| [1] | time_sleep | Sleeps for a float number of seconds |
| [2] | time_time | Float number of seconds since the epoch |

1: use js setTimeout() instead.

2: use moment() or Date() instead.

## API documentation

## Thanks

[pigpio library and pigpiod](http://abyz.co.uk/rpi/pigpio/) Thanks to _joan2937_ for the development and documentation of the pigpio C library.

[pigpio](https://github.com/fivdi/pigpio) Thanks for _fivdi_ for his work on the pigpio module for Node.js. I used this as the base for my development and got additional development help.

# Breaking change

## 1.0.0

The `dht22` call has been switched to an asynchronous implementation, returning a promise.

## 2.0.0

I have to revert the changes released in 1.0.0, as the async `dht22` API works fine in a standalone example,
but causes intermittent process hangs in a project using additional API calls.
