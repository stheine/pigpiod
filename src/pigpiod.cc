#include <errno.h>
#include <pigpiod_if2.h>
#include <nan.h>

// TODO
// [x] gpio_read
// [x] gpio_write
// [x] pigpio_start
// [x] pigpio_stop
// [x] get_mode
// [x] set_mode
// [x] set_pull_up_down
// [x] set_watchdog
// [x] callback
// [ ] callback_cancel
// [x] set_glitch_filter
// [x] spi_open
// [x] spi_close
// [ ] spi_xfer

// [ ] time_time
// [ ] time_sleep
// [ ] get_current_tick


#if NODE_VERSION_AT_LEAST(0, 11, 13)
static void gpioISREventLoopHandler(uv_async_t* handle);
static void gpioAlertEventLoopHandler(uv_async_t* handle);
#else
static void gpioISREventLoopHandler(uv_async_t* handle, int status);
static void gpioAlertEventLoopHandler(uv_async_t* handle, int status);
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


class GpioAlert_t : public GpioCallback_t {
public:
  GpioAlert_t() : GpioCallback_t() {
    uv_async_init(uv_default_loop(), &async_, gpioAlertEventLoopHandler);

    // Prevent async from keeping event loop alive, for the time being.
    uv_unref((uv_handle_t *) &async_);
  }
};


static GpioISR_t gpioISR_g[PI_MAX_USER_GPIO + 1];
static GpioAlert_t gpioAlert_g[PI_MAX_USER_GPIO + 1];
// TODO: warum sind die global? koennten die nicht in die class?
// dann braeuchte ich den semaphore gar nicht...
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
}


// gpioAlertHandler is not executed in the event loop thread
static void gpioAlertHandler(int gpio, int level, uint32_t tick) {
  uv_sem_wait(&sem_g);

  gpio_g  = gpio;
  level_g = level;
  tick_g  = tick;

  gpioAlert_g[gpio].AsyncSend();
}


// gpioAlertEventLoopHandler is executed in the event loop thread.
#if NODE_VERSION_AT_LEAST(0, 11, 13)
static void gpioAlertEventLoopHandler(uv_async_t* handle) {
#else
static void gpioAlertEventLoopHandler(uv_async_t* handle, int status) {
#endif
  Nan::HandleScope scope;

  if (gpioAlert_g[gpio_g].Callback()) {
    v8::Local<v8::Value> args[3] = {
      Nan::New<v8::Integer>(gpio_g),
      Nan::New<v8::Integer>(level_g),
      Nan::New<v8::Integer>(tick_g)
    };
    gpioAlert_g[gpio_g].Callback()->Call(3, args);
  }

  uv_sem_post(&sem_g);
}


// TODO for pigpiod
static NAN_METHOD(gpioSetAlertFunc) {
  if (info.Length() < 1 ||
      !info[0]->IsUint32()) {
    return Nan::ThrowError(Nan::ErrnoException(EINVAL, "gpioSetAlertFunc"));
  }

  if (info.Length() >= 2 &&
      !info[1]->IsFunction() &&
      !info[1]->IsNull() &&
      !info[1]->IsUndefined()) {
    return Nan::ThrowError(Nan::ErrnoException(EINVAL, "gpioSetAlertFunc"));
  }

  unsigned user_gpio = info[0]->Uint32Value();
  Nan::Callback *callback = 0;
  gpioAlertFunc_t alertFunc = 0;

  if (info.Length() >= 2 && info[1]->IsFunction()) {
    callback = new Nan::Callback(info[1].As<v8::Function>());
    alertFunc = gpioAlertHandler;
  }

  gpioAlert_g[user_gpio].SetCallback(callback);

  int rc = gpioSetAlertFunc(user_gpio, alertFunc);
  if (rc < 0) {
    return ThrowPigpiodError(rc, "gpioSetAlertFunc");
  }
}



// ###########################################################################
// Essential
// ###########################################################################

NAN_METHOD(pigpio_start) {
  int rc = pigpio_start(NULL, NULL);
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
}


char *v8ToCharPtr(v8::Local<v8::Value> v8Value) {
  v8::String::Utf8Value string(v8Value);
  char *charPtr = (char *) malloc(string.length() + 1);
  strcpy(charPtr, *string);
  return charPtr;
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
     !info[3]->IsObject() || // rxBuf
     !info[4]->IsUint32()    // count
  ) {
    return Nan::ThrowError(Nan::ErrnoException(EINVAL, "spi_xfer", ""));
  }

  int      pi     = info[0]->Int32Value();
  unsigned handle = info[1]->Uint32Value();
  char*    txBuf  = node::Buffer::Data(info[2]->ToObject());
  char*    rxBuf  = node::Buffer::Data(info[3]->ToObject());
  int      count  = (int)info[4]->Uint32Value();

//  printBits(count, txBuf); // TODO weg

  int rc = spi_xfer(pi, handle, txBuf, rxBuf, count);
  if(rc != count) {
    return ThrowPigpiodError(rc, "spi_xfer");
  }

//  printBits(count, rxBuf); // TODO weg

  info.GetReturnValue().Set(rc);
}






