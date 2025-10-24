#include <time.h>
#include "status_handler.h"
#include "display_manager.h"
#include "lora_notification_reciver.h"

extern long long timestamp;

String horarioAtual = "S/H";  // Começa com "S/H"

unsigned long lastMillisRelogio = 0;
int ultimoMinuto = -1;
extern int num_mensagem_boas_vindas;

void recebeData(String data) {
  timestamp = atoll(data.c_str());
  ultimoMinuto = -1;
  Serial.println("Horário recebido.");
}

String obterHorario() {
  return horarioAtual;
}

void relogio() {
  unsigned long currentMillis = millis();

  if (timestamp > 0 && currentMillis - lastMillisRelogio >= 1000) {
    timestamp++;
    lastMillisRelogio = currentMillis;

    // Verifica se o minuto mudou
    time_t localTimestamp = (time_t)(timestamp - 3 * 3600);  // UTC-3
    struct tm timeinfo;
    localtime_r(&localTimestamp, &timeinfo);

    if (timeinfo.tm_min != ultimoMinuto) {
      ultimoMinuto = timeinfo.tm_min;

      // Atualiza o horário formatado
      horarioAtual = String(timeinfo.tm_hour < 10 ? "0" : "") + String(timeinfo.tm_hour) + ":" +
                     String(timeinfo.tm_min < 10 ? "0" : "") + String(timeinfo.tm_min);

      // Define o índice da mensagem de boas-vindas baseado na hora
      if (timeinfo.tm_hour >= 6 && timeinfo.tm_hour < 12) {
        num_mensagem_boas_vindas = 1;  // Bom dia
      } else if (timeinfo.tm_hour >= 12 && timeinfo.tm_hour < 18) {
        num_mensagem_boas_vindas = 2;  // Boa tarde
      } else if (timeinfo.tm_hour >= 18 || timeinfo.tm_hour < 6) {
        num_mensagem_boas_vindas = 3;  // Boa noite
      } else {
        num_mensagem_boas_vindas = 0;  // Olá
      }
    }
  } else if (timestamp == 0) {
    horarioAtual = "S/H";
    ultimoMinuto = -1;
    num_mensagem_boas_vindas = 0;
  }
}

void VextOFF() {
  pinMode(Vext, OUTPUT);
  digitalWrite(Vext, HIGH); // desliga Vext
}

unsigned long lastActivity = 0;

void resetInactivityTimer() {
  lastActivity = millis();
}

void enterDeepSleep() {
  Serial.println("Inatividade >> Entrando em deep sleep");

  // 1. Desliga o display
  myDisplay.displayOff();

  // 2. Desliga o lora e o Vext
  desligarLoRa();
  VextOFF();

  // 3. Wake-up por botão (ajuste o pino conforme seu botão)
  const gpio_num_t WAKE_PIN = GPIO_NUM_0;  
  esp_sleep_enable_ext0_wakeup(WAKE_PIN, 0);

  // 4. Deep sleep
  esp_deep_sleep_start();
}

void requisitarHorario() {
  enviarMensagemLoRa("!get_tower_time");
}

void requisitarPingTower() {
  enviarMensagemLoRa("!ping_tower");
}

void requisitarNotificacoes() {
  enviarMensagemLoRa("!req_not");
}


