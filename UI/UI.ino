#include <Wire.h>
#include <vector>
#include "HT_SSD1306Wire.h"
#include "HT_DisplayUi.h"
#include <Preferences.h>

#define BTN_PIN 0  // Botão PROG do ESP32 (GPIO0)
#define Vext 21    // Defina seu pino correto de Vext aqui!

static SSD1306Wire display(0x3c, 500000, SDA_OLED, SCL_OLED, GEOMETRY_128_64, RST_OLED);
DisplayUi ui(&display);

int wifiSignalStrength = 3;
bool inSubMenu = false; 

const uint8_t activeSymbol[] = {
  0x3C, 0x7E, 0xFF, 0xFF, 0xFF, 0xFF, 0x7E, 0x3C
};

const uint8_t inactiveSymbol[] = {
  0x00, 0x3C, 0x42, 0x81, 0x81, 0x81, 0x42, 0x3C
};

// Lista dinâmica de notificações
std::vector<String> notifications = {
};

int scrollIndex = 0;

// 0 = main Menu | 1 = Notification List
int currentMenu; 

int current_FrameCount = 0;

const int maxVisibleNotifications = 4; // Número de notificações visíveis por tela
Preferences preferences;

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

void saveNotifications() {
  preferences.begin("notif", false);

  String serialized = "";
  for (size_t i = 0; i < notifications.size(); ++i) {
    serialized += notifications[i];
    if (i < notifications.size() - 1) {
      serialized += "\n"; // separador
    }
  }

  preferences.putString("list", serialized);
  preferences.end();

  Serial.println("Notificações salvas na NVS.");
}

void loadNotifications() {
  preferences.begin("notif", true);
  String serialized = preferences.getString("list", "");
  preferences.end();

  notifications.clear();
  if (serialized.length() > 0) {
    int start = 0;
    while (true) {
      int idx = serialized.indexOf('\n', start);
      if (idx == -1) {
        notifications.push_back(serialized.substring(start));
        break;
      } else {
        notifications.push_back(serialized.substring(start, idx));
        start = idx + 1;
      }
    }
  }

  Serial.println("Notificações carregadas da NVS:");
  for (auto &n : notifications) {
    Serial.println(" - " + n);
  }
}

// Tela principal
void drawMainFrame(ScreenDisplay *display, DisplayUiState* state, int16_t x, int16_t y) {
  display->setTextAlignment(TEXT_ALIGN_CENTER);
  display->setFont(ArialMT_Plain_16);
  display->drawString(x + 64, y + 32, "Boa noite!");
  currentMenu = 0;
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

  currentMenu = 0;
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
  currentMenu = 1;
}

// Adiciona uma nova notificação
void addNotification(String notif) {
  notifications.push_back(notif);
  saveNotifications();
}

// Arrays de menus
FrameCallback menus[] = { drawMainFrame, drawNotificationsFrame };
int menuAmount = 2;

FrameCallback submenuNotifications[] = { drawNotificationsList };
int submenuNotificationAmount = 1;

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

  pinMode(BTN_PIN, INPUT_PULLUP);

  addNotification("Olha a notificação! 1");
  addNotification("Olha a notificação! 2");
  addNotification("Olha a notificação! 3");
  addNotification("Olha a notificação! 4");
  addNotification("Olha a notificação! 5");
  addNotification("Olha a notificação! 6");

  loadNotifications();

  ui.setTargetFPS(60);
  ui.disableAutoTransition();
  ui.setActiveSymbol(activeSymbol);
  ui.setInactiveSymbol(inactiveSymbol);
  ui.setIndicatorPosition(BOTTOM);
  ui.setIndicatorDirection(LEFT_RIGHT);
  ui.setFrameAnimation(SLIDE_LEFT);
  ui.setFrames(menus, menuAmount);
  ui.setOverlays(overlays, overlaysCount);

  ui.init();
}

unsigned long buttonPressedTime = 0;
unsigned long buttonReleasedTime = 0;
bool isButtonPressed = false;
bool longPressDetected = false;

// 0 = none  |  1 = single click  |  2 = double click  |  3 = long click
unsigned int buttonPressedType = 0;

unsigned long lastClickTime = 0;
bool waitingForDoubleClick = false;
const unsigned long doubleClickThreshold = 400; 

void loop() {
  int buttonState = digitalRead(BTN_PIN);
  unsigned long currentTime = millis();

  if (buttonState == LOW && !isButtonPressed) {
    // Botão foi pressionado
    isButtonPressed = true;
    buttonPressedTime = currentTime;
    longPressDetected = false;
  }

  if (buttonState == HIGH && isButtonPressed) {
    // Botão foi liberado
    isButtonPressed = false;
    buttonReleasedTime = currentTime;
    unsigned long pressDuration = buttonReleasedTime - buttonPressedTime;

    if (pressDuration >= 1500) {
      buttonPressedType = 3;
      longPressDetected = true;
      waitingForDoubleClick = false; // cancela qualquer tentativa de clique duplo
    } else {
      if (waitingForDoubleClick && (currentTime - lastClickTime <= doubleClickThreshold)) {
        buttonPressedType = 2;
        waitingForDoubleClick = false;
      } else {
        waitingForDoubleClick = true;
        lastClickTime = currentTime;
      }
    }
  }

  // Se passou o tempo do duplo clique e não houve segundo clique
  if (waitingForDoubleClick && (currentTime - lastClickTime > doubleClickThreshold)) {
    buttonPressedType = 1;
    waitingForDoubleClick = false;
  }

  if (buttonPressedType == 1){
    Serial.println("Clique simples detectado");
    // Se estamos no menu principal navegando entre as opções de submenu
    if (currentMenu == 0){
      current_FrameCount++;
      if (current_FrameCount >= menuAmount) {
        current_FrameCount = 0;
      }
      ui.switchToFrame(current_FrameCount);
      buttonPressedType = NULL;
    }
    // Se estamos no submenu notificações
    else if (currentMenu == 1){
      if ((scrollIndex + 1) < notifications.size()){
        scrollIndex = scrollIndex + 1;
      }
      else {
        scrollIndex = 0;
      }
      // Recarrega com o scroll
      ui.setFrames(&submenuNotifications[0], 1);
      buttonPressedType = NULL;
    }
  }
  if (buttonPressedType == 2){
    Serial.println("Clique duplo detectado");
    // Volta para o menu principal
    ui.setFrames(menus, menuAmount);  // Corrigido
    current_FrameCount = 0;
    currentMenu = 0;
    buttonPressedType = NULL;
  }
  if (buttonPressedType == 3){
    Serial.println("Clique longo detectado");
    if (currentMenu == 0){
      if (current_FrameCount == 1){
        // Vai para o submenu de notificações
        ui.setFrames(&submenuNotifications[0], 1);
        current_FrameCount = 0;
        currentMenu = 1;  // Atualiza para submenu de notificações
        buttonPressedType = NULL;
      }
    }
    else if (currentMenu == 1){
      if (notifications.size() > 0){
        // Exclui a notificação selecionada
        notifications.erase(notifications.begin() + scrollIndex);
        scrollIndex = 0;
        saveNotifications();
        // Atualiza a tela de notificações após exclusão
        ui.setFrames(&submenuNotifications[0], 1);
        buttonPressedType = NULL;
      }
      else {
        Serial.println("Lista de notificações vazia!");
        buttonPressedType = NULL;
      }
    }
  }

  // Voltamos ao valor nulo
  buttonPressedType = NULL;
  ui.update();
}