// ###########################################################################
// TODO for pigpiod
// ###########################################################################


// TODO for pigpiod
NAN_METHOD(gpioTrigger) {
  if (info.Length() < 3 || !info[0]->IsUint32() || !info[1]->IsUint32() || !info[2]->IsUint32()) {
    return Nan::ThrowError(Nan::ErrnoException(EINVAL, "gpioTrigger", ""));
  }

  unsigned gpio = info[0]->Uint32Value();
  unsigned pulseLen = info[1]->Uint32Value();
  unsigned level = info[2]->Uint32Value();

  int rc = gpioTrigger(gpio, pulseLen, level);
  if (rc < 0) {
    return ThrowPigpiodError(rc, "gpioTrigger");
  }
}


// TODO for pigpiod
NAN_METHOD(gpioPWM) {
  if (info.Length() < 2 || !info[0]->IsUint32() || !info[1]->IsUint32()) {
    return Nan::ThrowError(Nan::ErrnoException(EINVAL, "gpioPWM", ""));
  }

  unsigned user_gpio = info[0]->Uint32Value();
  unsigned dutycycle = info[1]->Uint32Value();

  int rc = gpioPWM(user_gpio, dutycycle);
  if (rc < 0) {
    return ThrowPigpiodError(rc, "gpioPWM");
  }
}


// TODO for pigpiod
NAN_METHOD(gpioHardwarePWM) {
  if (info.Length() < 3 ||
      !info[0]->IsUint32() ||
      !info[1]->IsUint32() ||
      !info[2]->IsUint32()) {
    return Nan::ThrowError(Nan::ErrnoException(EINVAL, "gpioHardwarePWM", ""));
  }

  unsigned gpio = info[0]->Uint32Value();
  unsigned frequency = info[1]->Uint32Value();
  unsigned dutycycle = info[2]->Uint32Value();

  int rc = gpioHardwarePWM(gpio, frequency, dutycycle);
  if (rc < 0) {
    return ThrowPigpiodError(rc, "gpioHardwarePWM");
  }
}


// TODO for pigpiod
NAN_METHOD(gpioGetPWMdutycycle) {
  if (info.Length() < 1 || !info[0]->IsUint32()) {
    return Nan::ThrowError(Nan::ErrnoException(EINVAL, "gpioGetPWMdutycycle", ""));
  }

  unsigned user_gpio = info[0]->Uint32Value();

  int rc = gpioGetPWMdutycycle(user_gpio);
  if (rc < 0) {
    return ThrowPigpiodError(rc, "gpioGetPWMdutycycle");
  }

  info.GetReturnValue().Set(rc);
}


// TODO for pigpiod
NAN_METHOD(gpioSetPWMrange) {
  if (info.Length() < 2 || !info[0]->IsUint32() || !info[1]->IsUint32()) {
    return Nan::ThrowError(Nan::ErrnoException(EINVAL, "gpioSetPWMrange", ""));
  }

  unsigned user_gpio = info[0]->Uint32Value();
  unsigned range = info[1]->Uint32Value();

  int rc = gpioSetPWMrange(user_gpio, range);
  if (rc < 0) {
    return ThrowPigpiodError(rc, "gpioSetPWMrange");
  }
}


// TODO for pigpiod
NAN_METHOD(gpioGetPWMrange) {
  if (info.Length() < 1 || !info[0]->IsUint32()) {
    return Nan::ThrowError(Nan::ErrnoException(EINVAL, "gpioGetPWMrange", ""));
  }

  unsigned user_gpio = info[0]->Uint32Value();

  int rc = gpioGetPWMrange(user_gpio);
  if (rc < 0) {
    return ThrowPigpiodError(rc, "gpioGetPWMrange");
  }

  info.GetReturnValue().Set(rc);
}


