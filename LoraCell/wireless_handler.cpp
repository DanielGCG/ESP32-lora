#include "BluetoothSerial.h"

BluetoothSerial SerialBT;

void setup_bluetooth() {
  SerialBT.begin("ESP32_BT"); // Nome do dispositivo Bluetooth
  Serial.println("Bluetooth iniciado. Pronto para emparelhar.");
}

void loop() {
  if (SerialBT.available()) {
    char c = SerialBT.read();
    Serial.print("Recebido via BT: ");
    Serial.println(c);
  }
}
