#include <errno.h>
#include <pigpiod_if2.h>
#include <nan.h>

// time_time -> replace by Date or moment in js
// time_sleep -> replace by setTimeout in js


#if NODE_VERSION_AT_LEAST(0, 11, 13)
static void gpioISREventLoopHandler(uv_async_t* handle);
#else
static void gpioISREventLoopHandler(uv_async_t* handle, int status);
#endif

// TODO errors returned by uv calls are ignored



// ###########################################################################
// Error handling
// ###########################################################################

void ThrowPigpiodError(int err, const char *pigpiodcall) {
  char buf[128];

  snprintf(buf, sizeof(buf), "pigpiod error %d in %s", err, pigpiodcall);

  Nan::ThrowError(buf);
}



// ###########################################################################
// Callback handling from C -> javascript
// ###########################################################################

class GpioCallback_t {
public:
  GpioCallback_t() : callback_(0) {
  }

  virtual ~GpioCallback_t() {
    if (callback_) {
      uv_unref((uv_handle_t *) &async_);
      delete callback_;
    }

    uv_close((uv_handle_t *) &async_, 0);

    callback_ = 0;
  }

  void AsyncSend() {
    uv_async_send(&async_);
  }

  void SetCallback(Nan::Callback *callback) {
    if (callback_) {
      uv_unref((uv_handle_t *) &async_);
      delete callback_;
    }

    callback_ = callback;

    if (callback_) {
      uv_ref((uv_handle_t *) &async_);
    }
  }

  Nan::Callback *Callback() {
    return callback_;
  }

protected:
  uv_async_t async_;

private:
  Nan::Callback *callback_;
};


class GpioISR_t : public GpioCallback_t {
public:
  GpioISR_t() : GpioCallback_t() {
    uv_async_init(uv_default_loop(), &async_, gpioISREventLoopHandler);

    // Prevent async from keeping event loop alive, for the time being.
    uv_unref((uv_handle_t *) &async_);
  }
};


static GpioISR_t gpioISR_g[PI_MAX_USER_GPIO + 1];

// Globals, protected by semaphore, passing the callback parameters
// from the C-handler into the js-event-loop
static int gpio_g;
static int level_g;
static uint32_t tick_g;
static uv_sem_t sem_g;


// gpioISRHandler is not executed in the event loop thread
static void gpioISRHandler(int pi, unsigned gpio, unsigned level, uint32_t tick) {
  uv_sem_wait(&sem_g);

  gpio_g  = gpio;
  level_g = level;
  tick_g  = tick;

  gpioISR_g[gpio].AsyncSend();
}


