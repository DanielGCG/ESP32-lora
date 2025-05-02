#include "Arduino.h"
#include "lora_handler.h"
#include "wifi_handler.h"
#include "mensagem_handler.h"

void setup() {
  Serial.begin(115200);
  iniciarWiFi();
  configurarLoRa();
}

void loop() {
  processarIrqLoRa();

  if (lora_idle && Serial.available()) {
    String texto = Serial.readStringUntil('\n');
    texto.trim();
    enviarMensagemLoRa(texto);
  }
}
