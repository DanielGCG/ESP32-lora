// display_manager.cpp
#include "display_manager.h"
#include "notifications.h"
#include "status_handler.h"

extern SSD1306Wire myDisplay;
DisplayUi ui(&myDisplay);

int loraSignalStrength = 0;

const uint8_t activeSymbol[] = { 0x3C, 0x7E, 0xFF, 0xFF, 0xFF, 0xFF, 0x7E, 0x3C };
const uint8_t inactiveSymbol[] = { 0x00, 0x3C, 0x42, 0x81, 0x81, 0x81, 0x42, 0x3C };

FrameCallback menus[] = { drawMainFrame, drawNotificationsFrame };
FrameCallback submenuNotifications[] = { drawNotificationsList };
OverlayCallback overlays[] = { headerOverlay };

String mensagens_boas_vindas[4] = {
  "Olá!",
  "Bom dia!",
  "Boa tarde!",
  "Boa noite!",
};


unsigned long lastScrollTime = 0;
int scrollOffset = 0;
const int scrollSpeed = 50; // ms entre atualizações

extern int num_mensagem_boas_vindas = 0; // Padrão é apenas "olá!"
extern int currentMenu;
extern int current_FrameCount;


void setloraSignalStrength(int rssi, int snr) {
  int nivelRSSI = 0;
  if (rssi > -70) nivelRSSI = 3;
  else if (rssi > -90) nivelRSSI = 2;
  else if (rssi > -110) nivelRSSI = 1;
  else nivelRSSI = 0;

  int nivelSNR = 0;
  if (snr > 7) nivelSNR = 3;
  else if (snr > 0) nivelSNR = 2;
  else if (snr > -5) nivelSNR = 1;
  else nivelSNR = 0;

  // Usa o menor entre os dois níveis como sinal final
  loraSignalStrength = min(nivelRSSI, nivelSNR);
}

void initDisplay() {
  ui.setTargetFPS(60);
  ui.disableAutoTransition();
  ui.setActiveSymbol(activeSymbol);
  ui.setInactiveSymbol(inactiveSymbol);
  ui.setIndicatorPosition(BOTTOM);
  ui.setIndicatorDirection(LEFT_RIGHT);
  ui.setFrameAnimation(SLIDE_LEFT);
  setMainFrames();
  ui.setOverlays(overlays, 1);
  ui.init();
}

void setMainFrames() {
  ui.setFrames(menus, 2);
  ui.enableIndicator();
}

void setNotificationListFrame() {
  ui.setFrames(submenuNotifications, 1);
  ui.disableIndicator();
}

void updateUI() {
  ui.update();
}

void drawMainFrame(ScreenDisplay *display, DisplayUiState* state, int16_t x, int16_t y) {
  ui.enableIndicator();
  display->setTextAlignment(TEXT_ALIGN_CENTER);
  display->setFont(ArialMT_Plain_16);
  display->drawString(x + 64, y + 32, mensagens_boas_vindas[num_mensagem_boas_vindas]);
}

// Tela de notificações
void drawNotificationsFrame(ScreenDisplay *display, DisplayUiState* state, int16_t x, int16_t y) {
  display->setTextAlignment(TEXT_ALIGN_CENTER);
  display->setFont(ArialMT_Plain_16);

  if (notifications.size() > 0) {
    display->drawString(x + 64, y + 32, String(notifications.size()) + " Notificações");
  } else {
    display->drawString(x + 64, y + 32, "Sem notificações");
  }
}

// Tela de lista de notificações com scroll
void drawNotificationsList(ScreenDisplay *display, DisplayUiState* state, int16_t x, int16_t y) {
  ui.disableIndicator();
  display->setTextAlignment(TEXT_ALIGN_LEFT);
  display->setFont(ArialMT_Plain_10);

  int start = max(0, scrollIndex - (maxVisibleNotifications / 2));
  int end = min(start + maxVisibleNotifications, (int)notifications.size());

  if (end - start < maxVisibleNotifications && start > 0) {
    start = max(0, end - maxVisibleNotifications);
  }

  for (int i = start; i < end; i++) {
    String prefix = (i == scrollIndex) ? "> " : "  ";
    String message = prefix + notifications[i];
    int yOffset = y + 10 + (i - start) * 12;

    if (i == scrollIndex) {
      int16_t textWidth = display->getStringWidth(message);
      if (textWidth > 128) {
        // Atualiza o scroll somente se tempo passou
        if (millis() - lastScrollTime > scrollSpeed) {
          scrollOffset++;
          if (scrollOffset > textWidth) scrollOffset = -128;
          lastScrollTime = millis();
        }
        display->drawString(x - scrollOffset, yOffset, message);
      } else {
        scrollOffset = 0; // Reset se não precisar de scroll
        display->drawString(x, yOffset, message);
      }
    } else {
      display->drawString(x, yOffset, message);
    }
  }
}

void headerOverlay(ScreenDisplay *display, DisplayUiState* state) {
  if (currentMenu != 0) return; // Não mostra cabeçalho no submenu

  display->setTextAlignment(TEXT_ALIGN_LEFT);
  display->setFont(ArialMT_Plain_10);

  for (int i = 0; i < 3; i++) {
    if (i < loraSignalStrength) {
      display->fillRect(2 + i * 4, 10 - i * 3, 3, i * 3 + 2);
    } else {
      display->drawRect(2 + i * 4, 10 - i * 3, 3, i * 3 + 2);
    }
  }

  display->setTextAlignment(TEXT_ALIGN_CENTER);
  display->drawString(64, 0, "(!)  " + String(notifications.size()));

  display->setTextAlignment(TEXT_ALIGN_RIGHT);
  display->drawString(128, 0, obterHorario());
}

