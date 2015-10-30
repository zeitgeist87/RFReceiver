#ifndef RECEIVER_H
#define RECEIVER_H

#if defined(ARDUINO) && ARDUINO >= 100
  #include "Arduino.h"
#elif defined(ENERGIA)
  #include "Energia.h"
#else
  #include "WProgram.h"
#endif

#include <PinChangeInterruptHandler.h>

const byte MAX_PAYLOAD_SIZE = 80;
const byte MIN_PACKAGE_SIZE = 4;
const byte MAX_PACKAGE_SIZE = MAX_PAYLOAD_SIZE + MIN_PACKAGE_SIZE;
const byte MAX_SENDER_ID = 31;

class RFReceiver : PinChangeInterruptHandler {
    const byte inputPin;
    const unsigned int pulseLength;

    // Input buffer and input state
    byte shiftByte;
    byte errorCorBuf[3];
    byte bitCount, byteCount, errorCorBufCount;
    unsigned long lastTimestamp, lastSuccTimestamp;
    bool packageStarted;

    byte inputBuf[MAX_PACKAGE_SIZE];
    byte inputBufLen;
    volatile bool inputBufReady;

    // Used to filter out duplicate packages
    byte prevPackageIds[MAX_SENDER_ID + 1];

    byte recvDataRaw(byte * data);
    byte recvData(byte * data);

  public:
    RFReceiver(byte inputPin, unsigned int pulseLength = 100) : inputPin(inputPin),
        pulseLength(pulseLength), inputBufLen(0), inputBufReady(false), bitCount(0),
        byteCount(0), errorCorBufCount(0), lastTimestamp(0), packageStarted(false),
        shiftByte(0), lastSuccTimestamp(0) {

    }
    void begin() {
      pinMode(inputPin, INPUT);
      attachPCInterrupt(digitalPinToPCINT(inputPin));
    }

    void stop() {
      detachPCInterrupt(digitalPinToPCINT(inputPin));
    }

    byte recvPackage(byte * data, byte *pSenderId = 0, byte *pPackageId = 0);

    void decodeByte(byte inputByte);
    virtual void handlePCInterrupt(int8_t pcIntNum, bool value);
};

#endif  /* RECEIVER_H */
