#include "wifi_handler.h"
#include <WiFi.h>

const char* ssid1 = "Pacoca";
const char* password1 = "kimberli43";

const char* ssid2 = "Wifi_PET_Publico";
const char* password2 = "petpublico";

bool conectarWiFi(const char* ssid, const char* password, int tentativasMax) {
  Serial.printf("Tentando conectar à rede: %s\n", ssid);
  WiFi.begin(ssid, password);

  int tentativas = 0;
  while (WiFi.status() != WL_CONNECTED && tentativas < tentativasMax) {
    delay(1000);
    Serial.print(".");
    tentativas++;
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\nWi-Fi conectado!");
    Serial.print("IP: ");
    Serial.println(WiFi.localIP());
    return true;
  } else {
    Serial.println("\nFalha na conexão.");
    return false;
  }
}

void iniciarWiFi() {
  if (conectarWiFi(ssid1, password1, 3)) {
    return;
  }

  // Tenta a segunda rede
  conectarWiFi(ssid2, password2, 3);
}
