/* ✔ */

#pragma once

#include <AH/Settings/Warnings.hpp>
AH_DIAGNOSTIC_WERROR() // Enable errors on warnings

#include <AH/Hardware/ExtendedInputOutput/ExtendedInputOutput.hpp>
#include <AH/Settings/SettingsWrapper.hpp>

#include <AH/Adafruit_Freetouch/adafruit_ptc.h>

BEGIN_AH_NAMESPACE

/**
 * @brief   A class for reading and debouncing buttons and switches.
 * 
 * @ingroup AH_HardwareUtils
 */
class QTouchButton {
  public:
    /**
     * @brief   Construct a new Button object.
     * 
     * **This constructor should not be used.**  
     * It is just a way to easily create arrays of buttons, and initializing
     * them later.
     */
    QTouchButton() : pin(NO_PIN) {}

    /**
     * @brief   Construct a new Button object.
     *
     * @param   pin
     *          The digital pin to read from. The internal pull-up resistor
     *          will be enabled when `begin` is called.
     */
    QTouchButton(pin_t pin);

    /// @brief   Initialize (enable the internal pull-up resistor).
    bool begin();

// Adafruit Freetouch
    uint16_t measure(void);
    uint16_t measureRaw(void);

    /**
     * @brief   Invert the input state of this button
     *          (button pressed is `HIGH` instead of `LOW`).
     */
    void invert();

    /// @brief   An enumeration of the different states a button can be in.
    enum State {
        Pressed = 0b00,  ///< Input went from low to low   (0,0)
        Released = 0b11, ///< Input went from high to high (1,1)
        Falling = 0b10,  ///< Input went from high to low  (1,0)
        Rising = 0b01    ///< Input went from low to high  (0,1)
    };

    /**
     * @brief   Read the button and return its new state.
     *
     * The button is debounced, the debounce time can be set in
     * Settings.hpp: #BUTTON_DEBOUNCE_TIME.
     *
     * ```
     * Debounce time: ├────┤
     *
     * Raw input:
     *    HIGH  ──────┐      ┌──────┐ ┌─┐      ┌─┐ ┌──────┐ ┌────────
     *    LOW         └──────┘      └─┘ └──────┘ └─┘      └─┘
     *                ├────┤ ├────┤ ├─┼─┼────┤ ├─┼─┼────┤ ├─┼────┤
     *
     * Debounced output:
     *    HIGH  ──────┐      ┌──────┐          ┌──────────┐      ┌───
     *    LOW         └──────┘      └──────────┘          └──────┘
     *
     * States:
     *    HIGH  ────────────────┐                  ┌─────────────────
     *    LOW                   └──────────────────┘
     *            Released   Falling   Pressed   Rising
     * ```
     *
     * @return  The state of the button, either Button::Pressed,
     *          Button::Released, Button::Falling or Button::Rising.
     */
    State update();

    /**
     * @brief   Get the state of the button, without updating it.
     *          Returns the same value as the last call to @ref update.
     *
     * @return  The state of the button, either Button::Pressed,
     *          Button::Released, Button::Falling or Button::Rising.
     */
    State getState() const;

    /// @brief   Return the name of the state as a string.
    static FlashString_t getName(State state);

    /// Return the time point (in milliseconds) when the button last bounced.
    unsigned long previousBounceTime() const;

    /// Return the time (in milliseconds) that the button has been stable for,
    /// compared to the given time point.
    unsigned long stableTime(unsigned long now) const;

    /// Return the time (in milliseconds) that the button has been stable for.
    unsigned long stableTime() const;

    /**
     * @brief   Set the debounce time for all Button%s.
     * 
     * @note    This function affects **all** Button objects.
     * 
     * @param   debounceTime
     *          The new debounce time in milliseconds.
     */
    static void
    setDebounceTime(unsigned long debounceTime = BUTTON_DEBOUNCE_TIME);

    /**
     * @brief   Get the debounce time.
     * @return  The debounce time in milliseconds.
     * @see     setDebounceTime
     */
    static unsigned long getDebounceTime();

  private:
    pin_t pin;

    struct InternalState {
        InternalState()
            : debounced(0b11), bouncing(true), prevInput(HIGH), invert(false),
              prevBounceTime(0) {}
        uint8_t debounced : 2;
        bool bouncing : 1;
        bool prevInput : 1;
        bool invert : 1;
        unsigned long prevBounceTime;
    } state;

    // Adafruit Freetouch
  struct adafruit_ptc_config config;

  void ptcInitSettings(void);
  void ptcConfigIOpin(void);
  uint16_t startPtcAcquire(void);

  int getYLine(void);
  void selectYLine(void);
  void setOversampling(oversample_t lvl);
  void setSeriesResistor(series_resistor_t res);
  void setFreqHopping(freq_mode_t fh, freq_hop_t hops = FREQ_HOP_1);
  void setCompCap(uint16_t cc);
  void setIntCap(uint8_t ic);

  void snapshotRegsAndPrint(uint32_t base, uint8_t numregs);
  void printHex(uint8_t h, boolean newline);
  void printPTCregs(uint32_t base, uint8_t *regs, uint8_t num);
// Adafruit freetouch end

    /// Edit this in Settings.hpp
    /// @see    QTOUCH_BUTTON_DEBOUNCE_TIME
    /// @see    QTOUCH_BUTTON_THRESHOLD
    static unsigned long debounceTime;
    static unsigned long threshold;
};

END_AH_NAMESPACE

AH_DIAGNOSTIC_POP()
