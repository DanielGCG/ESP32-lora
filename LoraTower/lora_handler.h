#ifndef LORA_HANDLER_H
#define LORA_HANDLER_H

#include "Arduino.h"
#include "LoRaWan_APP.h"

extern bool lora_idle;

void processarIrqLoRa();
void configurarLoRa();
void loopLoRa();
void enviarMensagemSequenciadaLoRa(String texto);
void enviarMensagemLoRa(String texto);

#endif
