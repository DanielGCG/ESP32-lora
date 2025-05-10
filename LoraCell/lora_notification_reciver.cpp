#include "LoRaWan_APP.h"
#include "mensagem_handler.h"
#include "lora_notification_reciver.h"
#include "notifications.h"
#include "status_handler.h"
#include "display_manager.h"

#define RF_FREQUENCY                915000000 // Hz
#define TX_OUTPUT_POWER             22        // dBm
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
char buffer[BUFFER_SIZE];
int16_t rxSize;
bool lora_idle = true;

static RadioEvents_t RadioEvents;
int16_t txNumber;
int16_t rssi;

String mensagemAtual = "";
String idMensagemAtual = "";

volatile bool tx_done = true;

void OnTxDone(void) {
  Serial.println("Envio concluído.");
  tx_done = true;
  lora_idle = true;
  Radio.Rx(RX_TIMEOUT_VALUE);
}

void OnTxTimeout() {
  Serial.println("TX Timeout.");
  lora_idle = true;
  Radio.Rx(RX_TIMEOUT_VALUE);
}

void setupLoRa() {
  Serial.begin(115200);
  Mcu.begin(HELTEC_BOARD, SLOW_CLK_TPYE);

  txNumber = 0;
  RadioEvents.TxDone = OnTxDone;
  rssi = 0;

  RadioEvents.RxDone = OnRxDone;
  Radio.Init(&RadioEvents);
  Radio.SetChannel(RF_FREQUENCY);
  Radio.SetRxConfig(MODEM_LORA, LORA_BANDWIDTH, LORA_SPREADING_FACTOR,
                    LORA_CODINGRATE, 0, LORA_PREAMBLE_LENGTH,
                    LORA_SYMBOL_TIMEOUT, LORA_FIX_LENGTH_PAYLOAD_ON,
                    0, true, 0, 0, LORA_IQ_INVERSION_ON, true);
  Radio.SetTxConfig(MODEM_LORA, TX_OUTPUT_POWER, 0, LORA_BANDWIDTH,
                    LORA_SPREADING_FACTOR, LORA_CODINGRATE,
                    LORA_PREAMBLE_LENGTH, LORA_FIX_LENGTH_PAYLOAD_ON,
                    true, false, 0, LORA_IQ_INVERSION_ON, 3000);
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

void loopLoRa() {
  if (Serial.available()) {
    String comando = Serial.readStringUntil('\n');
    comando.trim();
    if (comando.startsWith("enviar:")) {
      String conteudo = comando.substring(8);
      enviarMensagemSequenciadaLoRa(conteudo);
    }
    if (comando.startsWith("ping_tower")) {
      enviarMensagemLoRa("!ping_tower");
    }
    if (comando.startsWith("get_tower_time")) {
      enviarMensagemLoRa("!get_tower_time");
    }
  }
  Radio.IrqProcess();
}

void OnRxDone(uint8_t *payload, uint16_t size, int16_t rssi, int8_t snr) {
  memcpy(rxpacket, payload, size);
  rxpacket[size] = '\0';

  Serial.printf("\nRecebido LoRa: %s\n", rxpacket);
  setloraSignalStrength(rssi, snr);
  Serial.printf("RSSI: %d dBm, SNR: %d dB\n", rssi, snr);

  String rxString = String(rxpacket);
  int marcadorInicio = rxString.indexOf('[');
  int marcadorFim = rxString.indexOf(']');

  if (marcadorInicio == -1 || marcadorFim == -1) {
    if (rxString.indexOf("!ping_cell") != -1) {
      enviarMensagemLoRa("!pong_cell");
    }
    if (rxString.indexOf("!date") != -1) {
      String data = rxString.substring(6);
      recebeData(data);
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
        addNotification(mensagemAtual);

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