RFReceiver
===========

![433 MHz modules](https://github.com/zeitgeist87/RFReceiver/raw/master/images/xy-mk-5v.jpg)

This is an Arduino library for low-cost 433 MHz receiver modules with a focus on
reliable one-way communication and forward error correction. It
uses the [PinChangeInterruptHanlder](https://github.com/zeitgeist87/PinChangeInterruptHandler) library.

The corresponding [RFTransmitter library](https://github.com/zeitgeist87/RFTransmitter)
for the 433 MHz transmitter modules can
be found [here](https://github.com/zeitgeist87/RFTransmitter).

Usage
-----

![433 MHz module connection](https://github.com/zeitgeist87/RFReceiver/raw/master/images/xy-mk-5v-connect.jpg)

```cpp
#include <PinChangeInterruptHandler.h>
#include <RFReceiver.h>

// Listen on digital pin 2
RFReceiver receiver(2);

void setup() {
  Serial.begin(9600);
  receiver.begin();
}

void loop() {
  char msg[MAX_PACKAGE_SIZE];
  byte senderId = 0;
  byte packageId = 0;
  byte len = receiver.recvPackage((byte *)msg, &senderId, &packageId);

  Serial.println("");
  Serial.print("Package: ");
  Serial.println(packageId);
  Serial.print("Sender: ");
  Serial.println(senderId);
  Serial.print("Message: ");
  Serial.println(msg);
}
```