// TODO for pigpiod
NAN_METHOD(gpioGetPWMrealRange) {
  if (info.Length() < 1 || !info[0]->IsUint32()) {
    return Nan::ThrowError(Nan::ErrnoException(EINVAL, "gpioGetPWMrealRange", ""));
  }

  unsigned user_gpio = info[0]->Uint32Value();

  int rc = gpioGetPWMrealRange(user_gpio);
  if (rc < 0) {
    return ThrowPigpiodError(rc, "gpioGetPWMrealRange");
  }

  info.GetReturnValue().Set(rc);
}


// TODO for pigpiod
NAN_METHOD(gpioSetPWMfrequency) {
  if (info.Length() < 2 || !info[0]->IsUint32() || !info[1]->IsUint32()) {
    return Nan::ThrowError(Nan::ErrnoException(EINVAL, "gpioSetPWMfrequency", ""));
  }

  unsigned user_gpio = info[0]->Uint32Value();
  unsigned frequency = info[1]->Uint32Value();

  int rc = gpioSetPWMfrequency(user_gpio, frequency);
  if (rc < 0) {
    return ThrowPigpiodError(rc, "gpioSetPWMfrequency");
  }
}


// TODO for pigpiod
NAN_METHOD(gpioGetPWMfrequency) {
  if (info.Length() < 1 || !info[0]->IsUint32()) {
    return Nan::ThrowError(Nan::ErrnoException(EINVAL, "gpioGetPWMfrequency", ""));
  }

  unsigned user_gpio = info[0]->Uint32Value();

  int rc = gpioGetPWMfrequency(user_gpio);
  if (rc < 0) {
    return ThrowPigpiodError(rc, "gpioGetPWMfrequency");
  }

  info.GetReturnValue().Set(rc);
}


// TODO for pigpiod
NAN_METHOD(gpioServo) {
  if (info.Length() < 2 || !info[0]->IsUint32() || !info[1]->IsUint32()) {
    return Nan::ThrowError(Nan::ErrnoException(EINVAL, "gpioServo", ""));
  }

  unsigned user_gpio = info[0]->Uint32Value();
  unsigned pulsewidth = info[1]->Uint32Value();

  int rc = gpioServo(user_gpio, pulsewidth);
  if (rc < 0) {
    return ThrowPigpiodError(rc, "gpioServo");
  }
}


// TODO for pigpiod
NAN_METHOD(gpioGetServoPulsewidth) {
  if (info.Length() < 1 || !info[0]->IsUint32()) {
    return Nan::ThrowError(Nan::ErrnoException(EINVAL, "gpioGetServoPulsewidth", ""));
  }

  unsigned user_gpio = info[0]->Uint32Value();

  int rc = gpioGetServoPulsewidth(user_gpio);
  if (rc < 0) {
    return ThrowPigpiodError(rc, "gpioGetServoPulsewidth");
  }

  info.GetReturnValue().Set(rc);
}


/* ------------------------------------------------------------------------ */
/* GpioBank                                                                 */
/* ------------------------------------------------------------------------ */


NAN_METHOD(GpioReadBits_0_31) {
  info.GetReturnValue().Set(gpioRead_Bits_0_31());
}


NAN_METHOD(GpioReadBits_32_53) {
  info.GetReturnValue().Set(gpioRead_Bits_32_53());
}


NAN_METHOD(GpioWriteBitsSet_0_31) {
  if (info.Length() < 1 || !info[0]->IsUint32()) {
    return Nan::ThrowError(Nan::ErrnoException(EINVAL, "GpioWriteBitsSet_0_31", ""));
  }

  unsigned bits = info[0]->Uint32Value();

  int rc = gpioWrite_Bits_0_31_Set(bits);
  if (rc < 0) {
    return ThrowPigpiodError(rc, "GpioWriteBitsSet_0_31");
  }

  info.GetReturnValue().Set(rc);
}


NAN_METHOD(GpioWriteBitsSet_32_53) {
  if (info.Length() < 1 || !info[0]->IsUint32()) {
    return Nan::ThrowError(Nan::ErrnoException(EINVAL, "GpioWriteBitsSet_32_53", ""));
  }

  unsigned bits = info[0]->Uint32Value();

  int rc = gpioWrite_Bits_32_53_Set(bits);
  if (rc < 0) {
    return ThrowPigpiodError(rc, "GpioWriteBitsSet_32_53");
  }

  info.GetReturnValue().Set(rc);
}


