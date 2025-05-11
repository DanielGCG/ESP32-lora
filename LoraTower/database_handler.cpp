#include "database_handler.h"
#include <WiFi.h>
#include <HTTPClient.h>
#include <time.h>
#include <Arduino.h>


void enviarParaDatabase(String mensagem, String idRecebido) {
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;

    // Escapa as aspas na mensagem
    String mensagemEscapada = mensagem;
    mensagemEscapada.replace("\"", "\\\"");

    // Inclui o ID da mensagem no JSON
    String json = "{\"mensagem\":\"" + mensagemEscapada + "\", \"mensagemID\":\"" + idRecebido + "\"}";

    http.begin("https://www.botecors.me/API/lora/lora_recive");
    http.addHeader("Content-Type", "application/json");

    int code = http.POST(json);
    if (code > 0) {
        String resposta = http.getString();
        Serial.printf("HTTP %d: %s\n", code, resposta.c_str());
    } else {
        Serial.printf("Erro HTTP: %s\n", http.errorToString(code).c_str());
    }

    http.end();
  }
  else {
    Serial.println("Wi-Fi desconectado.");
  }
}
