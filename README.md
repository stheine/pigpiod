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

## API documentation

## Thanks

[pigpio library and pigpiod](http://abyz.co.uk/rpi/pigpio/) Thanks to _joan2937_ for the development and documentation of the pigpio C library.
[pigpio](https://github.com/fivdi/pigpio) Thanks for fivdi for his work on the pigpio module for Node.js. I used this as the base for my development and got additional development help.
