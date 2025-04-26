#include <Wire.h>
#include <vector>
#include "HT_SSD1306Wire.h"
#include "HT_DisplayUi.h"

#define BTN_PIN 0  // Botão PROG do ESP32 (GPIO0)
#define Vext 21    // Defina seu pino correto de Vext aqui!

static SSD1306Wire display(0x3c, 500000, SDA_OLED, SCL_OLED, GEOMETRY_128_64, RST_OLED);
DisplayUi ui(&display);

int wifiSignalStrength = 3;
int currentFrame = 0; 
bool inSubMenu = false; 

const uint8_t activeSymbol[] = {
  0x3C, 0x7E, 0xFF, 0xFF, 0xFF, 0xFF, 0x7E, 0x3C
};

const uint8_t inactiveSymbol[] = {
  0x00, 0x3C, 0x42, 0x81, 0x81, 0x81, 0x42, 0x3C
};

// Lista dinâmica de notificações
std::vector<String> notifications = {
  "Novo Email",
  "Atualização App",
  "Mensagem Recebida",
  "Lembrete de Reunião",
  "Notificação de Sistema"
};

int scrollIndex = 0;
unsigned long lastClickTime = 0;
bool waitingSecondClick = false;
const unsigned long doubleClickThreshold = 300;

const int maxVisibleNotifications = 4; // Número de notificações visíveis por tela

// Overlay (cabeçalho)
void headerOverlay(ScreenDisplay *display, DisplayUiState* state) {
  if (inSubMenu) return; // Não mostra cabeçalho no submenu

  display->setTextAlignment(TEXT_ALIGN_LEFT);
  display->setFont(ArialMT_Plain_10);

  for (int i = 0; i < 3; i++) {
    if (i < wifiSignalStrength) {
      display->fillRect(2 + i * 4, 10 - i * 3, 3, i * 3 + 2);
    } else {
      display->drawRect(2 + i * 4, 10 - i * 3, 3, i * 3 + 2);
    }
  }

  display->setTextAlignment(TEXT_ALIGN_CENTER);
  display->drawString(64, 0, "(!)  " + String(notifications.size()));

  display->setTextAlignment(TEXT_ALIGN_RIGHT);
  display->drawString(128, 0, String(millis() / 1000) + "s");
}

// Tela principal
void drawMainFrame(ScreenDisplay *display, DisplayUiState* state, int16_t x, int16_t y) {
  display->setTextAlignment(TEXT_ALIGN_CENTER);
  display->setFont(ArialMT_Plain_16);
  display->drawString(x + 64, y + 32, "Boa noite!");
}

// Tela de notificações
void drawNotificationsFrame(ScreenDisplay *display, DisplayUiState* state, int16_t x, int16_t y) {
  display->setTextAlignment(TEXT_ALIGN_CENTER);
  display->setFont(ArialMT_Plain_16);
  display->drawString(x + 64, y + 32, String(notifications.size()) + " Notificações");
}

// Tela de lista de notificações com scroll
void drawNotificationsList(ScreenDisplay *display, DisplayUiState* state, int16_t x, int16_t y) {
  display->setTextAlignment(TEXT_ALIGN_LEFT);
  display->setFont(ArialMT_Plain_10);

  int start = max(0, scrollIndex - (maxVisibleNotifications / 2));
  int end = min(start + maxVisibleNotifications, (int)notifications.size());

  if (end - start < maxVisibleNotifications && start > 0) {
    start = max(0, end - maxVisibleNotifications);
  }

  for (int i = start; i < end; i++) {
    String prefix = (i == scrollIndex) ? "> " : "  ";
    display->drawString(x + 0, y + 10 + (i - start) * 12, prefix + notifications[i]);
  }
}

// Adiciona uma nova notificação
void addNotification(String notif) {
  notifications.push_back(notif);
}

// Arrays de frames
FrameCallback frames[] = { drawMainFrame, drawNotificationsFrame };
int frameCount = 2;

FrameCallback frames_notifications[] = { drawNotificationsList };
int framesNotificationsCount = 1;

