// main.ino
#include "display_manager.h"
#include "notifications.h"
#include "button_handler.h"
#include "lora_notification_reciver.h"
#include "status_handler.h"

#define Vext 21

int current_FrameCount = 0;
int currentMenu = 0;
int menuAmount = 3;
long long timestamp = 0;

SSD1306Wire myDisplay(0x3c, 500000, SDA_OLED, SCL_OLED, GEOMETRY_128_64, RST_OLED);

void VextON() {
  pinMode(Vext, OUTPUT);
  digitalWrite(Vext, LOW);
}

void setup() {
  Serial.begin(115200);
  VextON();
  delay(100);

  setupLoRa();

  initButton();
  initDisplay();
  loadNotifications();

  resetInactivityTimer();
}

void loop() {
  relogio();
  loopLoRa();
  handleButtonLogic();
  updateUI();

  if (millis() - lastActivity >= 20000UL) {
    enterDeepSleep();
  }
}
