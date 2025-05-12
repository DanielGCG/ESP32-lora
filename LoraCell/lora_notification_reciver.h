// lora_notification_reciver.h
#pragma once

#ifndef LORAWAN_DISPLAY_H
#define LORAWAN_DISPLAY_H

// Declarações externas de variáveis
extern char rxpacket[200];   // Defina o tamanho no .cpp
extern int16_t rxSize;
extern bool lora_idle;

// Declarações de funções
void setupLoRa();
void loopLoRa();
void OnRxDone(uint8_t *payload, uint16_t size, int16_t rssi, int8_t snr);
void OnTxDone(void);

#endif
