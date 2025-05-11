#ifndef MENSAGEM_HANDLER_H
#define MENSAGEM_HANDLER_H

#define MAX_PACKETS_PER_SEQUENCE 20
#define MAX_TEXTO_BYTES_IN_PACKET 180

#include <Arduino.h>

struct Pacote {
  String id_mensagem;
  int num_pacote;
  int total_pacotes;
  char texto[MAX_TEXTO_BYTES_IN_PACKET];
};

struct Sequencia {
  String id_mensagem;
  Pacote pacotes[MAX_PACKETS_PER_SEQUENCE];
  int total_pacotes;
};

Sequencia sequenciarMensagem(String texto);

#endif