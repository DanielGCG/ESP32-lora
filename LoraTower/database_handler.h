#ifndef DATABASE_HANDLER_H
#define DATABASE_HANDLER_H

#include "Arduino.h"
#include "LoRaWan_APP.h"

void enviarParaDatabase(String mensagem, String idRecebido);
void sendTime();
void configurarTempoNTP();

#endif