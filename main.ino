#include <Arduino.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <DHTesp.h>

#define PIN_DHT 12
#define PIN_TRIG 25
#define PIN_ECHO 26
#define PIN_LED_R 21
#define PIN_LED_G 22
#define PIN_LED_B 23
#define PIN_BUZZER 13
#define PIN_LED_REMOTO 15

#define INTERVALO_PUBLICACAO 3000 

#define STATUS_VERDE 0
#define STATUS_AMARELO 1
#define STATUS_VERMELHO 2

const char* SSID = "Wokwi-GUEST";
const char* PASSWORD = "";
const char* BROKER_MQTT = "broker.hivemq.com";
const int BROKER_PORT = 1883;
const char* ID_MQTT = "clyvo_pet_esp32_001";
const char* TOPIC_PUB_TELEMETRIA = "clyvo/pet/telemetria";
const char* TOPIC_SUB_COMANDO = "clyvo/pet/comando";
const char* TOPIC_PUB_ALERTA = "clyvo/pet/alerta";

const float TEMP_MIN_SAUDAVEL = 37.5;
const float TEMP_MAX_SAUDAVEL = 39.2;
const float TEMP_LIMITE_ALERTA_INF = 36.5;
const float TEMP_LIMITE_ALERTA_SUP = 40.0;

const int DISTANCIA_PROXIMA_CM = 30;
const int LIMITE_INATIVIDADE = 10;

WiFiClient espClient;
PubSubClient MQTT(espClient);
DHTesp dht;

unsigned long ultimaPublicacao = 0;
int ciclosInativos = 0;
float ultimaDistancia = -1;
bool ledRemotoEstado = false;

void setup() {
  Serial.begin(115200);

  pinMode(PIN_LED_R, OUTPUT);
  pinMode(PIN_LED_G, OUTPUT);
  pinMode(PIN_LED_B, OUTPUT);
  pinMode(PIN_BUZZER, OUTPUT);
  pinMode(PIN_LED_REMOTO, OUTPUT);
  pinMode(PIN_TRIG, OUTPUT);
  pinMode(PIN_ECHO, INPUT);

  dht.setup(PIN_DHT, DHTesp::DHT22);

  digitalWrite(PIN_LED_REMOTO, LOW);
  atualizarLedRGB(STATUS_VERDE);

  initWiFi();
  initMQTT();
  delay(1000);
}

void loop() {
  verificaConexoes();
  MQTT.loop();

  if (millis() - ultimaPublicacao >= INTERVALO_PUBLICACAO) {
    ultimaPublicacao = millis();

    TempAndHumidity dados = dht.getTempAndHumidity();
    float temperatura = dados.temperature;
    float umidade = dados.humidity;
    float distancia = lerDistancia();

    if (isnan(temperatura) || isnan(umidade)) {
      return;
    }

    if (ultimaDistancia >= 0 && fabs(distancia - ultimaDistancia) < 2.0) {
      ciclosInativos++;
    } else {
      ciclosInativos = 0;
    }
    ultimaDistancia = distancia;

    int status = calcularScoreRisco(temperatura, distancia);

    atualizarLedRGB(status);
    tocarAlerta(status);
    publicarTelemetria(temperatura, umidade, distancia, status);
  }
}

void initWiFi() {
  WiFi.begin(SSID, PASSWORD);
  while (WiFi.status() != WL_CONNECTED) {
    delay(200);
  }
}

void initMQTT() {
  MQTT.setServer(BROKER_MQTT, BROKER_PORT);
  MQTT.setCallback(callbackMQTT);
}

void reconectaMQTT() {
  while (!MQTT.connected()) {
    if (MQTT.connect(ID_MQTT)) {
      MQTT.subscribe(TOPIC_SUB_COMANDO);
    } else {
      delay(2000);
    }
  }
}

void verificaConexoes() {
  if (WiFi.status() != WL_CONNECTED) initWiFi();
  if (!MQTT.connected()) reconectaMQTT();
}

