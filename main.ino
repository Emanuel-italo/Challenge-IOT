#include <Arduino.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <DHTesp.h>

#define PIN_DHT       12
#define PIN_TRIG      25
#define PIN_ECHO      26