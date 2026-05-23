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