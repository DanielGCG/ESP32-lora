#include "wifi_handler.h"
#include <WiFi.h>

const char* ssid = "Pacoca";
const char* password = "kimberli43";

void iniciarWiFi() {
  Serial.println("Conectando ao Wi-Fi...");
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("\nWi-Fi conectado!");
  Serial.println(WiFi.localIP());
}
