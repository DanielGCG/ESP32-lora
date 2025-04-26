#include "LoRaWan_APP.h"
#include "Arduino.h"

#define RF_FREQUENCY                                915000000 // Hz
#define TX_OUTPUT_POWER                             14        // dBm
#define LORA_BANDWIDTH                              0         // [0: 125 kHz,
                                                              //  1: 250 kHz,
                                                              //  2: 500 kHz,
                                                              //  3: Reserved]
#define LORA_SPREADING_FACTOR                       7         // [SF7..SF12]
#define LORA_CODINGRATE                             1         // [1: 4/5,
                                                              //  2: 4/6,
                                                              //  3: 4/7,
                                                              //  4: 4/8]
#define LORA_PREAMBLE_LENGTH                        8         // Same for Tx and Rx
#define LORA_SYMBOL_TIMEOUT                         0         // Symbols
#define LORA_FIX_LENGTH_PAYLOAD_ON                  false
#define LORA_IQ_INVERSION_ON                        false
#define RX_TIMEOUT_VALUE                            1000
#define BUFFER_SIZE                                 200 // Define the payload size here
#define MAX_PACKETS_PER_SEQUENCE                    20  // Define quantidade máxima de pacotes por sequência
#define MAX_TEXTO_CHARS_IN_PACKET                   150 // Define limite de caracteres no texto de pacotes

char txpacket[BUFFER_SIZE];
char rxpacket[BUFFER_SIZE];

double txNumber;

bool lora_idle=true;

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
void OnTxDone( void );
void OnTxTimeout( void );

void setup() {
    Serial.begin(115200);
    Mcu.begin(HELTEC_BOARD,SLOW_CLK_TPYE);
	
    txNumber=0;

    RadioEvents.TxDone = OnTxDone;
    RadioEvents.TxTimeout = OnTxTimeout;
    
    Radio.Init( &RadioEvents );
    Radio.SetChannel( RF_FREQUENCY );
    Radio.SetTxConfig( MODEM_LORA, TX_OUTPUT_POWER, 0, LORA_BANDWIDTH,
                                   LORA_SPREADING_FACTOR, LORA_CODINGRATE,
                                   LORA_PREAMBLE_LENGTH, LORA_FIX_LENGTH_PAYLOAD_ON,
                                   true, 0, 0, LORA_IQ_INVERSION_ON, 3000 ); 
   }

// Função para criar uma sequência de pacotes de uma mensagem
Sequencia sequenciar_mensagem(String texto) {
  Sequencia sequencia_final;
  unsigned int id_mensagem = (unsigned int) millis();  // Gera um ID baseado no tempo

  sequencia_final.id_mensagem = id_mensagem;
  sequencia_final.total_pacotes = (texto.length() + MAX_TEXTO_CHARS_IN_PACKET - 1) / MAX_TEXTO_CHARS_IN_PACKET;

  for (int i = 0; i < sequencia_final.total_pacotes; i++) {
    Pacote pacote_temp;

    pacote_temp.id_mensagem = id_mensagem;
    String parte = texto.substring(i * MAX_TEXTO_CHARS_IN_PACKET, (i + 1) * MAX_TEXTO_CHARS_IN_PACKET);
    parte.toCharArray(pacote_temp.texto, MAX_TEXTO_CHARS_IN_PACKET);
    pacote_temp.num_pacote = i+1;
    pacote_temp.total_pacotes = sequencia_final.total_pacotes;

    sequencia_final.pacotes[i] = pacote_temp;
  }

  return sequencia_final;
}
char buffer[BUFFER_SIZE];

void loop()
{
  Radio.IrqProcess();

  if (lora_idle && Serial.available()) {
    String texto = Serial.readStringUntil('\n');
    texto.trim();

    Sequencia sequencia_final = sequenciar_mensagem(texto);

    for (int i = 0; i < sequencia_final.total_pacotes; i++) {
      Pacote pacote = sequencia_final.pacotes[i];

      Serial.printf("\nPacote[%d/%d] da sequencia[%d] enviei o seguinte texto: %s\n", 
        pacote.num_pacote, 
        pacote.total_pacotes, 
        pacote.id_mensagem, 
        pacote.texto);

      snprintf(buffer, BUFFER_SIZE, "%d[%d/%d]%s", 
         pacote.id_mensagem, 
         pacote.num_pacote, 
         pacote.total_pacotes, 
         pacote.texto); // MONTA A MENSAGEM

      Radio.Send((uint8_t *)buffer, strlen(buffer)); // ENVIA A MENSAGEM

      lora_idle = false;

      while (!lora_idle) {
        Radio.IrqProcess(); // Mantém o rádio respondendo
      }
    }
  }
}

void OnTxDone( void )
{
	Serial.println("TX done......");
	lora_idle = true;
}

void OnTxTimeout( void )
{
    Radio.Sleep( );
    Serial.println("TX Timeout......");
    lora_idle = true;
}