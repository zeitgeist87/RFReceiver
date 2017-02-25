#include "RFReceiver.h"

#if defined(__AVR__)
#include <util/crc16.h>
#endif

static inline uint16_t crc_update(uint16_t crc, uint8_t data) {
  #if defined(__AVR__)
    return _crc_ccitt_update(crc, data);
  #else
    // Source: http://www.atmel.com/webdoc/AVRLibcReferenceManual/group__util__crc_1ga1c1d3ad875310cbc58000e24d981ad20.html
    data ^= crc & 0xFF;
    data ^= data << 4;

    return ((((uint16_t)data << 8) | (crc >> 8)) ^ (uint8_t)(data >> 4)
            ^ ((uint16_t)data << 3));
  #endif
}

static inline byte recoverByte(const byte b1, const byte b2, const byte b3) {
  // Discard all bits that occur only once in the three input bytes
  // Use all bits that are in b1 and b2
  byte res = b1 & b2;
  // Use all bits that are in b1 and b3
  res |= b1 & b3;
  // Use all bits that are in b2 and b3
  res |= b2 & b3;
  return res;
}

void RFReceiver::decodeByte(byte inputByte) {
  if (!packageStarted)
    return;

  errorCorBuf[errorCorBufCount++] = inputByte;

  if (errorCorBufCount != 3)
    return;
  errorCorBufCount = 0;

  if (!byteCount) {
    // Quickly decide if this is really a package or not
    if (errorCorBuf[0] < MIN_PACKAGE_SIZE || errorCorBuf[0] > MAX_PACKAGE_SIZE ||
            errorCorBuf[0] != errorCorBuf[1] || errorCorBuf[0] != errorCorBuf[2]) {
      packageStarted = false;
      return;
    }

    inputBufLen = errorCorBuf[0];
    checksum = crc_update(checksum, inputBufLen);
  } else {
    byte data = recoverByte(errorCorBuf[0], errorCorBuf[1], errorCorBuf[2]);
    inputBuf[byteCount - 1] = data;
    // Calculate the checksum on the fly
    checksum = crc_update(checksum, data);

    if (byteCount == inputBufLen) {
      byte senderId = inputBuf[inputBufLen - 4];
      byte packageId = inputBuf[inputBufLen - 3];

      // Ignore duplicate packages and check if the checksum is correct
      if (!checksum && senderId <= MAX_SENDER_ID && prevPackageIds[senderId] != packageId) {
        prevPackageIds[senderId] = packageId;
        inputBufReady = true;
      }

      packageStarted = false;
      return;
    }
  }

  ++byteCount;
}

void RFReceiver::handlePCInterrupt(int8_t pcIntNum, bool state) {
  if (inputBufReady)
    return;

  ++changeCount;

  {
    unsigned long time = micros();
    if (time - lastTimestamp < pulseLimit)
      return;

    lastTimestamp = time;
  }

  shiftByte = (shiftByte >> 2) | ((changeCount - 1) << 6);
  changeCount = 0;

  if (packageStarted) {
    bitCount += 2;
    if (bitCount != 8)
      return;
    bitCount = 0;

    decodeByte(shiftByte);
  } else if (shiftByte == 0xE0) {
    // New package starts here
    bitCount = 0;
    byteCount = 0;
    errorCorBufCount = 0;
    inputBufLen = 0;
    checksum = 0xffff;
    packageStarted = true;
  }
}

byte RFReceiver::recvDataRaw(byte * data) {
  while (!inputBufReady);

  byte len = inputBufLen;
  memcpy(data, inputBuf, len - 2);

  // Enable the input as fast as possible
  inputBufReady = false;
  // The last two bytes contain the checksum, which is no longer needed
  return len - 2;
}

byte RFReceiver::recvPackage(byte *data, byte *pSenderId, byte *pPackageId) {
  for (;;) {
    byte len = recvDataRaw(data);
    byte senderId = data[len - 2];
    byte packageId = data[len - 1];

    if (pSenderId)
      *pSenderId = senderId;

    if (pPackageId)
      *pPackageId = packageId;

    // The last two bytes contain the sender and package ids
    return len - 2;
  }
}
