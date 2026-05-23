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


// CONFIGURAÇÕES WiFi / MQTT 
const char* SSID         = "Wokwi-GUEST";
const char* PASSWORD     = "";

const char* BROKER_MQTT  = "broker.hivemq.com";
const int   BROKER_PORT  = 1883;
const char* ID_MQTT      = "clyvo_pet_esp32_001";

// Tópicos MQTT 
const char* TOPIC_PUB_TELEMETRIA = "clyvo/pet/telemetria";
const char* TOPIC_SUB_COMANDO    = "clyvo/pet/comando";
const char* TOPIC_PUB_ALERTA     = "clyvo/pet/alerta";

#define INTERVALO_PUBLICACAO 3000   

// Faixas de temperatura corporal saudáveis (em °C) — referência veterinária
const float TEMP_MIN_SAUDAVEL = 37.5;
const float TEMP_MAX_SAUDAVEL = 39.2;
const float TEMP_LIMITE_ALERTA_INF = 36.5;  // hipotermia
const float TEMP_LIMITE_ALERTA_SUP = 40.0;  // febre alta

const int DISTANCIA_PROXIMA_CM = 30;   // pet próximo do tutor / em repouso confortável
const int LIMITE_INATIVIDADE   = 10;   // ciclos consecutivos sem variação -> sedentarismo

WiFiClient espClient;
PubSubClient MQTT(espClient);
DHTesp dht;

unsigned long ultimaPublicacao = 0;
int ciclosInativos = 0;
float ultimaDistancia = -1;
bool ledRemotoEstado = false;

// Estados de score de risco
enum StatusSaude { VERDE, AMARELO, VERMELHO };


void initWiFi();
void initMQTT();
void callbackMQTT(char* topic, byte* payload, unsigned int length);
void reconectaMQTT();
void verificaConexoes();
float lerDistancia();
StatusSaude calcularScoreRisco(float temp, float distancia);
void atualizarLedRGB(StatusSaude status);
void tocarAlerta(StatusSaude status);
void publicarTelemetria(float temp, float umid, float dist, StatusSaude status);
void publicarAlerta(const char* mensagem, StatusSaude status);



void setup() {
  Serial.begin(115200);
  Serial.println("\n=== CLYVO PET — Wearable IoT iniciando ===");

  // Configura pinos de atuadores
  pinMode(PIN_LED_R, OUTPUT);
  pinMode(PIN_LED_G, OUTPUT);
  pinMode(PIN_LED_B, OUTPUT);
  pinMode(PIN_BUZZER, OUTPUT);
  pinMode(PIN_LED_REMOTO, OUTPUT);

  // Configura pinos do sensor ultrassônico
  pinMode(PIN_TRIG, OUTPUT);
  pinMode(PIN_ECHO, INPUT);

   // Inicializa DHT22
  dht.setup(PIN_DHT, DHTesp::DHT22);

  // Estado inicial dos LEDs
  digitalWrite(PIN_LED_REMOTO, LOW);
  atualizarLedRGB(VERDE);

  // Conecta WiFi e MQTT
  initWiFi();
  initMQTT();

  Serial.println("Sistema CLYVO PET pronto para monitorar.");
  delay(1000);
}

// =====================================================================================
//  LOOP PRINCIPAL
// =====================================================================================
void loop() {
  verificaConexoes();
  MQTT.loop();

  if (millis() - ultimaPublicacao >= INTERVALO_PUBLICACAO) {
    ultimaPublicacao = millis();

    // ---- Leitura dos sensores ----
    TempAndHumidity dados = dht.getTempAndHumidity();
    float temperatura = dados.temperature;
    float umidade     = dados.humidity;
    float distancia   = lerDistancia();

    // Trata leitura inválida
    if (isnan(temperatura) || isnan(umidade)) {
      Serial.println("Falha na leitura do DHT22 — pulando ciclo.");
      return;
    }

    // ---- Detecção de inatividade do pet ----
    if (ultimaDistancia >= 0 && fabs(distancia - ultimaDistancia) < 2.0) {
      ciclosInativos++;
    } else {
      ciclosInativos = 0;
    }
    ultimaDistancia = distancia;

    // ---- Calcula score de risco ----
    StatusSaude status = calcularScoreRisco(temperatura, distancia);

    // ---- Aciona atuadores ----
    atualizarLedRGB(status);
    tocarAlerta(status);

    // ---- Envia telemetria via MQTT ----
    publicarTelemetria(temperatura, umidade, distancia, status);

    // ---- Logs no Serial ----
    Serial.printf("Temp: %.1f°C | Umid: %.1f%% | Dist: %.1fcm | Status: %d | Inativo: %d\n",
                  temperatura, umidade, distancia, status, ciclosInativos);
  }
}

