#include "QTouchButton.hpp"

AH_DIAGNOSTIC_WERROR() // Enable errors on warnings

BEGIN_AH_NAMESPACE

QTouchButton::QTouchButton(pin_t pin) : pin(pin) {
    adafruit_ptc_get_config_default(&config);
    int8_t port_offset = 0;
    if (g_APinDescription[pin].ulPort == PORTB) {
    port_offset += 32;
  }    
  
    config.pin = port_offset + g_APinDescription[pin].ulPin;
    config.yline = getYLine(); // determine the Y0-15 #
    config.oversample = OVERSAMPLE_4;
    config.seriesres = RESISTOR_50K;
    config.freqhop = FREQ_MODE_NONE;
}

bool QTouchButton::begin() {   
    if (config.yline == -1) { // not all pins have Y line
    return false;
    }
 
    
  /* Setup and enable generic clock source for PTC module.
     struct system_gclk_chan_config gclk_chan_conf;
     system_gclk_chan_get_config_defaults(&gclk_chan_conf);
  */

  uint8_t channel = PTC_GCLK_ID;
  uint8_t source_generator = 3;

    // original line: system_gclk_chan_set_config(PTC_GCLK_ID, &gclk_chan_conf);
  uint32_t new_clkctrl_config = (channel << GCLK_CLKCTRL_ID_Pos); // from gclk.c

  // original line: gclk_chan_conf.source_generator = GCLK_GENERATOR_1;
  /* Select the desired generic clock generator */
  new_clkctrl_config |= source_generator << GCLK_CLKCTRL_GEN_Pos; // from gclk.c

  /* Disable generic clock channel */
  // original line: system_gclk_chan_disable(channel);
  noInterrupts();

  /* Select the requested generator channel */
  *((uint8_t *)&GCLK->CLKCTRL.reg) = channel;

  /* Sanity check WRTLOCK */
  // Assert(!GCLK->CLKCTRL.bit.WRTLOCK);

  /* Switch to known-working source so that the channel can be disabled */
  uint32_t prev_gen_id = GCLK->CLKCTRL.bit.GEN;
  GCLK->CLKCTRL.bit.GEN = 0;

  /* Disable the generic clock */
  GCLK->CLKCTRL.reg &= ~GCLK_CLKCTRL_CLKEN;
  while (GCLK->CLKCTRL.reg & GCLK_CLKCTRL_CLKEN) {
    /* Wait for clock to become disabled */
  }

  /* Restore previous configured clock generator */
  GCLK->CLKCTRL.bit.GEN = prev_gen_id;

  // system_interrupt_leave_critical_section();
  interrupts();

  /* Write the new configuration */
  GCLK->CLKCTRL.reg = new_clkctrl_config;

  // original line: system_gclk_chan_enable(PTC_GCLK_ID);
  *((uint8_t *)&GCLK->CLKCTRL.reg) = channel;
  GCLK->CLKCTRL.reg |= GCLK_CLKCTRL_CLKEN; /* Enable the generic clock */

  // original line: system_apb_clock_set_mask(SYSTEM_CLOCK_APB_APBC,
  // PM_APBCMASK_PTC);
  PM->APBCMASK.reg |= PM_APBCMASK_PTC;

  adafruit_ptc_init(PTC, &config);

  return true;

}


uint16_t QTouchButton::measure(void) {
  uint16_t m;

  m = measureRaw();
  if (m == -1)
    return -1;

  // normalize the signal
  switch (config.oversample) {
  case OVERSAMPLE_1:
    return m;
  case OVERSAMPLE_2:
    return m / 2;
  case OVERSAMPLE_4:
    return m / 4;
  case OVERSAMPLE_8:
    return m / 8;
  case OVERSAMPLE_16:
    return m / 16;
  case OVERSAMPLE_32:
    return m / 32;
  case OVERSAMPLE_64:
    return m / 64;
  }

  return -1; // shouldn't reach here but fail if we do!
}

