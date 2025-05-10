#include "mensagem_handler.h"


String gerarCodigo() {
  String caracteres = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789!@#$%^&*()_-+=<>?";
  String codigo = "";
  for (int i = 0; i < 8; i++) {
    int indice = random(caracteres.length());
    codigo += caracteres[indice];
  }
  
  return codigo;
}

Sequencia sequenciarMensagem(String texto) {
  Sequencia s;
  
  // Gerar codigo para mensagem
  s.id_mensagem = gerarCodigo();
  s.total_pacotes = (texto.length() + MAX_TEXTO_CHARS_IN_PACKET - 1) / MAX_TEXTO_CHARS_IN_PACKET;

  for (int i = 0; i < s.total_pacotes; i++) {
    Pacote p;
    p.id_mensagem = s.id_mensagem;
    String parte = texto.substring(i * MAX_TEXTO_CHARS_IN_PACKET, (i + 1) * MAX_TEXTO_CHARS_IN_PACKET);
    parte.toCharArray(p.texto, MAX_TEXTO_CHARS_IN_PACKET);
    p.num_pacote = i + 1;
    p.total_pacotes = s.total_pacotes;
    s.pacotes[i] = p;
  }

  return s;
}