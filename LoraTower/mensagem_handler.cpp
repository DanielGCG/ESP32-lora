#include "mensagem_handler.h"

String gerarCodigo() {
  String caracteres = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789";
  String codigo = "";
  for (int i = 0; i < 8; i++) {
    int indice = random(caracteres.length());
    codigo += caracteres[indice];
  }
  
  return codigo;
}

Sequencia sequenciarMensagem(String texto) {
  Sequencia s;
  s.id_mensagem = gerarCodigo();

  int textoLen = texto.length();
  int i = 0;
  int pacoteIndex = 0;

  while (i < textoLen) {
    Pacote p;
    p.id_mensagem = s.id_mensagem;

    String parte = "";
    int bytesContados = 0;

    while (i < textoLen) {
      char c = texto.charAt(i);
      int bytesDoChar = 1;

      if ((uint8_t)c >= 0xC0) {  // caractere multibyte
        if ((uint8_t)c >= 0xF0) bytesDoChar = 4;
        else if ((uint8_t)c >= 0xE0) bytesDoChar = 3;
        else bytesDoChar = 2;
      }

      // Evita cortar no meio de um caractere
      if (bytesContados + bytesDoChar > MAX_TEXTO_BYTES_IN_PACKET - 1) break;

      while (bytesDoChar-- && i < textoLen) {
        parte += texto.charAt(i++);
        bytesContados++;
      }
    }

    parte.toCharArray(p.texto, MAX_TEXTO_BYTES_IN_PACKET);

    p.num_pacote = pacoteIndex + 1;
    p.total_pacotes = 0;  // será preenchido depois
    s.pacotes[pacoteIndex++] = p;
  }

  s.total_pacotes = pacoteIndex;

  // Preenche total_pacotes corretamente em todos os pacotes
  for (int j = 0; j < s.total_pacotes; j++) {
    s.pacotes[j].total_pacotes = s.total_pacotes;
  }

  return s;
}