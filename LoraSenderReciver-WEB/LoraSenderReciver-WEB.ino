#include "LoRaWan_APP.h"
#include "Arduino.h"
#include <WiFi.h>
#include <HTTPClient.h>


#define RF_FREQUENCY                                915000000 // Hz
#define TX_OUTPUT_POWER                             14        // dBm
#define LORA_BANDWIDTH                              0         // [0: 125 kHz, 1: 250 kHz, 2: 500 kHz]
#define LORA_SPREADING_FACTOR                       7
#define LORA_CODINGRATE                             1
#define LORA_PREAMBLE_LENGTH                        8
#define LORA_SYMBOL_TIMEOUT                         0
#define LORA_FIX_LENGTH_PAYLOAD_ON                  false
#define LORA_IQ_INVERSION_ON                        false
#define RX_TIMEOUT_VALUE                            1000
#define BUFFER_SIZE                                 200
#define MAX_PACKETS_PER_SEQUENCE                    20
#define MAX_TEXTO_CHARS_IN_PACKET                   150

// 🚨 Troque pelos dados da sua rede:
const char* ssid = "Pacoca";
const char* password = "kimberli43";

char txpacket[BUFFER_SIZE];
char rxpacket[BUFFER_SIZE];

double txNumber;
bool lora_idle = true;

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

static RadioEvents_t RadioEvents;

void OnTxDone(void);
void OnTxTimeout(void);
void OnRxDone(uint8_t *payload, uint16_t size, int16_t rssi, int8_t snr);
void OnRxTimeout(void);
void OnRxError(void);

void setup() {
  Serial.begin(115200);
  Mcu.begin(HELTEC_BOARD, SLOW_CLK_TPYE);

  Serial.println("Conectando ao Wi-Fi...");
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("\nWi-Fi conectado!");
  Serial.println(WiFi.localIP());

  txNumber = 0;

  RadioEvents.TxDone = OnTxDone;
  RadioEvents.TxTimeout = OnTxTimeout;
  RadioEvents.RxDone = OnRxDone;
  RadioEvents.RxTimeout = OnRxTimeout;
  RadioEvents.RxError = OnRxError;

  Radio.Init(&RadioEvents);
  Radio.SetChannel(RF_FREQUENCY);
  Radio.SetTxConfig(MODEM_LORA, TX_OUTPUT_POWER, 0, LORA_BANDWIDTH,
                    LORA_SPREADING_FACTOR, LORA_CODINGRATE,
                    LORA_PREAMBLE_LENGTH, LORA_FIX_LENGTH_PAYLOAD_ON,
                    true, 0, 0, LORA_IQ_INVERSION_ON, 3000);

  Radio.Rx(RX_TIMEOUT_VALUE);  // Começa a escutar
}

Sequencia sequenciar_mensagem(String texto) {
  Sequencia sequencia_final;
  unsigned int id_mensagem = (unsigned int) millis();

  sequencia_final.id_mensagem = id_mensagem;
  sequencia_final.total_pacotes = (texto.length() + MAX_TEXTO_CHARS_IN_PACKET - 1) / MAX_TEXTO_CHARS_IN_PACKET;

  for (int i = 0; i < sequencia_final.total_pacotes; i++) {
    Pacote pacote_temp;
    pacote_temp.id_mensagem = id_mensagem;

    String parte = texto.substring(i * MAX_TEXTO_CHARS_IN_PACKET, (i + 1) * MAX_TEXTO_CHARS_IN_PACKET);
    parte.toCharArray(pacote_temp.texto, MAX_TEXTO_CHARS_IN_PACKET);

    pacote_temp.num_pacote = i + 1;
    pacote_temp.total_pacotes = sequencia_final.total_pacotes;

    sequencia_final.pacotes[i] = pacote_temp;
  }

  return sequencia_final;
}

char buffer[BUFFER_SIZE];

void loop() {
  Radio.IrqProcess();

  // Envia mensagens da serial
  if (lora_idle && Serial.available()) {
    String texto = Serial.readStringUntil('\n');
    texto.trim();

    Sequencia sequencia_final = sequenciar_mensagem(texto);

    for (int i = 0; i < sequencia_final.total_pacotes; i++) {
      Pacote pacote = sequencia_final.pacotes[i];

      snprintf(buffer, BUFFER_SIZE, "%d[%d/%d]%s",
               pacote.id_mensagem,
               pacote.num_pacote,
               pacote.total_pacotes,
               pacote.texto);

      Serial.printf("\nEnviando LoRa: %s\n", buffer);

      Radio.Send((uint8_t *)buffer, strlen(buffer));
      lora_idle = false;

      while (!lora_idle) {
        Radio.IrqProcess();  // Espera o envio terminar
      }
    }
  }
}

// 📡 CALLBACKS

void OnTxDone(void) {
  Serial.println("TX concluído.");
  lora_idle = true;
  Radio.Rx(RX_TIMEOUT_VALUE);  // Volta a escutar
}

void OnTxTimeout(void) {
  Serial.println("TX Timeout.");
  lora_idle = true;
  Radio.Rx(RX_TIMEOUT_VALUE);
}

void OnRxDone(uint8_t *payload, uint16_t size, int16_t rssi, int8_t snr) {
  memcpy(rxpacket, payload, size);
  rxpacket[size] = '\0';  // Garante que termina como string

  Serial.printf("\nRecebido LoRa: %s\n", rxpacket);
  Serial.printf("RSSI: %d dBm, SNR: %d dB\n", rssi, snr);

  // Envia para servidor via HTTP
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;

    String serverUrl = "http://botecors.me/API/lora/lora_recive";

    // Escapa aspas duplas na mensagem (básico)
    String mensagemEscapada = String(rxpacket);
    mensagemEscapada.replace("\"", "\\\"");

    String json_payload = "{\"mensagem\":\"" + mensagemEscapada + "\"}";

    http.begin(serverUrl);
    http.addHeader("Content-Type", "application/json");

    int httpResponseCode = http.POST(json_payload);

    if (httpResponseCode > 0) {
      String resposta = http.getString();
      Serial.printf("HTTP %d: %s\n", httpResponseCode, resposta.c_str());
    } else {
      Serial.printf("Erro HTTP: %s\n", http.errorToString(httpResponseCode).c_str());
    }

    http.end();
  } else {
    Serial.println("Wi-Fi desconectado, não foi possível enviar.");
  }

  Radio.Rx(RX_TIMEOUT_VALUE);  // Continua escutando
}

void OnRxTimeout(void) {
  Radio.Rx(RX_TIMEOUT_VALUE);  // Reinicia recepção
}

void OnRxError(void) {
  Serial.println("Erro na recepção.");
  Radio.Rx(RX_TIMEOUT_VALUE);
}
