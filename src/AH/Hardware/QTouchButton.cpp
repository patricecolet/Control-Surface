#include "QTouchButton.hpp"

AH_DIAGNOSTIC_WERROR() // Enable errors on warnings

BEGIN_AH_NAMESPACE

QTouchButton::QTouchButton(pin_t pin) : pin(pin) {}

void QTouchButton::begin() { 
    ExtIO::pinMode(pin, INPUT_PULLUP); 
    Adafruit_FreeTouch qt = Adafruit_FreeTouch(pin, OVERSAMPLE_4, RESISTOR_50K, FREQ_MODE_NONE);
    }

void QTouchButton::invert() { state.invert = true; }

QTouchButton::State QTouchButton::update() {
    // Read pin state and current time
    int touching = qt.measure();
    unsigned long now = millis();

    bool input = 1;
    if(touching >= threshold){
      input=0;
    }
    // Check if enough time has elapsed after last bounce
    if (state.bouncing)
        state.bouncing = now - state.prevBounceTime <= debounceTime;
    // Shift the debounced state one bit to the left, either appending the
    // new input state if not bouncing, or repeat the old state if bouncing
    bool prevState = state.debounced & 0b01;
    bool newState = state.bouncing ? prevState : input;
//    bool newState = 1;
    state.debounced = (prevState << 1) | newState;
//    state.debounced = 1;
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