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
    ui.setFrames(menus, menuAmount);
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
}
