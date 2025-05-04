#include "Arduino.h"
#include "lora_handler.h"
#include "wifi_handler.h"
#include "mensagem_handler.h"
#include "display_handler.h"

void setup() {
  Serial.begin(115200);
  iniciarWiFi();
  configurarLoRa();
  configTzTime("America/Sao_Paulo", "pool.ntp.org");
}

void loop() {
  processarIrqLoRa();

  if (lora_idle && Serial.available()) {
    String texto = Serial.readStringUntil('\n');
    texto.trim();
    enviarMensagemSequenciadaLoRa(texto);
  }
}
