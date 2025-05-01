// button_handler.cpp
#include "button_handler.h"
#include "notifications.h"
#include "display_manager.h"

#define BTN_PIN 0
unsigned long buttonPressedTime = 0, buttonReleasedTime = 0;
bool isButtonPressed = false, longPressDetected = false;
unsigned int buttonPressedType = 0;
unsigned long lastClickTime = 0;
bool waitingForDoubleClick = false;
const unsigned long doubleClickThreshold = 400;
extern int currentMenu;
extern int current_FrameCount;
extern int menuAmount;
extern FrameCallback menus[];
extern FrameCallback submenuNotifications[];
extern OverlayCallback overlays[];

void initButton() {
  pinMode(BTN_PIN, INPUT_PULLUP);
}

void handleButtonLogic() {
  int buttonState = digitalRead(BTN_PIN);
  unsigned long currentTime = millis();

  // Verifica o início do pressionamento do botão
  if (buttonState == LOW && !isButtonPressed) {
    isButtonPressed = true;
    buttonPressedTime = currentTime;
    longPressDetected = false;
  }

  // Verifica o momento de liberação do botão
  if (buttonState == HIGH && isButtonPressed) {
    isButtonPressed = false;
    buttonReleasedTime = currentTime;
    unsigned long pressDuration = buttonReleasedTime - buttonPressedTime;

    if (pressDuration >= 1500) { // Long press
      buttonPressedType = 3;
      longPressDetected = true;
      waitingForDoubleClick = false; // Cancela o clique duplo
    } else {
      if (waitingForDoubleClick && (currentTime - lastClickTime <= doubleClickThreshold)) {
        // Clique duplo detectado
        buttonPressedType = 2;
        waitingForDoubleClick = false;
      } else {
        // Esperando o segundo clique para um possível clique duplo
        waitingForDoubleClick = true;
        lastClickTime = currentTime;
      }
    }
  }

  // Se não houve clique duplo, e o tempo se passou
  if (waitingForDoubleClick && (currentTime - lastClickTime > doubleClickThreshold)) {
    // Clique simples detectado
    buttonPressedType = 1;
    waitingForDoubleClick = false;
  }

  // Lógica para clique simples
  if (buttonPressedType == 1){
    Serial.println("Clique simples detectado");
    if (currentMenu == 0) {
      current_FrameCount++;
      if (current_FrameCount >= menuAmount) {
        current_FrameCount = 0;
      }
      ui.switchToFrame(current_FrameCount);
    } else if (currentMenu == 1) {
      // Lógica de scroll nas notificações
      if ((scrollIndex + 1) < notifications.size()) {
        scrollIndex++;
      } else {
        scrollIndex = 0;
      }
      ui.setFrames(&submenuNotifications[0], 1);
    }
    buttonPressedType = NULL;
  }

  // Lógica para clique duplo
  if (buttonPressedType == 2) {
    Serial.println("Clique duplo detectado");
    // Volta para o menu principal
    ui.setFrames(menus, menuAmount);
    current_FrameCount = 0;
    currentMenu = 0;
    buttonPressedType = NULL;
  }

  // Lógica para clique longo
  if (buttonPressedType == 3) {
    Serial.println("Clique longo detectado");
    if (currentMenu == 0) {
      if (current_FrameCount == 1) {
        // Vai para o submenu de notificações
        ui.setFrames(&submenuNotifications[0], 1);
        current_FrameCount = 0;
        currentMenu = 1;  // Atualiza para submenu de notificações
      }
    } else if (currentMenu == 1) {
      if (notifications.size() > 0) {
        // Exclui a notificação selecionada
        notifications.erase(notifications.begin() + scrollIndex);
        scrollIndex = 0;
        saveNotifications();
        ui.setFrames(&submenuNotifications[0], 1);
      } else {
        Serial.println("Lista de notificações vazia!");
      }
    }
    buttonPressedType = NULL;
  }
}