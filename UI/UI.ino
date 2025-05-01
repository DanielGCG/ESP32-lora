#include "display_manager.h"
#include "notifications.h"
#include "button_handler.h"

#define Vext 21

int current_FrameCount = 0;
int currentMenu = 0;
int menuAmount = 2;

SSD1306Wire myDisplay(0x3c, 500000, SDA_OLED, SCL_OLED, GEOMETRY_128_64, RST_OLED);

void VextON() {
  pinMode(Vext, OUTPUT);
  digitalWrite(Vext, LOW);
}

void setup() {
  Serial.begin(115200);
  VextON();
  delay(100);

  initButton();
  addNotification("Olha a notificação! 1");
  addNotification("Olha a notificação! 2");
  addNotification("Olha a notificação! 3");
  addNotification("Olha a notificação! 4");
  addNotification("Olha a notificação! 5");
  addNotification("Olha a notificação! 6");

  loadNotifications();
  initDisplay();
}

void loop() {
  handleButtonLogic();
  updateUI();
}
