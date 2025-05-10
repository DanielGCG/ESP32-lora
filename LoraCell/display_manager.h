// display_manager.h
#pragma once
#include <HT_SSD1306Wire.h>
#include <HT_DisplayUi.h>

extern SSD1306Wire myDisplay;
extern DisplayUi ui;
extern void initDisplay();
extern void setMainFrames();
extern void setNotificationListFrame();
extern void updateUI();
extern void setloraSignalStrength(int rssi, int snr);
extern void drawMainFrame(ScreenDisplay*, DisplayUiState*, int16_t, int16_t);
extern void drawNotificationsFrame(ScreenDisplay*, DisplayUiState*, int16_t, int16_t);
extern void drawNotificationsList(ScreenDisplay*, DisplayUiState*, int16_t, int16_t);
extern void headerOverlay(ScreenDisplay*, DisplayUiState*);