// gpioISREventLoopHandler is executed in the event loop thread.
#if NODE_VERSION_AT_LEAST(0, 11, 13)
static void gpioISREventLoopHandler(uv_async_t* handle) {
#else
static void gpioISREventLoopHandler(uv_async_t* handle, int status) {
#endif
  Nan::HandleScope scope;

  if (gpioISR_g[gpio_g].Callback()) {
    v8::Local<v8::Value> args[3] = {
      Nan::New<v8::Integer>(gpio_g),
      Nan::New<v8::Integer>(level_g),
      Nan::New<v8::Integer>(tick_g)
    };
    gpioISR_g[gpio_g].Callback()->Call(3, args);
  }

  uv_sem_post(&sem_g);
}


static NAN_METHOD(callback) {
  if(info.Length() < 4    ||
     !info[0]->IsInt32()  || // pi
     !info[1]->IsUint32() || // gpio
     !info[2]->IsUint32() || // edge
     !info[3]->IsFunction()  // handler
  ) {
    return Nan::ThrowError(Nan::ErrnoException(EINVAL, "callback", ""));
  }

  int      pi   = info[0]->Int32Value();
  unsigned gpio = info[1]->Uint32Value();
  unsigned edge = info[2]->Uint32Value();
  Nan::Callback *nanCallback = new Nan::Callback(info[3].As<v8::Function>());
  CBFunc_t callbackFunc = gpioISRHandler;
  gpioISR_g[gpio].SetCallback(nanCallback);

  int rc = callback(pi, gpio, edge, callbackFunc);
  if(rc < 0) {
    return ThrowPigpiodError(rc, "callback");
  }

  info.GetReturnValue().Set(rc);
}


static NAN_METHOD(callback_cancel) {
  if(info.Length() < 1    ||
     !info[0]->IsUint32()    // callback_id
  ) {
    return Nan::ThrowError(Nan::ErrnoException(EINVAL, "callback_cancel", ""));
  }

  unsigned callback_id = info[0]->Uint32Value();

  int rc = callback_cancel(callback_id);
  if(rc != 0) {
    return ThrowPigpiodError(rc, "callback_cancel");
  }

  info.GetReturnValue().Set(rc);
}



// ###########################################################################
// Essential
// ###########################################################################

NAN_METHOD(pigpio_start) {
  if(info.Length() < 2    ||
     !info[0]->IsString() || // addStr
     !info[1]->IsString()    // portStr
  ) {
    return Nan::ThrowError(Nan::ErrnoException(EINVAL, "pigpio_start", ""));
  }

  char* addrStr = v8ToCharPtr(info[0]->ToString());
  char* portStr = v8ToCharPtr(info[1]->ToString());

  int rc = pigpio_start(addrStr, portStr);
  if (rc < 0) {
    return ThrowPigpiodError(rc, "pigpio_start");
  }

  info.GetReturnValue().Set(rc);
}


NAN_METHOD(pigpio_stop) {
  if(info.Length() < 1 ||
     !info[0]->IsInt32() // pi
  ) {
    return Nan::ThrowError(Nan::ErrnoException(EINVAL, "pigpio_stop", ""));
  }

  int pi = info[0]->Int32Value();

  pigpio_stop(pi);
}



// ###########################################################################
// Beginner
// ###########################################################################

NAN_METHOD(set_mode) {
  if(info.Length() < 3 ||
     !info[0]->IsInt32()  || // pi
     !info[1]->IsUint32() || // gpio
     !info[2]->IsUint32()    // mode
  ) {
    return Nan::ThrowError(Nan::ErrnoException(EINVAL, "set_mode", ""));
  }

  int      pi   = info[0]->Int32Value();
  unsigned gpio = info[1]->Uint32Value();
  unsigned mode = info[2]->Uint32Value();

  int rc = set_mode(pi, gpio, mode);
  if (rc != 0) {
    return ThrowPigpiodError(rc, "set_mode");
  }
}


NAN_METHOD(get_mode) {
  if(info.Length() < 2   ||
     !info[0]->IsInt32() || // pi
     !info[1]->IsUint32()   // gpio
  ) {
    return Nan::ThrowError(Nan::ErrnoException(EINVAL, "get_mode", ""));
  }

  int      pi   = info[0]->Int32Value();
  unsigned gpio = info[1]->Uint32Value();

  int rc = get_mode(pi, gpio);
  if (rc < 0) {
    return ThrowPigpiodError(rc, "get_mode");
  }

  info.GetReturnValue().Set(rc);
}


NAN_METHOD(set_pull_up_down) {
  if(info.Length() < 2    ||
     !info[0]->IsInt32()  || // pi
     !info[1]->IsUint32() || // gpio
     !info[2]->IsUint32()    // pud
  ) {
    return Nan::ThrowError(Nan::ErrnoException(EINVAL, "set_pull_up_down", ""));
  }

  int      pi   = info[0]->Int32Value();
  unsigned gpio = info[1]->Uint32Value();
  unsigned pud  = info[2]->Uint32Value();

  int rc = set_pull_up_down(pi, gpio, pud);
  if(rc < 0) {
    return ThrowPigpiodError(rc, "set_pull_up_down");
  }

  info.GetReturnValue().Set(rc);
}


NAN_METHOD(gpio_read) {
  if(info.Length() < 2   ||
     !info[0]->IsInt32() || // pi
     !info[1]->IsUint32()   // gpio
  ) {
    return Nan::ThrowError(Nan::ErrnoException(EINVAL, "gpio_read", ""));
  }

  int      pi   = info[0]->Int32Value();
  unsigned gpio = info[1]->Uint32Value();

  int rc = gpio_read(pi, gpio);
  if(rc < 0) {
    return ThrowPigpiodError(rc, "gpio_read");
  }

  info.GetReturnValue().Set(rc);
}


NAN_METHOD(gpio_write) {
  if(info.Length() < 3    ||
     !info[0]->IsInt32()  || // pi
     !info[1]->IsUint32() || // gpio
     !info[2]->IsUint32()    // level
  ) {
    return Nan::ThrowError(Nan::ErrnoException(EINVAL, "gpio_write", ""));
  }

  int      pi    = info[0]->Int32Value();
  unsigned gpio  = info[1]->Uint32Value();
  unsigned level = info[2]->Uint32Value();

  int rc = gpio_write(pi, gpio, level);
  if(rc != 0) {
    return ThrowPigpiodError(rc, "gpio_write");
  }

  info.GetReturnValue().Set(rc);
}



// ###########################################################################
// Intermediate
// ###########################################################################

NAN_METHOD(set_watchdog) {
  if(info.Length() < 3    ||
     !info[0]->IsInt32()  || // pi
     !info[1]->IsUint32() || // gpio
     !info[2]->IsUint32()    // timeout
  ) {
    return Nan::ThrowError(Nan::ErrnoException(EINVAL, "set_watchdog", ""));
  }

  int      pi      = info[0]->Int32Value();
  unsigned gpio    = info[1]->Uint32Value();
  unsigned timeout = info[2]->Uint32Value();

  int rc = set_watchdog(pi, gpio, timeout);
  if(rc != 0) {
    return ThrowPigpiodError(rc, "set_watchdog");
  }

  info.GetReturnValue().Set(rc);
}



// ###########################################################################
// Advanced
// ###########################################################################

NAN_METHOD(set_glitch_filter) {
  if(info.Length() < 3    ||
     !info[0]->IsInt32()  || // pi
     !info[1]->IsUint32() || // gpio
     !info[2]->IsUint32()    // steady
  ) {
    return Nan::ThrowError(Nan::ErrnoException(EINVAL, "set_glitch_filter", ""));
  }

  int      pi     = info[0]->Int32Value();
  unsigned gpio   = info[1]->Uint32Value();
  unsigned steady = info[2]->Uint32Value();

  int rc = set_glitch_filter(pi, gpio, steady);
  if(rc != 0) {
    return ThrowPigpiodError(rc, "set_glitch_filter");
  }

  info.GetReturnValue().Set(rc);
}



NAN_METHOD(set_noise_filter) {
  if(info.Length() < 4    ||
     !info[0]->IsInt32()  || // pi
     !info[1]->IsUint32() || // gpio
     !info[2]->IsUint32() || // steady
     !info[3]->IsUint32()    // active
  ) {
    return Nan::ThrowError(Nan::ErrnoException(EINVAL, "set_noise_filter", ""));
  }

  int      pi     = info[0]->Int32Value();
  unsigned gpio   = info[1]->Uint32Value();
  unsigned steady = info[2]->Uint32Value();
  unsigned active = info[3]->Uint32Value();

  int rc = set_noise_filter(pi, gpio, steady, active);
  if(rc != 0) {
    return ThrowPigpiodError(rc, "set_noise_filter");
  }

  info.GetReturnValue().Set(rc);
}



// ###########################################################################
// SPI
// ###########################################################################

NAN_METHOD(spi_open) {
  if(info.Length() < 4    ||
     !info[0]->IsInt32()  || // pi
     !info[1]->IsUint32() || // spi_channel
     !info[2]->IsUint32() || // baud
     !info[3]->IsUint32()    // spi_flags
  ) {
    return Nan::ThrowError(Nan::ErrnoException(EINVAL, "spi_open", ""));
  }

  int      pi          = info[0]->Int32Value();
  unsigned spi_channel = info[1]->Uint32Value();
  unsigned baud        = info[2]->Uint32Value();
  unsigned spi_flags   = info[3]->Uint32Value();

  int rc = spi_open(pi, spi_channel, baud, spi_flags);
  if(rc < 0) {
    return ThrowPigpiodError(rc, "spi_open");
  }

  info.GetReturnValue().Set(rc);
}


NAN_METHOD(spi_close) {
  if(info.Length() < 2    ||
     !info[0]->IsInt32()  || // pi
     !info[1]->IsUint32()    // handle
  ) {
    return Nan::ThrowError(Nan::ErrnoException(EINVAL, "spi_close", ""));
  }

  int      pi     = info[0]->Int32Value();
  unsigned handle = info[1]->Uint32Value();

  int rc = spi_close(pi, handle);
  if(rc != 0) {
    return ThrowPigpiodError(rc, "spi_close");
  }

  info.GetReturnValue().Set(rc);
}


// printBits() - helper function for dumping binary values
void printBits(int numBytes, char *bytePtr) {
  char byte;
  int i, j;

  for(i = 0; i < numBytes; i++) {
    for(j = 7; j >= 0; j--) {
      byte = bytePtr[i] & (1<<j);
      byte >>= j;
      printf("%u", byte);
    }
  }
  printf("\n");
}


NAN_METHOD(spi_xfer) {
  if(info.Length() < 5    ||
     !info[0]->IsInt32()  || // pi
     !info[1]->IsUint32() || // handle
     !info[2]->IsObject() || // txBuf
     !info[3]->IsObject() || // rxBuf    -> output buffer
     !info[4]->IsUint32()    // count
  ) {
    return Nan::ThrowError(Nan::ErrnoException(EINVAL, "spi_xfer", ""));
  }

  int      pi     = info[0]->Int32Value();
  unsigned handle = info[1]->Uint32Value();
  char*    txBuf  = node::Buffer::Data(info[2]->ToObject());
  char*    rxBuf  = node::Buffer::Data(info[3]->ToObject());
  int      count  = (int)info[4]->Uint32Value();

//  printBits(count, txBuf); // debugging

  int rc = spi_xfer(pi, handle, txBuf, rxBuf, count);
  if(rc != count) {
    printf("Error from spi_xfer count=%d, rc=%d\n", count, rc);
    printBits(count, rxBuf); // debugging
    return ThrowPigpiodError(rc, "spi_xfer");
  }

//  printBits(count, rxBuf); // debugging

  info.GetReturnValue().Set(rc);
}



// ###########################################################################
// Serial
// ###########################################################################

char *v8ToCharPtr(v8::Local<v8::Value> v8Value) {
  v8::String::Utf8Value string(v8Value);
  char *charPtr = (char *) malloc(string.length() + 1);
  strcpy(charPtr, *string);
  return charPtr;
}


NAN_METHOD(serial_open) {
  if(info.Length() < 4    ||
     !info[0]->IsInt32()  || // pi
     !info[1]->IsString() || // ser_tty
     !info[2]->IsUint32() || // baud
     !info[3]->IsUint32()    // ser_flags
  ) {
    return Nan::ThrowError(Nan::ErrnoException(EINVAL, "serial_open", ""));
  }

  int      pi        = info[0]->Int32Value();
  char*    ser_tty   = v8ToCharPtr(info[1]->ToString());
  unsigned baud      = info[2]->Uint32Value();
  unsigned ser_flags = info[3]->Uint32Value();

  int rc = serial_open(pi, ser_tty, baud, ser_flags);
  free(ser_tty);
  if(rc < 0) {
    return ThrowPigpiodError(rc, "serial_open");
  }

  info.GetReturnValue().Set(rc);
}


NAN_METHOD(serial_close) {
  if(info.Length() < 2    ||
     !info[0]->IsInt32()  || // pi
     !info[1]->IsUint32()    // handle
  ) {
    return Nan::ThrowError(Nan::ErrnoException(EINVAL, "serial_close", ""));
  }

  int      pi     = info[0]->Int32Value();
  unsigned handle = info[1]->Uint32Value();

  int rc = serial_close(pi, handle);
  if(rc != 0) {
    return ThrowPigpiodError(rc, "serial_close");
  }

  info.GetReturnValue().Set(rc);
}


NAN_METHOD(serial_write_byte) {
  if(info.Length() < 3    ||
     !info[0]->IsInt32()  || // pi
     !info[1]->IsUint32() || // handle
     !info[2]->IsUint32()    // bVal
  ) {
    return Nan::ThrowError(Nan::ErrnoException(EINVAL, "serial_write_byte", ""));
  }

  int      pi     = info[0]->Int32Value();
  unsigned handle = info[1]->Uint32Value();
  unsigned bVal   = info[2]->Uint32Value();

  int rc = serial_write_byte(pi, handle, bVal);
  if(rc != 0) {
    return ThrowPigpiodError(rc, "serial_write_byte");
  }

  info.GetReturnValue().Set(rc);
}


NAN_METHOD(serial_read_byte) {
  if(info.Length() < 2    ||
     !info[0]->IsInt32()  || // pi
     !info[1]->IsUint32()    // handle
  ) {
    return Nan::ThrowError(Nan::ErrnoException(EINVAL, "serial_read_byte", ""));
  }

  int      pi     = info[0]->Int32Value();
  unsigned handle = info[1]->Uint32Value();

  int rc = serial_read_byte(pi, handle);
  if(rc < 0) {
    return ThrowPigpiodError(rc, "serial_read_byte");
  }

  info.GetReturnValue().Set(rc);
}


NAN_METHOD(serial_write) {
  if(info.Length() < 4    ||
     !info[0]->IsInt32()  || // pi
     !info[1]->IsUint32() || // handle
     !info[2]->IsString() || // buf
     !info[3]->IsUint32()    // count
  ) {
    return Nan::ThrowError(Nan::ErrnoException(EINVAL, "serial_write", ""));
  }

  int      pi     = info[0]->Int32Value();
  unsigned handle = info[1]->Uint32Value();
  char*    buf    = v8ToCharPtr(info[2]->ToString());
  unsigned count  = info[3]->Uint32Value();

  int rc = serial_write(pi, handle, buf, count);
  free(buf);
  if(rc != 0) {
    return ThrowPigpiodError(rc, "serial_write");
  }

  info.GetReturnValue().Set(rc);
}


NAN_METHOD(serial_read) {
  if(info.Length() < 4    ||
     !info[0]->IsInt32()  || // pi
     !info[1]->IsUint32() || // handle
     !info[2]->IsObject() || // buf    -> output buffer
     !info[3]->IsUint32()    // count
  ) {
    return Nan::ThrowError(Nan::ErrnoException(EINVAL, "serial_read", ""));
  }

  int      pi     = info[0]->Int32Value();
  unsigned handle = info[1]->Uint32Value();
  char*    buf    = node::Buffer::Data(info[2]->ToObject());
  unsigned count  = info[3]->Uint32Value();

  int rc = serial_read(pi, handle, buf, count);
  if(rc <= 0) {
    return ThrowPigpiodError(rc, "serial_read");
  }

  info.GetReturnValue().Set(rc);
}


NAN_METHOD(serial_data_available) {
  if(info.Length() < 2    ||
     !info[0]->IsInt32()  || // pi
     !info[1]->IsUint32()    // handle
  ) {
    return Nan::ThrowError(Nan::ErrnoException(EINVAL, "serial_data_available", ""));
  }

  int      pi     = info[0]->Int32Value();
  unsigned handle = info[1]->Uint32Value();

  int rc = serial_data_available(pi, handle);
  if(rc < 0) {
    return ThrowPigpiodError(rc, "serial_data_available");
  }

  info.GetReturnValue().Set(rc);
}





// ###########################################################################
// Utilities
// ###########################################################################

NAN_METHOD(get_current_tick) {
  if(info.Length() < 1   ||
     !info[0]->IsInt32()    // pi
  ) {
    return Nan::ThrowError(Nan::ErrnoException(EINVAL, "get_current_tick", ""));
  }

  int pi = info[0]->Int32Value();

  int rc = get_current_tick(pi);

  info.GetReturnValue().Set(rc);
}


NAN_METHOD(get_hardware_revision) {
  if(info.Length() < 1   ||
     !info[0]->IsInt32()    // pi
  ) {
    return Nan::ThrowError(Nan::ErrnoException(EINVAL, "get_hardware_revision", ""));
  }

  int pi = info[0]->Int32Value();

  int rc = get_hardware_revision(pi);

  if(rc == 0) {
    return ThrowPigpiodError(rc, "get_hardware_revision");
  }

  info.GetReturnValue().Set(rc);
}


NAN_METHOD(get_pigpio_version) {
  if(info.Length() < 1   ||
     !info[0]->IsInt32()    // pi
  ) {
    return Nan::ThrowError(Nan::ErrnoException(EINVAL, "get_pigpio_version", ""));
  }

  int pi = info[0]->Int32Value();

  int rc = get_pigpio_version(pi);

  if(rc < 0) {
    return ThrowPigpiodError(rc, "get_pigpio_version");
  }

  info.GetReturnValue().Set(rc);
}



// ###########################################################################
// DHT22
// This is a synchronous C implementation, as I failed to code a reliable
// js based implementation (as - due to node.js's nature - it's
// async handling might be busy with other tasks, so the
// interrupt/ callback handling needed to read the DHT22 data is too slow).
// The code is based on the example code taken from
// http://abyz.co.uk/rpi/pigpio/examples.html
// and I shrunk it down to the pure DHT22 handling, to make it as
// simple as possible to get it included here.
// Note: the function _decode_dht22() calculates temperature and humidity,
//       but in fact I'm returning the binary data and re-run that same
//       calculation in js. This is as I failed to return float/ double
//       data from C code back into js - let me know if you know...
// ###########################################################################

#define DHT_GOOD         0
#define DHT_BAD_CHECKSUM 1
#define DHT_BAD_DATA     2
#define DHT_TIMEOUT      3

struct DHT22_s;
typedef struct DHT22_s DHT22_t;

typedef struct
{
  float  temperature;
  float  humidity;
  int    status;
} DHT22_data_t;
typedef void (*DHT22_CB_t)(DHT22_data_t);

struct DHT22_s
{
  int          _cb_id;
  int          _in_code;
  union
  {
    char      *_char[8];
    uint8_t    _byte[8];
    uint64_t   _code;
  };
  int          _bits;
  int          _data_finished;
  DHT22_data_t _data;
  uint32_t     _last_edge_tick;
};

static void _decode_dht22(DHT22_t *self)
{
  uint8_t chksum;
  float div;
  float t, h;

  chksum = (self->_byte[1] + self->_byte[2] +
            self->_byte[3] + self->_byte[4]) & 0xFF;

  if (chksum == self->_byte[0])
  {
    h = ((float)((self->_byte[4]<<8) + self->_byte[3])) / 10.0;

    if (self->_byte[2] & 128)
    {
      div = -10.0;
    }
    else
    {
      div = 10.0;
    }

    t = ((float)(((self->_byte[2]&127)<<8) + self->_byte[1])) / div;

    if ((h > 110.0) || (t < -50.0) || (t > 135.0))
    {
      self->_data.status = DHT_BAD_DATA;
    }
    else
    {
      self->_data.temperature = t;
      self->_data.humidity    = h;
      self->_data.status      = DHT_GOOD;
    }
  }
  else
  {
    self->_data.status = DHT_BAD_CHECKSUM;
  }

  self->_data_finished = 1;
}

static void _cb(
  int cbPi, unsigned cbGpio, unsigned level, uint32_t tick, void *user)
{
  DHT22_t *self = (DHT22_t *)user;
  int edge_len;

  edge_len = tick - self->_last_edge_tick;
  self->_last_edge_tick = tick;

  if (edge_len > 10000)
  {
    self->_in_code = 1;
    self->_bits = -2;
    self->_code = 0;
  }
  else if (self->_in_code)
  {
    self->_bits++;
    if (self->_bits >= 1)
    {
      self->_code <<= 1;

      if ((edge_len >= 60) && (edge_len <= 100))
      {
        /* 0 bit */
      }
      else if ((edge_len > 100) && (edge_len <= 150))
      {
        /* 1 bit */
        self->_code += 1;
      }
      else
      {
        /* invalid bit */
        self->_in_code = 0;
      }

      if (self->_in_code)
      {
        if (self->_bits == 40)
        {
          _decode_dht22(self);
        }
      }
    }
  }
}

void DHT22(int pi, int gpio, char *buf)
{
  int i;
  DHT22_t *self;

  self = (DHT22_t *)malloc(sizeof(DHT22_t));
  if (!self) {
    return;
  }

  self->_data.temperature = 0.0;
  self->_data.humidity    = 0.0;
  self->_data.status      = 0;
  self->_in_code          = 0;
  self->_data_finished    = 0;
  self->_last_edge_tick = get_current_tick(pi) - 10000;

  set_mode(pi, gpio, PI_INPUT);

  self->_cb_id = callback_ex(pi, gpio, RISING_EDGE, _cb, self);

  self->_data_finished = 0;

  // Trigger read
  set_mode(pi, gpio, PI_OUTPUT);
  gpio_write(pi, gpio, 0);
  time_sleep(0.018);
  set_mode(pi, gpio, PI_INPUT);

  // timeout if no new reading
  for (i = 0; i < 5; i++) /* 0.25 seconds */
  {
    time_sleep(0.05);
    if (self->_data_finished)
    {
      break;
    }
  }

  if (!self->_data_finished)
  {
    memset((void *)buf, 0, 8);
  }
  else
  {
    memcpy((void *)buf, (void *)(&self->_code), 8);
  }

  if (self->_cb_id >= 0)
  {
    callback_cancel(self->_cb_id);
    self->_cb_id = -1;
  }
  free(self);
}


static NAN_METHOD(dht22_get) {
  if(info.Length() < 3    ||
     !info[0]->IsInt32()  || // pi
     !info[1]->IsUint32() || // gpio
     !info[2]->IsObject()    // buf    -> output buffer
  ) {
    return Nan::ThrowError(Nan::ErrnoException(EINVAL, "get_dht22", ""));
  }

  int      pi   = info[0]->Int32Value();
  unsigned gpio = info[1]->Uint32Value();
  char*    buf  = node::Buffer::Data(info[2]->ToObject());

  DHT22(pi, gpio, buf);
}



// ###########################################################################
// Module init
// ###########################################################################

static void SetConst(
  Nan::ADDON_REGISTER_FUNCTION_ARGS_TYPE target,
  const char* name,
  int value
) {
  Nan::Set(target,
    Nan::New<v8::String>(name).ToLocalChecked(),
    Nan::New<v8::Integer>(value)
  );
}


static void SetFunction(
  Nan::ADDON_REGISTER_FUNCTION_ARGS_TYPE target,
  const char* name,
  Nan::FunctionCallback callback
) {
  Nan::Set(target,
    Nan::New(name).ToLocalChecked(),
    Nan::GetFunction(Nan::New<v8::FunctionTemplate>(callback)).ToLocalChecked()
  );
}


NAN_MODULE_INIT(InitAll) {
  uv_sem_init(&sem_g, 1);

  /* mode constants */
  SetConst(target, "PI_INPUT", PI_INPUT);
  SetConst(target, "PI_OUTPUT", PI_OUTPUT);
  SetConst(target, "PI_ALT0", PI_ALT0);
  SetConst(target, "PI_ALT1", PI_ALT1);
  SetConst(target, "PI_ALT2", PI_ALT2);
  SetConst(target, "PI_ALT3", PI_ALT3);
  SetConst(target, "PI_ALT4", PI_ALT4);
  SetConst(target, "PI_ALT5", PI_ALT5);

  /* pud constants */
  SetConst(target, "PI_PUD_OFF", PI_PUD_OFF);
  SetConst(target, "PI_PUD_DOWN", PI_PUD_DOWN);
  SetConst(target, "PI_PUD_UP", PI_PUD_UP);

  /* isr constants */
  SetConst(target, "RISING_EDGE", RISING_EDGE);
  SetConst(target, "FALLING_EDGE", FALLING_EDGE);
  SetConst(target, "EITHER_EDGE", EITHER_EDGE);

  /* level constants */
  SetConst(target, "PI_OFF",  PI_OFF);
  SetConst(target, "PI_ON",   PI_ON);

  SetConst(target, "PI_CLEAR", PI_CLEAR);
  SetConst(target, "PI_SET",   PI_SET);

  SetConst(target, "PI_LOW",   PI_LOW);
  SetConst(target, "PI_HIGH",  PI_HIGH);

  /* timeout constant */
  SetConst(target, "PI_TIMEOUT", PI_TIMEOUT);

  /* gpio number constants */
  SetConst(target, "PI_MIN_GPIO", PI_MIN_GPIO);
  SetConst(target, "PI_MAX_GPIO", PI_MAX_GPIO);
  SetConst(target, "PI_MAX_USER_GPIO", PI_MAX_USER_GPIO);

  /* error code constants */
  SetConst(target, "PI_INIT_FAILED", PI_INIT_FAILED);

  /* gpioCfgClock cfgPeripheral constants */
  SetConst(target, "PI_CLOCK_PWM", PI_CLOCK_PWM);
  SetConst(target, "PI_CLOCK_PCM", PI_CLOCK_PCM);

  /* functions */
  SetFunction(target, "callback", callback);
  SetFunction(target, "callback_cancel", callback_cancel);
  SetFunction(target, "pigpio_start", pigpio_start);
  SetFunction(target, "pigpio_stop", pigpio_stop);
  SetFunction(target, "set_mode", set_mode);
  SetFunction(target, "get_mode", get_mode);
  SetFunction(target, "set_pull_up_down", set_pull_up_down);
  SetFunction(target, "gpio_read", gpio_read);
  SetFunction(target, "gpio_write", gpio_write);
  SetFunction(target, "set_watchdog", set_watchdog);
  SetFunction(target, "set_glitch_filter", set_glitch_filter);
  SetFunction(target, "set_noise_filter", set_noise_filter);
  SetFunction(target, "spi_open", spi_open);
  SetFunction(target, "spi_close", spi_close);
  SetFunction(target, "spi_xfer", spi_xfer);
  SetFunction(target, "serial_open", serial_open);
  SetFunction(target, "serial_close", serial_close);
  SetFunction(target, "serial_write_byte", serial_write_byte);
  SetFunction(target, "serial_read_byte", serial_read_byte);
  SetFunction(target, "serial_write", serial_write);
  SetFunction(target, "serial_read", serial_read);
  SetFunction(target, "serial_data_available", serial_data_available);
  SetFunction(target, "get_current_tick", get_current_tick);
  SetFunction(target, "get_hardware_revision", get_hardware_revision);
  SetFunction(target, "get_pigpio_version", get_pigpio_version);
  SetFunction(target, "dht22_get", dht22_get);
}

NODE_MODULE(pigpio, InitAll)
