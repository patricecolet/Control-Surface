#pragma once

#include <AH/Hardware/QTouchButton.hpp>
#include <Def/Def.hpp>
#include <MIDI_Outputs/Abstract/MIDIOutputElement.hpp>

BEGIN_CS_NAMESPACE

/**
 * @brief   An abstract class for momentary push buttons that send MIDI events.
 *
 * The button is debounced.
 *
 * @see     Button
 */
template <class Sender>
class MIDIQTouchButton : public MIDIOutputElement {
  public:
    /**
     * @brief   Construct a new MIDIButton.
     *
     * @param   pin
     *          The qtouch input pin with the button connected.
     * @param   address
     *          The MIDI address to send to.
     * @param   sender
     *          The MIDI sender to use.
     */
    MIDIQTouchButton(pin_t pin, MIDIAddress address, const Sender &sender)
        : QTouchButton(pin), address(address), sender(sender) {}

    void begin() override { QTouchButton.begin(); }
    void update() override {
        AH::QTouchButton::State state = QTouchButton.update();
        if (state == AH::QTouchButton::Falling) {
            sender.sendOn(address);
        } else if (state == AH::QTouchButton::Rising) {
            sender.sendOff(address);
        }
    }

    /// @see @ref AH::Button::invert()
    void invert() { QTouchButton.invert(); }

    AH::QTouchButton::State getButtonState() const { return QTouchButton.getState(); }

    /// Get the MIDI address.
    MIDIAddress getAddress() const { return this->address; }
    /// Set the MIDI address. Has unexpected consequences if used while the 
    /// push button is pressed. Use banks if you need to support that.
    void setAddressUnsafe(MIDIAddress address) { this->address = address; }

  private:
    AH::QTouchButton QTouchButton;
    const MIDIAddress address;

  public:
    Sender sender;
};

END_CS_NAMESPACE