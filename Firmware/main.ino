#include <Arduino.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>

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
const char* TOPIC_PUB_TELEMETRIA = "clyvo/pet/rm561337/telemetria";
const char* TOPIC_SUB_COMANDO = "clyvo/pet/rm561337/comando";
const char* TOPIC_PUB_ALERTA = "clyvo/pet/rm561337/alerta";

WiFiClient espClient;
PubSubClient MQTT(espClient);

unsigned long ultimaPublicacao = 0;
int statusAnterior = -1;

// --- MOTOR FISIOLÓGICO (Sem dados mockados estáticos) ---
bool emAtividade = false;
float tempCorporal = 38.0; // Temperatura de repouso saudável
int bpmAtual = 75;         // Batimentos de repouso
float distanciaTotalMetros = 0.0;
bool ledRemotoEstado = false;

void initWiFi();
void initMQTT();
void callbackMQTT(char* topic, byte* payload, unsigned int length);
void reconectaMQTT();
void verificaConexoes();
void atualizarLedRGB(int status);
void tocarAlerta(int status);
void publicarTelemetria(int status);
void publicarAlerta(const char* mensagem, const char* nivel);

void setup() {
  Serial.begin(115200);

  pinMode(PIN_LED_R, OUTPUT);
  pinMode(PIN_LED_G, OUTPUT);
  pinMode(PIN_LED_B, OUTPUT);
  pinMode(PIN_BUZZER, OUTPUT);
  pinMode(PIN_LED_REMOTO, OUTPUT);

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

    // === LÓGICA DINÂMICA DO METABOLISMO ===
    if (emAtividade) {
      // Pet se exercitando: Sinais vitais sobem gradativamente
      bpmAtual += random(4, 10);
      if (bpmAtual > 165) bpmAtual = 165; // Teto cardíaco

      tempCorporal += random(1, 3) / 10.0; // Sobe de 0.1 a 0.2 graus
      if (tempCorporal > 40.5) tempCorporal = 40.5; // Teto de temperatura

      distanciaTotalMetros += random(30, 80) / 10.0; // Avança de 3 a 8 metros
    } else {
      // Pet em repouso: Sinais vitais caem e estabilizam
      bpmAtual -= random(3, 8);
      if (bpmAtual < 75) bpmAtual = 75 + random(-2, 3); // Oscilação natural de repouso

      tempCorporal -= random(1, 2) / 10.0; // Resfria 0.1
      if (tempCorporal < 38.0) tempCorporal = 38.0 + (random(0, 2) / 10.0); // Normal
    }

    // === DEFINIÇÃO DE STATUS DE SAÚDE ===
    int status = STATUS_VERDE;
    if (tempCorporal >= 39.5 || bpmAtual >= 150) {
      status = STATUS_VERMELHO; // Crítico: Febre ou Taquicardia
    } else if (emAtividade) {
      status = STATUS_AMARELO;  // Amarelo: Agitado / Se exercitando
    }

    atualizarLedRGB(status);
    tocarAlerta(status);

    Serial.printf("Atividade: %s | Temp: %.1fC | BPM: %d | Dist: %.1fm | Status: %d\n", 
                  emAtividade ? "SIM" : "NAO", tempCorporal, bpmAtual, distanciaTotalMetros, status);

    publicarTelemetria(status);

    // Gestão inteligente de alertas
    if (status != statusAnterior) {
      if (status == STATUS_VERDE) {
        publicarAlerta("Os sinais vitais do pet normalizaram. Em repouso.", "info");
      } else if (status == STATUS_AMARELO) {
        publicarAlerta("Atividade detectada! O pet esta em movimento e os sinais estao subindo.", "atencao");
      } else if (status == STATUS_VERMELHO) {
        publicarAlerta("ALERTA CRITICO: Temperatura ou BPM acima do limite seguro. Interrompa a atividade!", "critico");
      }
      statusAnterior = status;
    }
  }
}

void initWiFi() {
  WiFi.begin(SSID, PASSWORD);
  while (WiFi.status() != WL_CONNECTED) { delay(200); }
}

void initMQTT() {
  MQTT.setServer(BROKER_MQTT, BROKER_PORT);
  MQTT.setCallback(callbackMQTT);
}

void reconectaMQTT() {
  while (!MQTT.connected()) {
    if (MQTT.connect(ID_MQTT)) {
      MQTT.subscribe(TOPIC_SUB_COMANDO);
    } else { delay(2000); }
  }
}

void verificaConexoes() {
  if (WiFi.status() != WL_CONNECTED) initWiFi();
  if (!MQTT.connected()) reconectaMQTT();
}

void atualizarLedRGB(int status) {
  digitalWrite(PIN_LED_R, LOW); digitalWrite(PIN_LED_G, LOW); digitalWrite(PIN_LED_B, LOW);
  if (status == STATUS_VERDE) digitalWrite(PIN_LED_G, HIGH);
  else if (status == STATUS_AMARELO) { digitalWrite(PIN_LED_R, HIGH); digitalWrite(PIN_LED_G, HIGH); }
  else if (status == STATUS_VERMELHO) digitalWrite(PIN_LED_R, HIGH);
}

void tocarAlerta(int status) {
  if (status == STATUS_VERMELHO) {
    tone(PIN_BUZZER, 1500, 200); delay(250); tone(PIN_BUZZER, 1500, 200);
  } else {
    noTone(PIN_BUZZER);
  }
}

void publicarTelemetria(int status) {
  JsonDocument doc;
  doc["temperatura"] = tempCorporal;
  doc["bpm"] = bpmAtual;
  doc["distancia_m"] = distanciaTotalMetros;
  doc["em_atividade"] = emAtividade;
  doc["status_saude"] = (status == STATUS_VERDE) ? "verde" : (status == STATUS_AMARELO ? "amarelo" : "vermelho");
  
  char buffer[300];
  serializeJson(doc, buffer);
  MQTT.publish(TOPIC_PUB_TELEMETRIA, buffer);
}

void publicarAlerta(const char* mensagem, const char* nivel) {
  JsonDocument doc;
  doc["nivel"] = nivel;
  doc["mensagem"] = mensagem;
  
  char buffer[200];
  serializeJson(doc, buffer);
  MQTT.publish(TOPIC_PUB_ALERTA, buffer);
}

void callbackMQTT(char* topic, byte* payload, unsigned int length) {
  String msg;
  for (unsigned int i = 0; i < length; i++) msg += (char)payload[i];
  
  JsonDocument json;
  DeserializationError err = deserializeJson(json, msg);
  if (err) return;

  // Gatilho Principal vindo do App do Tutor
  if (json["atividade"].is<bool>()) {
    emAtividade = json["atividade"];
    Serial.println(emAtividade ? ">>> INICIO DE ATIVIDADE <<<" : ">>> FIM DE ATIVIDADE <<<");
  }

  // Comandos administrativos
  if (json["led"].is<int>()) {
    ledRemotoEstado = (json["led"] == 1);
    digitalWrite(PIN_LED_REMOTO, ledRemotoEstado ? HIGH : LOW);
  }
} 