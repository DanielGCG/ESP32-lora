#ifndef MENSAGEM_HANDLER_H
#define MENSAGEM_HANDLER_H

#define MAX_PACKETS_PER_SEQUENCE 20
#define MAX_TEXTO_CHARS_IN_PACKET 150

#include <Arduino.h>

#define MAX_PACKETS_PER_SEQUENCE 20
#define MAX_TEXTO_CHARS_IN_PACKET 150

struct Pacote {
  int id_mensagem;
  int num_pacote;
  int total_pacotes;
  char texto[MAX_TEXTO_CHARS_IN_PACKET];
};

struct Sequencia {
  int id_mensagem;
  Pacote pacotes[MAX_PACKETS_PER_SEQUENCE];
  int total_pacotes;
};

Sequencia sequenciarMensagem(String texto);

#endif
