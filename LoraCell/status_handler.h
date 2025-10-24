// status_handler.h
#include <Arduino.h>

extern unsigned long lastActivity;    

void recebeData(String data);

String obterHorario();

void relogio();

void enterDeepSleep();

void resetInactivityTimer(); 

void requisitarHorario();

void requisitarPingTower();

void requisitarNotificacoes();