NAN_METHOD(GpioWriteBitsClear_0_31) {
  if (info.Length() < 1 || !info[0]->IsUint32()) {
    return Nan::ThrowError(Nan::ErrnoException(EINVAL, "GpioWriteBitsClear_0_31", ""));
  }

  unsigned bits = info[0]->Uint32Value();

  int rc = gpioWrite_Bits_0_31_Clear(bits);
  if (rc < 0) {
    return ThrowPigpiodError(rc, "GpioWriteBitsClear_0_31");
  }

  info.GetReturnValue().Set(rc);
}


NAN_METHOD(GpioWriteBitsClear_32_53) {
  if (info.Length() < 1 || !info[0]->IsUint32()) {
    return Nan::ThrowError(Nan::ErrnoException(EINVAL, "GpioWriteBitsClear_32_53", ""));
  }

  unsigned bits = info[0]->Uint32Value();

  int rc = gpioWrite_Bits_32_53_Clear(bits);
  if (rc < 0) {
    return ThrowPigpiodError(rc, "GpioWriteBitsClear_32_53");
  }

  info.GetReturnValue().Set(rc);
}

/* ------------------------------------------------------------------------ */
/* Notifier                                                                 */
/* ------------------------------------------------------------------------ */


NAN_METHOD(gpioNotifyOpen) {
  int rc = gpioNotifyOpen();
  if (rc < 0) {
    return ThrowPigpiodError(rc, "gpioNotifyOpen");
  }

  info.GetReturnValue().Set(rc);
}


NAN_METHOD(gpioNotifyOpenWithSize) {
  if (info.Length() < 1 || !info[0]->IsUint32()) {
    return Nan::ThrowError(Nan::ErrnoException(EINVAL, "gpioNotifyOpenWithSize", ""));
  }

  unsigned bufSize = info[0]->Uint32Value();

  int rc = gpioNotifyOpenWithSize(bufSize);
  if (rc < 0) {
    return ThrowPigpiodError(rc, "gpioNotifyOpen");
  }

  info.GetReturnValue().Set(rc);
}


NAN_METHOD(gpioNotifyBegin) {
  if (info.Length() < 2 || !info[0]->IsUint32() || !info[1]->IsUint32()) {
    return Nan::ThrowError(Nan::ErrnoException(EINVAL, "gpioNotifyBegin", ""));
  }

  unsigned handle = info[0]->Uint32Value();
  unsigned bits = info[1]->Uint32Value();

  int rc = gpioNotifyBegin(handle, bits);
  if (rc < 0) {
    return ThrowPigpiodError(rc, "gpioNotifyBegin");
  }
}


NAN_METHOD(gpioNotifyPause) {
  if (info.Length() < 1 || !info[0]->IsUint32()) {
    return Nan::ThrowError(Nan::ErrnoException(EINVAL, "gpioNotifyPause", ""));
  }

  unsigned handle = info[0]->Uint32Value();

  int rc = gpioNotifyPause(handle);
  if (rc < 0) {
    return ThrowPigpiodError(rc, "gpioNotifyPause");
  }
}


NAN_METHOD(gpioNotifyClose) {
  if (info.Length() < 1 || !info[0]->IsUint32()) {
    return Nan::ThrowError(Nan::ErrnoException(EINVAL, "gpioNotifyClose", ""));
  }

  unsigned handle = info[0]->Uint32Value();

  int rc = gpioNotifyClose(handle);
  if (rc < 0) {
    return ThrowPigpiodError(rc, "gpioNotifyClose");
  }
}


/* ------------------------------------------------------------------------ */
/* Configuration                                                            */
/* ------------------------------------------------------------------------ */


NAN_METHOD(gpioCfgClock) {
  if (info.Length() < 2 || !info[0]->IsUint32() || !info[1]->IsUint32()) {
    return Nan::ThrowError(Nan::ErrnoException(EINVAL, "gpioCfgClock", ""));
  }

  unsigned cfgMicros = info[0]->Uint32Value();
  unsigned cfgPeripheral = info[1]->Uint32Value();
  unsigned cfgSource = 0;

  gpioCfgClock(cfgMicros, cfgPeripheral, cfgSource);
}


/*static void SetConst(
  Nan::ADDON_REGISTER_FUNCTION_ARGS_TYPE target,
  const char* name,
  int value
) {
  Nan::Set(target,
    Nan::New<v8::String>(name).ToLocalChecked(),
    Nan::New<v8::Integer>(value)
  );
}*/


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
/*  SetConst(target, "PI_INPUT", PI_INPUT);
  SetConst(target, "PI_OUTPUT", PI_OUTPUT);
  SetConst(target, "PI_ALT0", PI_ALT0);
  SetConst(target, "PI_ALT1", PI_ALT1);
  SetConst(target, "PI_ALT2", PI_ALT2);
  SetConst(target, "PI_ALT3", PI_ALT3);
  SetConst(target, "PI_ALT4", PI_ALT4);
  SetConst(target, "PI_ALT5", PI_ALT5);*/

  /* pud constants */