void initWiFi() {
  Serial.print("Conectando ao WiFi: ");
  Serial.println(SSID);
  WiFi.begin(SSID, PASSWORD);
  while (WiFi.status() != WL_CONNECTED) {
    delay(200);
    Serial.print(".");
  }
  Serial.println("\nWiFi conectado!");
  Serial.print("IP: ");
  Serial.println(WiFi.localIP());
}

void initMQTT() {
  MQTT.setServer(BROKER_MQTT, BROKER_PORT);
  MQTT.setCallback(callbackMQTT);
}

void reconectaMQTT() {
  while (!MQTT.connected()) {
    Serial.print("Conectando ao broker MQTT...");
    if (MQTT.connect(ID_MQTT)) {
      Serial.println(" conectado!");
      MQTT.subscribe(TOPIC_SUB_COMANDO);
      Serial.printf("Inscrito em: %s\n", TOPIC_SUB_COMANDO);
    } else {
      Serial.printf(" falha (rc=%d). Tentando em 2s.\n", MQTT.state());
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

  long duracao = pulseIn(PIN_ECHO, HIGH, 30000); // timeout 30ms
  if (duracao == 0) return 400.0; // fora de alcance
  float distancia = duracao * 0.0343 / 2.0;
  return distancia;
}

StatusSaude calcularScoreRisco(float temp, float distancia) {
  // Faixa crítica de temperatura -> VERMELHO
  if (temp < TEMP_LIMITE_ALERTA_INF || temp > TEMP_LIMITE_ALERTA_SUP) {
    return VERMELHO;
  }

bool tempLimitrofe = (temp < TEMP_MIN_SAUDAVEL || temp > TEMP_MAX_SAUDAVEL);
bool inativo = (ciclosInativos >= LIMITE_INATIVIDADE);

  if (tempLimitrofe || inativo) {
    return AMARELO;
  }

    return VERDE;
}

void atualizarLedRGB(StatusSaude status) {
    digitalWrite(PIN_LED_R, LOW);
    digitalWrite(PIN_LED_G, LOW);
    digitalWrite(PIN_LED_B, LOW);

      switch (status) {
    case VERDE:
      digitalWrite(PIN_LED_G, HIGH);
            break;
    case AMARELO:

          digitalWrite(PIN_LED_R, HIGH);
      digitalWrite(PIN_LED_G, HIGH);
      break;
    case VERMELHO:
          digitalWrite(PIN_LED_R, HIGH);
      break;
  }
}

void tocarAlerta(StatusSaude status) {
  if (status == VERMELHO) {
    tone(PIN_BUZZER, 1500, 200);
    delay(250);
    tone(PIN_BUZZER, 1500, 200);
  } else {
    noTone(PIN_BUZZER);
  }
}

void publicarTelemetria(float temp, float umid, float dist, StatusSaude status) {
  JsonDocument doc;
  doc["pet_id"]       = "PET001";
  doc["nome"]         = "Rex";
  doc["temperatura"]  = temp;
  doc["umidade"]      = umid;
  doc["distancia_cm"] = dist;
  doc["atividade"]    = (dist < DISTANCIA_PROXIMA_CM) ? "repouso" : "ativo";
  doc["status_saude"] = (status == VERDE) ? "verde" : (status == AMARELO ? "amarelo" : "vermelho");
  doc["led_remoto"]   = ledRemotoEstado ? "on" : "off";
  doc["timestamp"]    = millis();

char buffer[256];
  serializeJson(doc, buffer);
  MQTT.publish(TOPIC_PUB_TELEMETRIA, buffer);


  if (status == VERMELHO) {
    publicarAlerta("Alerta crítico: temperatura fora da faixa segura!", status);
  }
}

void publicarAlerta(const char* mensagem, StatusSaude status) {
  JsonDocument doc;
  doc["pet_id"]   = "PET001";
  doc["nivel"]    = (status == VERMELHO) ? "critico" : "atencao";
  doc["mensagem"] = mensagem;
  doc["timestamp"] = millis();

  char buffer[200];
  serializeJson(doc, buffer);
  MQTT.publish(TOPIC_PUB_ALERTA, buffer);
  Serial.printf("[ALERTA] %s\n", mensagem);
}

void callbackMQTT(char* topic, byte* payload, unsigned int length) {
  String msg;
  for (unsigned int i = 0; i < length; i++) msg += (char)payload[i];
  Serial.printf("Comando recebido em %s: %s\n", topic, msg.c_str());

   JsonDocument json;
  DeserializationError err = deserializeJson(json, msg);
  if (err) {
    Serial.println("JSON inválido no comando.");
    return;
  }