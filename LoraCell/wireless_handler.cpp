/*#include "BluetoothSerial.h"

BluetoothSerial SerialBT;

void setupBluetooth() {
  Serial.println("Iniciando Bluetooth...");

  SerialBT.begin("CelulaLora");  // Modo sem PIN

  Serial.println("Bluetooth iniciado. Emparelhe sem PIN.");
}

void loopBluetooth() {
  // Se recebeu algo via Bluetooth → envia para USB Serial
  if (SerialBT.available()) {
    char c = SerialBT.read();
    Serial.write(c);
  }

  // Se recebeu algo via USB Serial → envia para Bluetooth (se conectado)
  if (Serial.available()) {
    char c = Serial.read();
    if (SerialBT.hasClient()) {
      SerialBT.write(c);
    }
  }
}*/
