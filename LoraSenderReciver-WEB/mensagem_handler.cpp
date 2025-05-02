#include "mensagem_handler.h"
#include "Arduino.h"

Sequencia sequenciarMensagem(String texto) {
  Sequencia s;
  unsigned int id = (unsigned int) millis();
  s.id_mensagem = id;
  s.total_pacotes = (texto.length() + MAX_TEXTO_CHARS_IN_PACKET - 1) / MAX_TEXTO_CHARS_IN_PACKET;

  for (int i = 0; i < s.total_pacotes; i++) {
    Pacote p;
    p.id_mensagem = id;
    String parte = texto.substring(i * MAX_TEXTO_CHARS_IN_PACKET, (i + 1) * MAX_TEXTO_CHARS_IN_PACKET);
    parte.toCharArray(p.texto, MAX_TEXTO_CHARS_IN_PACKET);
    p.num_pacote = i + 1;
    p.total_pacotes = s.total_pacotes;
    s.pacotes[i] = p;
  }

  return s;
}
