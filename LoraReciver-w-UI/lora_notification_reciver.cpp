#include "LoRaWan_APP.h"
#include "lora_notification_reciver.h"
#include "notifications.h"

#define RF_FREQUENCY                915000000 // Hz
#define TX_OUTPUT_POWER             14        // dBm
#define LORA_BANDWIDTH              0         
#define LORA_SPREADING_FACTOR       7         
#define LORA_CODINGRATE             1         
#define LORA_PREAMBLE_LENGTH        8         
#define LORA_SYMBOL_TIMEOUT         0         
#define LORA_FIX_LENGTH_PAYLOAD_ON  false
#define LORA_IQ_INVERSION_ON        false

#define RX_TIMEOUT_VALUE            1000
#define BUFFER_SIZE                 200

char rxpacket[BUFFER_SIZE];
int16_t rxSize;
bool lora_idle = true;

static RadioEvents_t RadioEvents;
int16_t txNumber;
int16_t rssi;

String mensagemAtual = "";
String idMensagemAtual = "";

void setupLoRa() {
  Serial.begin(115200);
  Mcu.begin(HELTEC_BOARD, SLOW_CLK_TPYE);

  txNumber = 0;
  rssi = 0;

  RadioEvents.RxDone = OnRxDone;
  Radio.Init(&RadioEvents);
  Radio.SetChannel(RF_FREQUENCY);
  Radio.SetRxConfig(MODEM_LORA, LORA_BANDWIDTH, LORA_SPREADING_FACTOR,
                    LORA_CODINGRATE, 0, LORA_PREAMBLE_LENGTH,
                    LORA_SYMBOL_TIMEOUT, LORA_FIX_LENGTH_PAYLOAD_ON,
                    0, true, 0, 0, LORA_IQ_INVERSION_ON, true);
}

void loopLoRa() {
  if (lora_idle) {
    lora_idle = false;
    Serial.println("into RX mode");
    Radio.Rx(0);
  }

  Radio.IrqProcess();
}

void OnRxDone(uint8_t *payload, uint16_t size, int16_t rssiParam, int8_t snr) {
  rxSize = size;
  memcpy(rxpacket, payload, size);
  rxpacket[size] = '\0';
  Serial.printf("\r\nreceived packet \"%s\" with rssi %d , length %d\r\n", rxpacket, rssiParam, rxSize);

  String rxString = String(rxpacket);

  int marcadorInicio = rxString.indexOf('[');
  int marcadorFim = rxString.indexOf(']');
  String idRecebido = "";
  String fragmentoVisivel = rxString;

  if (marcadorInicio != -1 && marcadorFim != -1 && marcadorFim > marcadorInicio) {
    idRecebido = rxString.substring(0, marcadorInicio);
    fragmentoVisivel = rxString.substring(marcadorFim + 1);
  }

  if (idRecebido != idMensagemAtual) {
    Serial.println("Novo ID detectado, limpando buffer.");
    idMensagemAtual = idRecebido;
    mensagemAtual = "";
  }

  mensagemAtual += fragmentoVisivel;

  if (marcadorInicio != -1 && marcadorFim != -1) {
    String marcador = rxString.substring(marcadorInicio + 1, marcadorFim); // "x/y"
    int barra = marcador.indexOf('/');
    if (barra != -1) {
      int x = marcador.substring(0, barra).toInt();
      int y = marcador.substring(barra + 1).toInt();
      Serial.printf("Progresso: %d de %d\n", x, y);
      if (x == y) {
        Serial.println("Último pacote recebido.");
        Serial.println("Mensagem completa:");
        Serial.println(mensagemAtual);
        addNotification(mensagemAtual);
        Radio.Sleep();
      }
    }
  }

  lora_idle = true;
}