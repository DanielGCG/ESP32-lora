#include <Wire.h>
#include "HT_SSD1306Wire.h"

// Definições de pinos, caso não estejam declaradas antes
#define SDA_OLED 21
#define SCL_OLED 22
#define RST_OLED -1

// Instância global do display
SSD1306Wire display(0x3c, 500000, SDA_OLED, SCL_OLED, GEOMETRY_128_64, RST_OLED);

/**
 * Exibe uma mensagem na tela OLED.
 * @param titulo Título da mensagem (linha superior)
 * @param corpo Corpo da mensagem (linha inferior)
 */
void mostrarMensagemNaTela(const String& titulo, const String& corpo) {
    display.clear();
    display.setFont(ArialMT_Plain_10);
    display.drawString(0, 0, titulo);
    display.drawString(0, 20, corpo);
    display.display();
    
    Serial.println("Mensagem exibida na tela.");
}

void setup() {
    Serial.begin(115200);

    if (!display.init()) {
        Serial.println("Erro: Não foi possível inicializar o display.");
        return;
    }

    mostrarMensagemNaTela("Título legal", "Mucho texto legal :o");
}

void loop() {
    // Nada aqui por enquanto
}
