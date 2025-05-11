// notifications.h
#pragma once
#include <Arduino.h>
#include <vector>

extern std::vector<String> notifications;
extern int scrollIndex;
extern const int maxVisibleNotifications;

void saveNotifications();
void loadNotifications();
void addNotification(String notif);
void clearAllNotifications();
