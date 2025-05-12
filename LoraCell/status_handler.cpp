#include <time.h>
#include "status_handler.h"
#include "display_manager.h"

extern long long timestamp;

String horarioAtual = "S/H";  // Começa com "S/H"

unsigned long lastMillisRelogio = 0;
int ultimoMinuto = -1;

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
    }
  } else if (timestamp == 0) {
    horarioAtual = "S/H";
    ultimoMinuto = -1;
  }
}