OverlayCallback overlays[] = { headerOverlay };
int overlaysCount = 1;

void VextON(void) {
  pinMode(Vext, OUTPUT);
  digitalWrite(Vext, LOW);
}

void VextOFF(void) {
  pinMode(Vext, OUTPUT);
  digitalWrite(Vext, HIGH);
}

void setup() {
  Serial.begin(115200);
  VextON();
  delay(100);

  pinMode(BTN_PIN, INPUT_PULLUP); // Botão com pull-up

  ui.setTargetFPS(60);
  ui.disableAutoTransition();
  ui.setActiveSymbol(activeSymbol);
  ui.setInactiveSymbol(inactiveSymbol);
  ui.setIndicatorPosition(BOTTOM);
  ui.setIndicatorDirection(LEFT_RIGHT);
  ui.setFrameAnimation(SLIDE_LEFT);
  ui.setFrames(frames, frameCount);
  ui.setOverlays(overlays, overlaysCount);

  ui.init();
}

void loop() {
  static unsigned long pressStartTime = 0;
  static bool longPressDetected = false;

  int buttonState = digitalRead(BTN_PIN);

  if (buttonState == LOW) {
    // Botão pressionado
    if (pressStartTime == 0) {
      pressStartTime = millis();
      longPressDetected = false;
      Serial.println("Botão pressionado - Iniciando contagem do pressStartTime");
    } else if (!longPressDetected && millis() - pressStartTime > 2000) {
      longPressDetected = true;
      Serial.println("Long press detectado!");

      // Long press acionado
      if (inSubMenu && notifications.size() > 0) {
        // Long press no submenu: deletar notificação
        Serial.printf("Deletando notificação: %s\n", notifications[scrollIndex].c_str());
        notifications.erase(notifications.begin() + scrollIndex);

        if (scrollIndex >= notifications.size()) {
          scrollIndex = max(0, (int)notifications.size() - 1);
        }
      } else if (!inSubMenu) {
        // Long press fora do submenu: entrar no submenu
        Serial.println("Long press fora do submenu - entrando no submenu de notificações");
        inSubMenu = true;
        scrollIndex = 0;
        ui.setFrames(frames_notifications, framesNotificationsCount);
        ui.disableAllIndicators();
        ui.switchToFrame(0);
      }

      // Garantir que a próxima ação será registrada corretamente
      pressStartTime = 0;
    }
  } else {
    // Botão não pressionado
    if (pressStartTime != 0 && !longPressDetected) {
      unsigned long pressDuration = millis() - pressStartTime;
      Serial.printf("Botão solto após %lu ms\n", pressDuration);

      if (waitingSecondClick && (millis() - lastClickTime < doubleClickThreshold)) {
        // Duplo clique: voltar ao menu inicial
        Serial.println("Duplo clique - voltando ao menu principal");
        inSubMenu = false;
        ui.setFrames(frames, frameCount);
        ui.setActiveSymbol(activeSymbol);
        ui.setInactiveSymbol(inactiveSymbol);
        ui.enableAllIndicators();
        ui.switchToFrame(0);
        waitingSecondClick = false;
      } else if (inSubMenu) {
        // Clique simples no submenu: scroll
        scrollIndex = (scrollIndex + 1) % notifications.size();
        Serial.printf("Scroll para notificação %d\n", scrollIndex);
        waitingSecondClick = true;
        lastClickTime = millis();
      } else {
        // Clique simples fora do submenu: trocar tela
        currentFrame = (currentFrame + 1) % frameCount;
        ui.switchToFrame(currentFrame);
      }

      // Resetar o tempo de pressionamento
      pressStartTime = 0;
    }
  }

  // Verificar o tempo limite para o clique duplo
  if (waitingSecondClick && millis() - lastClickTime > doubleClickThreshold) {
    waitingSecondClick = false;
  }

  // Atualizar a UI
  int remainingTimeBudget = ui.update();
  if (remainingTimeBudget > 0) {
    delay(remainingTimeBudget);
  }
}