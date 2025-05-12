#include "Arduino.h"
#include "lora_handler.h"
#include "wifi_handler.h"
#include "mensagem_handler.h"
#include "display_handler.h"
#include "database_handler.h"

void setup() {
  Serial.begin(115200);
  iniciarWiFi();
  setupLoRa();
  configurarTempoNTP();
}

void loop() {
  loopLoRa();
}
