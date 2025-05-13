#ifndef LORA_HANDLER_H
#define LORA_HANDLER_H

#include "Arduino.h"
#include "LoRaWan_APP.h"

extern bool lora_idle;

void setupLoRa();
void loopLoRa();
void OnRxDone(uint8_t *payload, uint16_t size, int16_t rssi, int8_t snr);
void OnTxDone(void);
void enviarMensagemLoRa(String texto);
void enviarMensagemSequenciadaLoRa(String texto);

#endif