/*  SetConst(target, "PI_PUD_OFF", PI_PUD_OFF);
  SetConst(target, "PI_PUD_DOWN", PI_PUD_DOWN);
  SetConst(target, "PI_PUD_UP", PI_PUD_UP);*/

  /* isr constants */
/*  SetConst(target, "RISING_EDGE", RISING_EDGE);
  SetConst(target, "FALLING_EDGE", FALLING_EDGE);
  SetConst(target, "EITHER_EDGE", EITHER_EDGE);*/

  /* timeout constant */
/*  SetConst(target, "PI_TIMEOUT", PI_TIMEOUT); */

  /* gpio number constants */
/*  SetConst(target, "PI_MIN_GPIO", PI_MIN_GPIO);
  SetConst(target, "PI_MAX_GPIO", PI_MAX_GPIO);
  SetConst(target, "PI_MAX_USER_GPIO", PI_MAX_USER_GPIO);*/

  /* error code constants */
/*  SetConst(target, "PI_INIT_FAILED", PI_INIT_FAILED);*/

  /* gpioCfgClock cfgPeripheral constants */
/*  SetConst(target, "PI_CLOCK_PWM", PI_CLOCK_PWM);
  SetConst(target, "PI_CLOCK_PCM", PI_CLOCK_PCM);*/

  /* functions */
  SetFunction(target, "pigpio_start", pigpio_start);
  SetFunction(target, "pigpio_stop", pigpio_stop);

  SetFunction(target, "set_mode", set_mode);
  SetFunction(target, "get_mode", get_mode);

  SetFunction(target, "set_pull_up_down", set_pull_up_down);

  SetFunction(target, "gpio_read", gpio_read);
  SetFunction(target, "gpio_write", gpio_write);
  SetFunction(target, "gpioTrigger", gpioTrigger);

  SetFunction(target, "spi_open", spi_open);
  SetFunction(target, "spi_xfer", spi_xfer);
  SetFunction(target, "spi_close", spi_close);

  SetFunction(target, "gpioPWM", gpioPWM);
  SetFunction(target, "gpioHardwarePWM", gpioHardwarePWM);
  SetFunction(target, "gpioGetPWMdutycycle", gpioGetPWMdutycycle);
  SetFunction(target, "gpioSetPWMrange", gpioSetPWMrange);
  SetFunction(target, "gpioGetPWMrange", gpioGetPWMrange);
  SetFunction(target, "gpioGetPWMrealRange", gpioGetPWMrealRange);
  SetFunction(target, "gpioSetPWMfrequency", gpioSetPWMfrequency);
  SetFunction(target, "gpioGetPWMfrequency", gpioGetPWMfrequency);

  SetFunction(target, "gpioServo", gpioServo);
  SetFunction(target, "gpioGetServoPulsewidth", gpioGetServoPulsewidth);

  SetFunction(target, "callback", callback);
  SetFunction(target, "set_watchdog", set_watchdog);
  SetFunction(target, "gpioSetAlertFunc", gpioSetAlertFunc);

  SetFunction(target, "GpioReadBits_0_31", GpioReadBits_0_31);
  SetFunction(target, "GpioReadBits_32_53", GpioReadBits_32_53);
  SetFunction(target, "GpioWriteBitsSet_0_31", GpioWriteBitsSet_0_31);
  SetFunction(target, "GpioWriteBitsSet_32_53", GpioWriteBitsSet_32_53);
  SetFunction(target, "GpioWriteBitsClear_0_31", GpioWriteBitsClear_0_31);
  SetFunction(target, "GpioWriteBitsClear_32_53", GpioWriteBitsClear_32_53);

  SetFunction(target, "gpioNotifyOpen", gpioNotifyOpen);
  SetFunction(target, "gpioNotifyOpenWithSize", gpioNotifyOpenWithSize);
  SetFunction(target, "gpioNotifyBegin", gpioNotifyBegin);
  SetFunction(target, "gpioNotifyPause", gpioNotifyPause);
  SetFunction(target, "gpioNotifyClose", gpioNotifyClose);

  SetFunction(target, "gpioCfgClock", gpioCfgClock);
}

NODE_MODULE(pigpio, InitAll)

