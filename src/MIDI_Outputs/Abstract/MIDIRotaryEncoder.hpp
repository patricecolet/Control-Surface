#pragma once

#include <AH/STL/utility> // std::forward
#include <Def/Def.hpp>
#include <MIDI_Outputs/Abstract/MIDIOutputElement.hpp>

BEGIN_CS_NAMESPACE

/**
 * @brief   An abstract class for rotary encoders that send MIDI events.
 */
template <class Enc, class Sender>
class GenericMIDIRotaryEncoder : public MIDIOutputElement {
  public:
    /**
     * @brief   Construct a new MIDIRotaryEncoder.
     *
     * @todo    Documentation
     */
    GenericMIDIRotaryEncoder(Enc &&encoder, MIDIAddress address,
                             int8_t speedMultiply, uint8_t pulsesPerStep,
                             const Sender &sender)
        : encoder(std::forward<Enc>(encoder)), address(address),
          speedMultiply(speedMultiply), pulsesPerStep(pulsesPerStep),
          sender(sender) {}

    void begin() override {}

    void update() override {
        int32_t encval = encoder.read();
        int32_t delta = (encval - deltaOffset) * speedMultiply / pulsesPerStep;
        if (delta) {
            sender.send(delta, address);
            deltaOffset += delta * pulsesPerStep / speedMultiply;
        }
    }

  private:
    Enc encoder;
    MIDIAddress address;
    int8_t speedMultiply;
    uint8_t pulsesPerStep;
    long deltaOffset = 0;

  public:
    Sender sender;
};

#if defined(Encoder_h_) || not defined(ARDUINO)

template <class Sender>
using MIDIRotaryEncoder = GenericMIDIRotaryEncoder<Encoder, Sender>;

template <class Sender>
using BorrowedMIDIRotaryEncoder = GenericMIDIRotaryEncoder<Encoder &, Sender>;

#endif

END_CS_NAMESPACE