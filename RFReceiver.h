#ifndef RECEIVER_H
#define RECEIVER_H

#if ARDUINO >= 100
#include "Arduino.h"
#else
#include "WProgram.h"
#include "pins_arduino.h"
#endif

#include <PinChangeInterruptHandler.h>

enum {
  MAX_PAYLOAD_SIZE = 80,
  MIN_PACKAGE_SIZE = 4,
  MAX_PACKAGE_SIZE = MAX_PAYLOAD_SIZE + MIN_PACKAGE_SIZE,
  MAX_SENDER_ID = 31
};

class RFReceiver : PinChangeInterruptHandler {
    const byte inputPin;
    const unsigned int pulseLimit;

    // Input buffer and input state
    byte shiftByte;
    byte errorCorBuf[3];
    byte bitCount, byteCount, errorCorBufCount;
    unsigned long lastTimestamp;
    bool packageStarted;

    byte inputBuf[MAX_PACKAGE_SIZE];
    byte inputBufLen;
    uint16_t checksum;
    volatile bool inputBufReady;
    byte changeCount;

    // Used to filter out duplicate packages
    byte prevPackageIds[MAX_SENDER_ID + 1];

    byte recvDataRaw(byte * data);

  public:
    RFReceiver(byte inputPin, unsigned int pulseLength = 100) : inputPin(inputPin),
        pulseLimit((pulseLength << 2) - (pulseLength >> 1)), shiftByte(0),
        bitCount(0), byteCount(0), errorCorBufCount(0), lastTimestamp(0),
        packageStarted(false), inputBufLen(0), checksum(0),
        inputBufReady(false), changeCount(0) {

    }
    void begin() {
      pinMode(inputPin, INPUT);
      attachPCInterrupt(digitalPinToPCINT(inputPin));
    }

    void stop() {
      detachPCInterrupt(digitalPinToPCINT(inputPin));
    }

    /**
     * Returns true if a valid and deduplicated package is in the buffer, so
     * that a subsequent call to recvPackage() will not block.
     *
     * @returns True if recvPackage() will not block
     */
    bool ready() const {
      return inputBufReady;
    }

    byte recvPackage(byte * data, byte *pSenderId = 0, byte *pPackageId = 0);

    void decodeByte(byte inputByte);
    virtual void handlePCInterrupt(int8_t pcIntNum, bool value);
};

#endif  /* RECEIVER_H */
