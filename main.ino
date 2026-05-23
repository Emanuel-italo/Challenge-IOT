#include <Arduino.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <DHTesp.h>

#define PIN_DHT       12
#define PIN_TRIG      25
#define PIN_ECHO      26

// LED RGB (status de saúde do pet)
#define PIN_LED_R     21
#define PIN_LED_G     22
#define PIN_LED_B     23

#define PIN_BUZZER    13

#define PIN_LED_REMOTO 15


// ====================== CONFIGURAÇÕES WiFi / MQTT ======================
const char* SSID         = "Wokwi-GUEST";
const char* PASSWORD     = "";

const char* BROKER_MQTT  = "broker.hivemq.com";
const int   BROKER_PORT  = 1883;
const char* ID_MQTT      = "clyvo_pet_esp32_001";

// Tópicos MQTT (padronizados com namespace 'clyvo/pet')
const char* TOPIC_PUB_TELEMETRIA = "clyvo/pet/telemetria";
const char* TOPIC_SUB_COMANDO    = "clyvo/pet/comando";
const char* TOPIC_PUB_ALERTA     = "clyvo/pet/alerta";