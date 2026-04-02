#include "soc/gpio_sig_map.h"

// — read master computer’s signals (middle level) —

// This example code is in the Public Domain (or CC0 licensed, at your option.)
// By Richard Li - 2020
//
// This example creates a bridge between Serial and Classical Bluetooth (SSP with authentication)
// and also demonstrate that SerialBT has the same functionalities as a normal Serial

template<typename T>
void printToAllPorts(T text, bool newLine = true) {
  String textResponse = String(text);
  if (newLine) {
    textResponse += "\r\n";
  }
  if (moduleActivatedQ[0]) { // serial2
    Serial2.print(textResponse);
  }
  PT(textResponse);
}
