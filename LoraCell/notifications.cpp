// notifications.cpp
#include "notifications.h"
#include <Preferences.h>

Preferences preferences;
std::vector<String> notifications;
int scrollIndex = 0;
const int maxVisibleNotifications = 4;

void saveNotifications() {
  preferences.begin("notif", false);
  String serialized = "";
  for (size_t i = 0; i < notifications.size(); ++i) {
    serialized += notifications[i];
    if (i < notifications.size() - 1) serialized += "\n";
  }
  preferences.putString("list", serialized);
  preferences.end();
}

void loadNotifications() {
  preferences.begin("notif", true);
  String serialized = preferences.getString("list", "");
  preferences.end();
  notifications.clear();

  if (serialized.length() == 0) return;

  int start = 0;
  while (true) {
    int idx = serialized.indexOf('\n', start);
    if (idx == -1) {
      notifications.push_back(serialized.substring(start));
      break;
    }
    notifications.push_back(serialized.substring(start, idx));
    start = idx + 1;
  }
}

void addNotification(String notif) {
  notifications.push_back(notif);
  saveNotifications();
}

void clearAllNotifications() {
  if (notifications.empty()) {
    Serial.println("Nenhuma notificação para deletar.");
    return;
  }

  notifications.clear();  // Limpa a lista em memória
  preferences.begin("notif", false);
  preferences.remove("list");  // Remove a chave armazenada no NVS
  preferences.end();
  Serial.println("Todas as notificações foram deletadas!");
}