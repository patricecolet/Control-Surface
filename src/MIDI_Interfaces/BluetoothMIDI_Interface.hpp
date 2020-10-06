#pragma once

#include "BLEMIDI/BLEMIDI.hpp"
#include "BLEMIDI/BLEMIDIPacketBuilder.hpp"
#include "SerialMIDI_Interface.hpp"

#include <AH/Error/Error.hpp>

BEGIN_CS_NAMESPACE

/**
 * @brief   Bluetooth Low Energy MIDI Interface for the ESP32.
 * 
 * @ingroup MIDIInterfaces
 */
class BluetoothMIDI_Interface : public Parsing_MIDI_Interface,
                                public BLEServerCallbacks,
                                public BLECharacteristicCallbacks {

  private:
    // BLE Callbacks
    void onConnect(BLEServer *pServer) override {
        (void)pServer;
        DEBUGFN("Connected");
        updateMTU();
    };
    void onDisconnect(BLEServer *pServer) override {
        (void)pServer;
        DEBUGFN("Disonnected");
        updateMTU();
    }

    void onRead(BLECharacteristic *pCharacteristic) override {
        DEBUGFN("Read");
        // respond with no payload
        pCharacteristic->setValue(nullptr, 0);
    }
    void onWrite(BLECharacteristic *pCharacteristic) override {
        std::string value = pCharacteristic->getValue();
        const uint8_t *data =
            reinterpret_cast<const uint8_t *>(value.data());
        size_t len = value.size();
        DEBUGFN("Write [" << len << "] " << AH::HexDump(data, len));
        parse(data, len);
    }

  private:
    /// Number of microseconds to wait before sending the buffered MIDI out data.
    constexpr static unsigned long MAX_MESSAGE_TIME = 10000; // microseconds

    unsigned long startTime = 0;
    
    BLEMIDIPacketBuilder packetbuilder;
    uint16_t min_mtu = 23;

    SerialMIDI_Parser parser;
    BLEMIDI bleMidi;

  private:
    void resetStartTime() {
        if (packetbuilder.getSize() == 0)
            startTime = millis();
    }

    /// Find the smallest MTU of all clients. Used to compute the MIDI BLE 
    /// packet size.
    void updateMTU() {
        min_mtu = bleMidi.get_min_mtu();
        DEBUGFN(NAMEDVALUE(min_mtu));
    }

  public:
    BluetoothMIDI_Interface() : Parsing_MIDI_Interface(parser) {}

    void begin() override { bleMidi.begin(this, this); }

    void publish() {
        if (packetbuilder.getSize() == 0)
            return;
        bleMidi.notifyValue(packetbuilder.getBuffer(), packetbuilder.getSize());
        packetbuilder.reset();
        packetbuilder.setCapacity(min_mtu - 3);
    }

    MIDIReadEvent read() override {
        update();                         // TODO
        return MIDIReadEvent::NO_MESSAGE; // TODO
    }

    void update() override {
        if (micros() - startTime >= MAX_MESSAGE_TIME)
            publish();
    }

    void sendImpl(uint8_t header, uint8_t d1, uint8_t d2, uint8_t cn) override {
        (void)cn;
        uint16_t timestamp = millis();
        resetStartTime();
        if (packetbuilder.add3B(header, d1, d2, timestamp))
            return;
        publish();
        resetStartTime();
        packetbuilder.add3B(header, d1, d2, timestamp);
    }
    void sendImpl(uint8_t header, uint8_t d1, uint8_t cn) override {
        (void)cn;
        uint16_t timestamp = millis();
        resetStartTime();
        if (packetbuilder.add2B(header, d1, timestamp))
            return;
        publish();
        resetStartTime();
        packetbuilder.add2B(header, d1, timestamp);
    }

    void sendImpl(const uint8_t *data, size_t length, uint8_t cn) override {
        (void)cn;
        if (length < 2)
            return;

        // Length of SysEx data without SysEx start/end
        length -= 2;
        // Data without SysEx start
        data += 1;

        resetStartTime();

        // BLE MIDI timestamp
        uint16_t timestamp = millis();

        if (!packetbuilder.addSysEx(data, length, timestamp)) {
            publish();
            resetStartTime();
            packetbuilder.addSysEx(data, length, timestamp);
        }
        while (data) {
            publish();
            resetStartTime();
            packetbuilder.continueSysEx(data, length, timestamp);
        }
    }

    void sendImpl(uint8_t rt, uint8_t cn) override {
        (void)cn;
        uint16_t timestamp = millis();
        resetStartTime();
        if (packetbuilder.addRealTime(rt, timestamp))
            return;
        publish();
        resetStartTime();
        packetbuilder.addRealTime(rt, timestamp);
    }

    void parse(const uint8_t *const data, const size_t len) {
        // TODO: documentation and link to BLE MIDI spec
        if (len <= 1)
            return;
        if (MIDI_Parser::isData(data[0]))
            return;
        if (MIDI_Parser::isData(data[1]))
            parse(data[1]);
        bool prevWasTimestamp = true;
        for (const uint8_t *d = data + 2; d < data + len; d++) {
            if (MIDI_Parser::isData(*d)) {
                parse(*d);
                prevWasTimestamp = false;
            } else {
                if (prevWasTimestamp)
                    parse(*d);
                prevWasTimestamp = !prevWasTimestamp;
            }
        }
    }

    void parse(uint8_t data) {
        event = parser.parse(data);
        // Best we can do is just retry until the pipe is no longer in exclusive
        // mode.
        // Keep in mind that this is executed in the callback of the BLE stack,
        // I don't know what happens to the Bluetooth connection if we let it
        // wait for longer than the communication interval.
        //
        // TODO: If this causes problems, we could buffer the data until the
        //       pipe is available for writing again.
        while (!dispatchMIDIEvent(event))
#ifdef ARDUINO
            delay(1);
#else
            usleep(1e3);
#endif
    }

    BLEMIDI &getBLEMIDI() { return bleMidi; }
};

END_CS_NAMESPACE