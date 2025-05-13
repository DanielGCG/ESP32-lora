#include "database_handler.h"
#include <WiFi.h>
#include <HTTPClient.h>
#include <time.h>
#include <Arduino.h>
#include <ArduinoJson.h>
#include "lora_handler.h"

const char* ntpServer = "pool.ntp.org";
const long gmtOffset_sec = -3 * 3600;
const int daylightOffset_sec = 0;


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

void configurarTempoNTP() {
  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
  Serial.println("Configurando horário via NTP...");

  struct tm timeinfo;
  int tentativas = 5;
  while (!getLocalTime(&timeinfo) && tentativas > 0) {
    Serial.println("Aguardando resposta do servidor NTP...");
    delay(2000);
    tentativas--;
  }

  if (tentativas == 0) {
    Serial.println("Falha ao obter hora via NTP.");
  } else {
    Serial.println("Hora sincronizada com sucesso!");
  }
}

void sendTime() {
  struct tm timeinfo;
  if (getLocalTime(&timeinfo)) {
    time_t timestamp = mktime(&timeinfo);

    char buffer[40];
    snprintf(buffer, sizeof(buffer), "!date:%llu", (uint64_t)timestamp);
    Serial.printf("Enviando via LoRa: %s\n", buffer);
    enviarMensagemLoRa(buffer);
  } else {
    Serial.println("Erro ao obter hora local.");
  }
}

void atualizarNotificacao(String mensagemID, String novoStatus) {
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;

    String json = "{\"mensagemID\":\"" + mensagemID + "\", \"status\":\"" + novoStatus + "\"}";

    http.begin("https://www.botecors.me/API/lora/lora_notifications");
    http.addHeader("Content-Type", "application/json");

    int code = http.sendRequest("PATCH", json);
    if (code > 0) {
      String resposta = http.getString();
      Serial.printf("Resposta do update (%d): %s\n", code, resposta.c_str());
    } else {
      Serial.printf("Erro ao atualizar notificação: %s\n", http.errorToString(code).c_str());
    }

    http.end();
  } else {
    Serial.println("Wi-Fi desconectado.");
  }
}

void receberNotificacoes() {
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;

    http.begin("https://www.botecors.me/API/lora/lora_notifications");
    http.addHeader("Content-Type", "application/json");

    int code = http.GET();
    if (code > 0) {
      String resposta = http.getString();
      Serial.println("Recebido Notificações e empilhando para envio por LoRa...");

      StaticJsonDocument<2048> doc;

      DeserializationError erro = deserializeJson(doc, resposta);
      if (erro) {
        Serial.print("Erro ao parsear JSON: ");
        Serial.println(erro.c_str());
        return;
      }

      JsonArray notificacoes = doc.as<JsonArray>();

      std::vector<String> pilha;

      // Empilha todas as mensagens com status "enviada"
      for (JsonObject notificacao : notificacoes) {
        String mensagem = notificacao["mensagem"];
        String status = notificacao["status"];

        if (status == "enviada") {
          pilha.push_back(mensagem);
        }
      }

      // Desempilha e envia as mensagens
      while (!pilha.empty()) {
        String mensagem = pilha.back();
        pilha.pop_back();
        Serial.printf("Enviando mensagem (PILHA): %s\n", mensagem.c_str());
        enviarMensagemSequenciadaLoRa(mensagem);
      }

    } else {
      Serial.printf("Erro ao buscar notificações: %s\n", http.errorToString(code).c_str());
    }

    http.end();
  } else {
    Serial.println("Wi-Fi desconectado.");
  }
}