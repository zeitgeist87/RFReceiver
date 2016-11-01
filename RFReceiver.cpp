#include "RFReceiver.h"

#include <util/crc16.h>

static inline uint16_t crc_update(uint16_t crc, uint8_t data) {
  return _crc_ccitt_update(crc, data);
}

#define checkBits(b1, b2, b3, pos) (((((b1) & (1 << pos)) != 0) + \
                                   (((b2) & (1 << pos)) != 0) + \
                                   (((b3) & (1 << pos)) != 0)) > 1)

static inline byte recoverByte(const byte b1, const byte b2, const byte b3) {
  byte res;

  if (b1 == b2 && b1 == b3)
    return b1;

  res = checkBits(b1, b2, b3, 0);
  res |= checkBits(b1, b2, b3, 1) << 1;
  res |= checkBits(b1, b2, b3, 2) << 2;
  res |= checkBits(b1, b2, b3, 3) << 3;
  res |= checkBits(b1, b2, b3, 4) << 4;
  res |= checkBits(b1, b2, b3, 5) << 5;
  res |= checkBits(b1, b2, b3, 6) << 6;
  res |= checkBits(b1, b2, b3, 7) << 7;

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

  unsigned long time = micros();
  unsigned int diff = time - lastTimestamp;
  lastTimestamp = time;

  if (diff <= (pulseLength << 1))
    return;

  byte missingBits = (((unsigned int)(time - lastSuccTimestamp)) + (pulseLength << 1)) / (pulseLength << 2);
  lastSuccTimestamp = time;

  if (packageStarted && bitCount + missingBits > 8) {
    shiftByte >>= 8 - bitCount;
    decodeByte(shiftByte);
    missingBits -= 8 - bitCount;

    while (missingBits > 8) {
      decodeByte(0);
      missingBits -= 8;
    }

    shiftByte = 0;
    bitCount = 0;
  }

  shiftByte >>= missingBits;
  if (!state)
     shiftByte |= 0x80;

  if (packageStarted) {
    bitCount += missingBits;
    if (bitCount != 8)
      return;
    bitCount = 0;

    decodeByte(shiftByte);
  } else if (shiftByte == 0xAA) {
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
