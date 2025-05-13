#include "lora_handler.h"
#include "mensagem_handler.h"
#include "database_handler.h"

#define RF_FREQUENCY 915000000
#define TX_OUTPUT_POWER 22
#define LORA_BANDWIDTH 0
#define LORA_SPREADING_FACTOR 7
#define LORA_CODINGRATE 1
#define LORA_PREAMBLE_LENGTH 8
#define LORA_SYMBOL_TIMEOUT 0
#define LORA_FIX_LENGTH_PAYLOAD_ON false
#define LORA_IQ_INVERSION_ON false

#define RX_TIMEOUT_VALUE 1000
#define BUFFER_SIZE 200

char rxpacket[BUFFER_SIZE];
char buffer[BUFFER_SIZE];
int16_t rxSize;
bool lora_idle = true;
volatile bool tx_done = true;
static RadioEvents_t RadioEvents;
int16_t txNumber;
int16_t rssi;

String mensagemAtual = "";
String idMensagemAtual = "";
String esperandoAckDe = "";
bool aguardandoConfirmacao = false;

Sequencia sequenciaAtual;
int pacoteAtualIndex = 0;
int tentativasPacoteAtual = 0;
unsigned long tempoUltimoEnvio = 0;


void OnTxDone(void) {
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
                    
  Radio.Rx(RX_TIMEOUT_VALUE);
}

void enviarMensagemLoRa(String texto) {
  Radio.Send((uint8_t *)texto.c_str(), strlen(texto.c_str()));
  lora_idle = false;
  
  // Espera TX terminar
  while (!lora_idle) {
    Radio.IrqProcess();
  }

  delay(100);
  Radio.Rx(RX_TIMEOUT_VALUE);
}

void enviarMensagemSequenciadaLoRa(String texto) {
  mensagemAtual = "";
  idMensagemAtual = "";
  aguardandoConfirmacao = true;

  sequenciaAtual = sequenciarMensagem(texto);
  pacoteAtualIndex = 0;
  tentativasPacoteAtual = 0;
  tempoUltimoEnvio = 0;

  Serial.printf("Total de pacotes: %d\n", sequenciaAtual.total_pacotes);
}

void verificaConfirmacao() {
  if (!aguardandoConfirmacao || pacoteAtualIndex >= sequenciaAtual.total_pacotes) return;

  if (millis() - tempoUltimoEnvio >= 5000 || tentativasPacoteAtual == 0) {
    Pacote p = sequenciaAtual.pacotes[pacoteAtualIndex];

    char numPacoteStr[3];
    char totalPacotesStr[3];
    snprintf(numPacoteStr, sizeof(numPacoteStr), "%02d", p.num_pacote);
    snprintf(totalPacotesStr, sizeof(totalPacotesStr), "%02d", p.total_pacotes);
    snprintf(buffer, BUFFER_SIZE, "%s[%s/%s]%s", p.id_mensagem.c_str(), numPacoteStr, totalPacotesStr, p.texto);

    esperandoAckDe = "ACK" + p.id_mensagem + "[" + numPacoteStr + "/" + totalPacotesStr + "]";

    Serial.printf("Enviando... (%d/3 tentativas)\n", tentativasPacoteAtual + 1);
    enviarMensagemLoRa(buffer);

    tempoUltimoEnvio = millis();
    tentativasPacoteAtual++;

    if (tentativasPacoteAtual >= 3) {
      Serial.println("Falha ao receber confirmação após 3 tentativas.");
      aguardandoConfirmacao = false;
    }
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
    if (comando.indexOf("ping_cell") != -1) {
      enviarMensagemLoRa("!ping_cell");
    }
    if (comando.indexOf("req_not") != -1) {
      receberNotificacoes();
    }
  }

  verificaConfirmacao();
  Radio.IrqProcess();
}

void OnRxDone(uint8_t *payload, uint16_t size, int16_t rssi, int8_t snr) {
  memset(rxpacket, 0, BUFFER_SIZE);
  rxSize = 0;

  memcpy(rxpacket, payload, size);
  rxpacket[size] = '\0';

  String rxString = String(rxpacket);

  if (rxString.startsWith("ACK")) {
    if (aguardandoConfirmacao && rxString == esperandoAckDe) {
      Serial.println("Recebido.");
      aguardandoConfirmacao = false;
      esperandoAckDe = "";
      tentativasPacoteAtual = 0;
      pacoteAtualIndex++;

      if (pacoteAtualIndex < sequenciaAtual.total_pacotes) {
        aguardandoConfirmacao = true;
        tempoUltimoEnvio = 0;
      } else {
        Serial.println("Todos os pacotes enviados com sucesso!\n");
      }
    }

  } else if (rxString.indexOf('[') == -1 || rxString.indexOf(']') == -1) {
    if (rxString.indexOf("!get_tower_time") != -1) {
      sendTime();
    }
    if (rxString.indexOf("!ping_tower") != -1) {
      enviarMensagemLoRa("!pong_tower");
    }
    if (rxString.indexOf("!pong_cell") != -1) {
      Serial.printf("\nPong from cell! RSSI: %d dBm  |  SNR: %d\n", rssi, snr);
    }
  } else {
    // Fragmento com ID[x/y]texto
    int marcadorInicio = rxString.indexOf('[');
    int marcadorFim = rxString.indexOf(']');
    String idRecebido = rxString.substring(0, marcadorInicio);
    String fragmento = rxString.substring(marcadorFim + 1);
    String marcador = rxString.substring(marcadorInicio + 1, marcadorFim);

    int barra = marcador.indexOf('/');
    if (barra == -1) {
      Serial.println("Formato inválido de fragmento.");
      Radio.Rx(RX_TIMEOUT_VALUE);
      return;
    }

    String strX = marcador.substring(0, barra);
    String strY = marcador.substring(barra + 1);
    int x = strX.toInt();
    int y = strY.toInt();

    // Confirmação
    String confirmacao = "ACK" + idRecebido + "[" + strX + "/" + strY + "]";
    enviarMensagemLoRa(confirmacao);

    // Monta mensagem
    if (idRecebido.length() > 0 && idRecebido != idMensagemAtual) {
      idMensagemAtual = idRecebido;
      mensagemAtual = "";
    }

    mensagemAtual += fragmento;
    Serial.printf("Progresso: %d de %d\n", x, y);

    if (x == y) {
      Serial.println("\nMensagem completa:");
      Serial.println(mensagemAtual);
      enviarParaDatabase(mensagemAtual, idMensagemAtual);
      mensagemAtual = "";
      idMensagemAtual = "";
    }
  }

  Radio.Rx(RX_TIMEOUT_VALUE);
}

void OnRxTimeout() {
  Radio.Rx(RX_TIMEOUT_VALUE);
}
