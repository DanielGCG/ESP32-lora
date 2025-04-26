#include <SPI.h>
#include <LoRa.h>

#define BAND 915E6

void setup() {
  Serial.begin(115200);
  while (!Serial);

  Serial.println("LoRa Sender/Receiver Ready");

  LoRa.setPins(8, 12, 14);

  if (!LoRa.begin(BAND)) {
    Serial.println("Starting LoRa failed!");
    while (1);
  }
}

void loop() {
  // Enviar dados digitados
  if (Serial.available()) {
    String outgoing = Serial.readStringUntil('\n');
    outgoing.trim();
    if (outgoing.length() > 0) {
      LoRa.beginPacket();
      LoRa.print(outgoing);
      LoRa.endPacket();
      Serial.print("Sent: ");
      Serial.println(outgoing);
    }
  }

  // Receber dados via LoRa
  int packetSize = LoRa.parsePacket();
  if (packetSize) {
    String incoming = "";
    while (LoRa.available()) {
      incoming += (char)LoRa.read();
    }
    Serial.print("Received: ");
    Serial.println(incoming);
  }
}