float lerDistancia() {
  digitalWrite(PIN_TRIG, LOW);
  delayMicroseconds(2);
  digitalWrite(PIN_TRIG, HIGH);
  delayMicroseconds(10);
  digitalWrite(PIN_TRIG, LOW);

  long duracao = pulseIn(PIN_ECHO, HIGH, 30000);
  if (duracao == 0) return 400.0; 
  return duracao * 0.0343 / 2.0;
}

int calcularScoreRisco(float temp, float distancia) {
  if (temp < TEMP_LIMITE_ALERTA_INF || temp > TEMP_LIMITE_ALERTA_SUP) {
    return STATUS_VERMELHO;
  }

  bool tempLimitrofe = (temp < TEMP_MIN_SAUDAVEL || temp > TEMP_MAX_SAUDAVEL);
  bool inativo = (ciclosInativos >= LIMITE_INATIVIDADE);

  if (tempLimitrofe || inativo) {
    return STATUS_AMARELO;
  }

  return STATUS_VERDE;
}

void atualizarLedRGB(int status) {
  digitalWrite(PIN_LED_R, LOW);
  digitalWrite(PIN_LED_G, LOW);
  digitalWrite(PIN_LED_B, LOW);

  if (status == STATUS_VERDE) {
    digitalWrite(PIN_LED_G, HIGH);
  } else if (status == STATUS_AMARELO) {
    digitalWrite(PIN_LED_R, HIGH);
    digitalWrite(PIN_LED_G, HIGH);
  } else if (status == STATUS_VERMELHO) {
    digitalWrite(PIN_LED_R, HIGH);
  }
}

void tocarAlerta(int status) {
  if (status == STATUS_VERMELHO) {
    tone(PIN_BUZZER, 1500, 200);
    delay(250);
    tone(PIN_BUZZER, 1500, 200);
  } else {
    noTone(PIN_BUZZER);
  }
}

void publicarTelemetria(float temp, float umid, float dist, int status) {
  JsonDocument doc;
  doc["pet_id"] = "PET001";
  doc["nome"] = "Rex";
  doc["temperatura"] = temp;
  doc["umidade"] = umid;
  doc["distancia_cm"] = dist;
  doc["atividade"] = (dist < DISTANCIA_PROXIMA_CM) ? "repouso" : "ativo";
  doc["status_saude"] = (status == STATUS_VERDE) ? "verde" : (status == STATUS_AMARELO ? "amarelo" : "vermelho");
  doc["led_remoto"] = ledRemotoEstado ? "on" : "off";
  doc["timestamp"] = millis();

  char buffer[256];
  serializeJson(doc, buffer);
  MQTT.publish(TOPIC_PUB_TELEMETRIA, buffer);

  if (status == STATUS_VERMELHO) {
    publicarAlerta("Alerta critico: temperatura fora da faixa segura!", status);
  }
}

void publicarAlerta(const char* mensagem, int status) {
  JsonDocument doc;
  doc["pet_id"] = "PET001";
  doc["nivel"] = (status == STATUS_VERMELHO) ? "critico" : "atencao";
  doc["mensagem"] = mensagem;
  doc["timestamp"] = millis();

  char buffer[200];
  serializeJson(doc, buffer);
  MQTT.publish(TOPIC_PUB_ALERTA, buffer);
}

void callbackMQTT(char* topic, byte* payload, unsigned int length) {
  String msg;
  for (unsigned int i = 0; i < length; i++) msg += (char)payload[i];

  JsonDocument json;
  DeserializationError err = deserializeJson(json, msg);
  if (err) {
    return;
  }

  if (json["led"].is<int>()) {
    int valor = json["led"];
    ledRemotoEstado = (valor == 1);
    digitalWrite(PIN_LED_REMOTO, ledRemotoEstado ? HIGH : LOW);
  }

  if (json["alerta_medicacao"].is<bool>() && json["alerta_medicacao"]) {
    for (int i = 0; i < 3; i++) {
      tone(PIN_BUZZER, 1000, 200);
      delay(300);
    }
    noTone(PIN_BUZZER);
  }
}