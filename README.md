RFReceiver
===========

This is an Arduino library for the low-cost 433 MHz receiver modules. It
uses the [PinChangeInterruptHanlder](https://github.com/zeitgeist87/PinChangeInterruptHandler) library.

Usage
-----

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