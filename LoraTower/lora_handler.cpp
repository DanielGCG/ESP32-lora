#include "lora_handler.h"
#include "LoRaWan_APP.h"
#include "mensagem_handler.h"
#include <WiFi.h>
#include <HTTPClient.h>
#include <time.h>

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

String idMensagemAtual = "";
String mensagemAtual = "";

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

void enviarMensagemSequenciadaLoRa(String texto) {
  Sequencia sequencia = sequenciarMensagem(texto);

  for (int i = 0; i < sequencia.total_pacotes; i++) {
    Pacote p = sequencia.pacotes[i];
    snprintf(buffer, BUFFER_SIZE, "%s[%d/%d]%s", p.id_mensagem, p.num_pacote, p.total_pacotes, p.texto);
    Serial.printf("\nEnviando LoRa: %s\n", buffer);

    Radio.Send((uint8_t *)buffer, strlen(buffer));
    lora_idle = false;

    while (!lora_idle) {
      Radio.IrqProcess();
    }
  }
}

void enviarMensagemLoRa(String texto) {
    Serial.printf("\nEnviando LoRa: %s\n", texto.c_str());

    Radio.Send((uint8_t *)texto.c_str(), strlen(texto.c_str()));
    lora_idle = false;

    while (!lora_idle) {
      Radio.IrqProcess();
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

void sendTime() {
  Serial.println("SendTime chamado!");
  time_t now;
    struct tm timeinfo;
    time(&now);
    localtime_r(&now, &timeinfo);

    char buffer[30];
    strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", &timeinfo);

    enviarMensagemLoRa(buffer);
}

void storeInDatabase () {

}

void loopLoRa() {
  if (Serial.available()) {
    String comando = Serial.readStringUntil('\n');
    comando.trim();
    if (comando.startsWith("enviar:")) {
      String conteudo = comando.substring(7);
      enviarMensagemSequenciadaLoRa(conteudo);
    }
    if (comando.indexOf("ping_cell") != -1) {
      delay(500);
      enviarMensagemLoRa("pong_cell");
    }
  }
  Radio.IrqProcess();
}

void OnRxDone(uint8_t *payload, uint16_t size, int16_t rssi, int8_t snr) {
  memcpy(rxpacket, payload, size);
  rxpacket[size] = '\0';

  Serial.printf("\nRecebido LoRa: %s\n", rxpacket);
  Serial.printf("RSSI: %d dBm, SNR: %d dB\n", rssi, snr);

  String rxString = String(rxpacket);
  int marcadorInicio = rxString.indexOf('[');
  int marcadorFim = rxString.indexOf(']');

  if (marcadorInicio == -1 || marcadorFim == -1) {
    if (rxString.indexOf("get_tower_time") != -1) {
      delay(500);
      sendTime();
    }
    if (rxString.indexOf("ping_tower") != -1) {
      delay(500);
      enviarMensagemLoRa("pong_tower");
    }
  }
  else {
  String idRecebido = "";
  String fragmento = rxString;

  // Extrai ID e texto, se possível
  if (marcadorInicio != -1 && marcadorFim != -1 && marcadorFim > marcadorInicio) {
    idRecebido = rxString.substring(0, marcadorInicio);
    fragmento = rxString.substring(marcadorFim + 1);
  }

  // Se mudou o ID, é nova mensagem
  if (idRecebido.length() > 0 && idRecebido != idMensagemAtual) {
    Serial.println("Novo ID detectado. Limpando buffer.");
    idMensagemAtual = idRecebido;
    mensagemAtual = "";
  }

  mensagemAtual += fragmento;

  // Verifica marcador [x/y]
  if (marcadorInicio != -1 && marcadorFim != -1) {
    String marcador = rxString.substring(marcadorInicio + 1, marcadorFim);
    int barra = marcador.indexOf('/');
    if (barra != -1) {
      int x = marcador.substring(0, barra).toInt();
      int y = marcador.substring(barra + 1).toInt();

      Serial.printf("Progresso: %d de %d\n", x, y);

      // Se é o último pacote, envia por HTTP
      if (x == y) {  // Se é o último pacote
        Serial.println("Último pacote recebido.");
        Serial.println("Mensagem completa:");
        Serial.println(mensagemAtual);

        if (WiFi.status() == WL_CONNECTED) {
            HTTPClient http;

            // Escapa as aspas na mensagem
            String mensagemEscapada = mensagemAtual;
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
        } else {
            Serial.println("Wi-Fi desconectado.");
        }

        // Limpa estado para próxima mensagem
        mensagemAtual = "";
        idMensagemAtual = "";
        }
      }
    }
  }
  lora_idle = true;
  Radio.Rx(RX_TIMEOUT_VALUE);
}

void OnRxTimeout() {
  Radio.Rx(RX_TIMEOUT_VALUE);
}

void OnRxError() {
  Serial.println("Erro na recepção.");
  Radio.Rx(RX_TIMEOUT_VALUE);
}
