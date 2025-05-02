#include "lora_handler.h"
#include "LoRaWan_APP.h"
#include "mensagem_handler.h"
#include <WiFi.h>
#include <HTTPClient.h>

#define RF_FREQUENCY 915000000
#define TX_OUTPUT_POWER 14
#define LORA_BANDWIDTH 0
#define LORA_SPREADING_FACTOR 7
#define LORA_CODINGRATE 1
#define LORA_PREAMBLE_LENGTH 8
#define LORA_SYMBOL_TIMEOUT 0
#define LORA_FIX_LENGTH_PAYLOAD_ON false
#define LORA_IQ_INVERSION_ON false
#define RX_TIMEOUT_VALUE 1000
#define BUFFER_SIZE 200

RadioEvents_t RadioEvents;
extern bool lora_idle;

char rxpacket[BUFFER_SIZE];
char buffer[BUFFER_SIZE];
bool lora_idle = true;

void processarIrqLoRa() {
  Radio.IrqProcess();
}

void OnTxDone();
void OnTxTimeout();
void OnRxDone(uint8_t *payload, uint16_t size, int16_t rssi, int8_t snr);
void OnRxTimeout();
void OnRxError();

void configurarLoRa() {
  Mcu.begin(HELTEC_BOARD, SLOW_CLK_TPYE);

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

  Radio.Rx(RX_TIMEOUT_VALUE);
}

void enviarMensagemLoRa(String texto) {
  Sequencia sequencia = sequenciarMensagem(texto);

  for (int i = 0; i < sequencia.total_pacotes; i++) {
    Pacote p = sequencia.pacotes[i];
    snprintf(buffer, BUFFER_SIZE, "%d[%d/%d]%s", p.id_mensagem, p.num_pacote, p.total_pacotes, p.texto);
    Serial.printf("\nEnviando LoRa: %s\n", buffer);

    Radio.Send((uint8_t *)buffer, strlen(buffer));
    lora_idle = false;

    while (!lora_idle) {
      Radio.IrqProcess();
    }
  }
}

void OnTxDone() {
  Serial.println("TX concluído.");
  lora_idle = true;
  Radio.Rx(RX_TIMEOUT_VALUE);
}

void OnTxTimeout() {
  Serial.println("TX Timeout.");
  lora_idle = true;
  Radio.Rx(RX_TIMEOUT_VALUE);
}

void OnRxDone(uint8_t *payload, uint16_t size, int16_t rssi, int8_t snr) {
  memcpy(rxpacket, payload, size);
  rxpacket[size] = '\0';

  Serial.printf("\nRecebido LoRa: %s\n", rxpacket);
  Serial.printf("RSSI: %d dBm, SNR: %d dB\n", rssi, snr);

  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;
    String mensagemEscapada = String(rxpacket);
    mensagemEscapada.replace("\"", "\\\"");
    String json = "{\"mensagem\":\"" + mensagemEscapada + "\"}";

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
  } else {
    Serial.println("Wi-Fi desconectado.");
  }

  Radio.Rx(RX_TIMEOUT_VALUE);
}

void OnRxTimeout() {
  Radio.Rx(RX_TIMEOUT_VALUE);
}

void OnRxError() {
  Serial.println("Erro na recepção.");
  Radio.Rx(RX_TIMEOUT_VALUE);
}
