#include "LoRaWan_APP.h"
#include "Arduino.h"
#include <Wire.h>
#include "HT_SSD1306Wire.h"
#include "HT_DisplayUi.h"
#include <deque>

#define RF_FREQUENCY                                915000000 // Hz
#define TX_OUTPUT_POWER                             14        // dBm
#define LORA_BANDWIDTH                              0         
#define LORA_SPREADING_FACTOR                       7         
#define LORA_CODINGRATE                             1         
#define LORA_PREAMBLE_LENGTH                        8         
#define LORA_SYMBOL_TIMEOUT                         0         
#define LORA_FIX_LENGTH_PAYLOAD_ON                  false
#define LORA_IQ_INVERSION_ON                        false

#define RX_TIMEOUT_VALUE                            1000
#define BUFFER_SIZE                                 200

static SSD1306Wire display(0x3c, 500000, SDA_OLED, SCL_OLED, GEOMETRY_128_64, RST_OLED);

char txpacket[BUFFER_SIZE];
char rxpacket[BUFFER_SIZE];

static RadioEvents_t RadioEvents;
int16_t txNumber;
int16_t rssi, rxSize;
bool lora_idle = true;

std::vector<String> linhas;
size_t totalPages = 0;
size_t paginaAtual = 0;
unsigned long ultimaTrocaDePagina = 0;
unsigned long intervaloPagina = 5500; // 5,5 segundos
String mensagemAtual = "";

char* dynamic_buffer = (char*)malloc(BUFFER_SIZE);  // Declarado apenas uma vez
size_t buffer_size = BUFFER_SIZE;

void inicializarDisplay() {
  display.init();
  display.clear();
  display.setFont(ArialMT_Plain_10);
  display.setTextAlignment(TEXT_ALIGN_LEFT);
}

std::vector<String> quebrarEmLinhas(String texto) {
  std::vector<String> resultado;
  int lineHeight = 12;
  int maxWidth = display.getWidth();
  
  int start = 0;
  
  while (start < texto.length()) {
    int end = texto.indexOf(' ', start);
    if (end == -1) end = texto.length();
    String word = texto.substring(start, end);
    String line = word;
    start = end + 1;

    while (start < texto.length()) {
      end = texto.indexOf(' ', start);
      if (end == -1) end = texto.length();
      String nextWord = texto.substring(start, end);
      
      if (display.getStringWidth(line + " " + nextWord) >= maxWidth) break;
      
      line += " " + nextWord;
      start = end + 1;
    }
    resultado.push_back(line);
  }
  return resultado;
}

void mostrarPaginaAtual() {
  int lineHeight = 12;
  int x = 0;
  int y = 0;
  int maxHeight = display.getHeight();
  int maxLines = maxHeight / lineHeight;

  display.clear();
  y = 0;

  // Calcula corretamente o início de cada linha para evitar o corte do texto
  for (size_t i = paginaAtual * maxLines; i < (paginaAtual + 1) * maxLines && i < linhas.size(); ++i) {
    display.drawString(x, y, linhas[i]);
    y += lineHeight;
  }

  display.display();
}

void atualizarDisplay(String texto) {
  // Reafirma o alinhamento
  display.setTextAlignment(TEXT_ALIGN_LEFT);
  mensagemAtual = texto;
  linhas = quebrarEmLinhas(mensagemAtual);

  // Calcular corretamente o número de páginas
  size_t maxLines = display.getHeight() / 12;
  totalPages = (linhas.size() + maxLines - 1) / maxLines;

  paginaAtual = 0;
  ultimaTrocaDePagina = millis();

  mostrarPaginaAtual();
}

void ligarVext() {
  pinMode(Vext, OUTPUT);
  digitalWrite(Vext, LOW);
}

void desenharBarraDeCarregamento(int atual, int totalEsperado) {
  // Limpa a tela
  display.clear();

  // Calcula a porcentagem de progresso (limitada entre 0 e 1)
  float progresso = (float)atual / totalEsperado;
  if (progresso > 1.0) progresso = 1.0;
  if (progresso < 0.0) progresso = 0.0;

  // Define as dimensões da barra
  int margem = 1;
  int x = margem;
  int y = display.getHeight() / 2;
  int larguraTotal = display.getWidth() - 2 * margem;
  int alturaBarra = 10;

  // Desenha o contorno da barra
  display.drawRect(x, y, larguraTotal, alturaBarra);

  // Calcula e desenha a parte preenchida
  int larguraPreenchida = progresso * (larguraTotal - 2); // subtrai 2 para respeitar as bordas internas
  display.fillRect(x + 1, y + 1, larguraPreenchida, alturaBarra - 2); // dentro da borda

  // Exibe o progresso numérico
  display.setTextAlignment(TEXT_ALIGN_CENTER);
  display.drawString(display.getWidth() / 2, y + alturaBarra + 2, String(atual) + "/" + String(totalEsperado));

  // Atualiza o display
  display.display();
}

void loop() {
  if (lora_idle) {
    lora_idle = false;
    Serial.println("into RX mode");
    Radio.Rx(0);
  }

  // alternar página automaticamente (corrigido)
  if (totalPages > 0 && millis() - ultimaTrocaDePagina > intervaloPagina) {
    paginaAtual = (paginaAtual + 1) % totalPages;
    ultimaTrocaDePagina = millis();
    mostrarPaginaAtual();
  }

  Radio.IrqProcess();
}

String idMensagemAtual = ""; // Declare fora da função, como variável global

void OnRxDone(uint8_t *payload, uint16_t size, int16_t rssiParam, int8_t snr) {
  rxSize = size;
  memcpy(rxpacket, payload, size);
  rxpacket[size] = '\0'; // Garante terminação nula
  Serial.printf("\r\nreceived packet \"%s\" with rssi %d , length %d\r\n", rxpacket, rssiParam, rxSize);

  String rxString = String(rxpacket);

  // Extrair marcador e ID antes dele
  int marcadorInicio = rxString.indexOf('[');
  int marcadorFim = rxString.indexOf(']');
  String idRecebido = "";
  String fragmentoVisivel = rxString;

  if (marcadorInicio != -1 && marcadorFim != -1 && marcadorFim > marcadorInicio) {
    idRecebido = rxString.substring(0, marcadorInicio);
    fragmentoVisivel = rxString.substring(marcadorFim + 1);
  }

  // Se o ID for diferente do atual, limpa a mensagem acumulada
  if (idRecebido != idMensagemAtual) {
    Serial.println("Novo ID detectado, limpando buffer.");
    idMensagemAtual = idRecebido;
    mensagemAtual = "";
  }

  // Acumula fragmento diretamente
  mensagemAtual += fragmentoVisivel;

  // Checa se é o último pacote
  if (marcadorInicio != -1 && marcadorFim != -1) {
    String marcador = rxString.substring(marcadorInicio + 1, marcadorFim); // "x/y"
    int barra = marcador.indexOf('/');
    if (barra != -1) {
      int x = marcador.substring(0, barra).toInt();
      int y = marcador.substring(barra + 1).toInt();
      desenharBarraDeCarregamento(x, y);
      if (x == y) {
        // Atualiza display
        atualizarDisplay(mensagemAtual);
        Serial.println("Último pacote recebido.");
        Radio.Sleep();
      }
    }
  }

  lora_idle = true;
}

void setup() {
  Serial.begin(115200);
  Mcu.begin(HELTEC_BOARD, SLOW_CLK_TPYE);
  ligarVext();
  delay(100);
  inicializarDisplay();

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