uint16_t QTouchButton::measureRaw(void) {
  adafruit_ptc_start_conversion(PTC, &config);

  while (!adafruit_ptc_is_conversion_finished(PTC)) {
    yield();
  }

  return adafruit_ptc_get_conversion_result(PTC);
}

/*********************************** low level config **/

int QTouchButton::getYLine(void) {
  int p = g_APinDescription[pin].ulPin;
  if (g_APinDescription[pin].ulPort == PORTA) {
    if ((p >= 2) && (p <= 7)) {
      return (p - 2);
    }
  }
  if (g_APinDescription[pin].ulPort == PORTB) {
    if ((p >= 0) && (p <= 9)) {
      return (p + 6);
    }
  }

  // not valid
  return -1;
}

void QTouchButton::setCompCap(uint16_t cc) {
  config.compcap = cc & 0x3FFF;
}

void QTouchButton::setIntCap(uint8_t ic) { config.intcap = ic & 0x3F; }

void QTouchButton::setOversampling(oversample_t lvl) {
  config.oversample = lvl; // back it up for later
}

void QTouchButton::setSeriesResistor(series_resistor_t res) {
  config.seriesres = res;
}

void QTouchButton::setFreqHopping(freq_mode_t fh, freq_hop_t hs) {
  config.freqhop = fh;
  config.hops = hs;
}

/**************************** DEBUGGING ASSIST *************************/
void QTouchButton::snapshotRegsAndPrint(uint32_t base, uint8_t numregs) {
  volatile uint32_t addr = base;
  uint8_t datas[255];

  for (uint8_t i = 0; i < numregs; i++) {
    datas[i] = *(uint8_t *)(addr + i);
  }
  printPTCregs(base, datas, numregs);

  for (uint8_t i = 0; i < numregs; i++) {
    //  Serial.print("$"); Serial.print(addr+i, HEX); Serial.print("\t0x");
    //  printHex(datas[i]); Serial.println();
  }
}


void QTouchButton::invert() { state.invert = true; }

QTouchButton::State QTouchButton::update() {
    // Read pin state and current time
    int touching = QTouchButton::measure();
    unsigned long now = millis();

    bool input = 0;
    if(touching >= threshold){
      input=1;
    }
    // Check if enough time has elapsed after last bounce
    if (state.bouncing)
        state.bouncing = now - state.prevBounceTime <= debounceTime;
    // Shift the debounced state one bit to the left, either appending the
    // new input state if not bouncing, or repeat the old state if bouncing
    bool prevState = state.debounced & 0b01;
    bool newState = state.bouncing ? prevState : input;
    state.debounced = (prevState << 1) | newState;
    // Check if the input changed state (button pressed, released or bouncing)
    if (input != state.prevInput) {
        state.bouncing = true;
        state.prevInput = input;
        state.prevBounceTime = now;
    }
    return getState();
}

QTouchButton::State QTouchButton::getState() const {
    return static_cast<State>(state.debounced);
}

FlashString_t QTouchButton::getName(QTouchButton::State state) {
    switch (state) {
        case QTouchButton::Pressed: return F("Pressed");
        case QTouchButton::Released: return F("Released");
        case QTouchButton::Falling: return F("Falling");
        case QTouchButton::Rising: return F("Rising");
        default: return F("<invalid>"); // Keeps the compiler happy
    }
}

unsigned long QTouchButton::previousBounceTime() const {
    return state.prevBounceTime;
}

unsigned long QTouchButton::stableTime(unsigned long now) const {
    return now - previousBounceTime();
}

unsigned long QTouchButton::stableTime() const { return stableTime(millis()); }

void QTouchButton::setDebounceTime(unsigned long debounceTime) {
    QTouchButton::debounceTime = debounceTime;
}

unsigned long QTouchButton::getDebounceTime() { return QTouchButton::debounceTime; }

unsigned long QTouchButton::debounceTime = QTOUCH_BUTTON_DEBOUNCE_TIME;
unsigned long QTouchButton::threshold = QTOUCH_BUTTON_THRESHOLD;

END_AH_NAMESPACE

AH_DIAGNOSTIC_POP()