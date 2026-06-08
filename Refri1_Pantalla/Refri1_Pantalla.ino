#include <WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <DHT.h>
#include <SPI.h>
#include <TFT_eSPI.h> // Librería para la pantalla gráfica

// ==========================================
// 1. CONFIGURACIÓN DE RED Y MQTT
// ==========================================
const char* ssid = "NOMBRE_DE_TU_WIFI";       
const char* password = "PASSWORD_DE_TU_WIFI"; 
const char* mqtt_server = "192.168.1.X";      

// ==========================================
// 2. IDENTIFICADOR Y TÓPICOS
// ==========================================
String refriID = "refri1"; 
String topic_temp = "monitoreo/" + refriID + "/temperatura";
String topic_puerta = "monitoreo/" + refriID + "/puerta";
String topic_alerta = "monitoreo/" + refriID + "/alerta";

// ==========================================
// 3. PINES FÍSICOS DEL HARDWARE
// ==========================================
#define DHT_PIN 4       // Pin de datos del sensor DHT22
#define DHT_TYPE DHT22  // Cambia a DHT11 si usas ese modelo
#define DOOR_PIN 5      // Pin del sensor magnético de puerta
#define LED_PIN 2       // LED integrado para alertas

// Inicialización de objetos
DHT dht(DHT_PIN, DHT_TYPE);
TFT_eSPI tft = TFT_eSPI(); 
WiFiClient espClient;
PubSubClient client(espClient);

unsigned long lastMsg = 0;

// ==========================================
// 4. FUNCIÓN PARA RECIBIR ALERTAS
// ==========================================
void callback(char* topic, byte* payload, unsigned int length) {
  String mensaje = "";
  for (int i = 0; i < length; i++) {
    mensaje += (char)payload[i];
  }
  
  // Control del LED de Alerta y actualización visual en pantalla
  if (mensaje == "ON") {
    digitalWrite(LED_PIN, HIGH);
    tft.fillRect(0, 180, 320, 40, TFT_RED);
    tft.setTextColor(TFT_WHITE, TFT_RED);
    tft.drawCentreString("! ALERTA DE TEMPERATURA !", 160, 190, 4);
  } else if (mensaje == "OFF") {
    digitalWrite(LED_PIN, LOW);
    tft.fillRect(0, 180, 320, 40, TFT_BLACK); // Borrar alerta
  }
}

// ==========================================
// 5. FUNCIÓN DE CONEXIÓN WI-FI Y MQTT
// ==========================================
void reconnect() {
  while (!client.connected()) {
    tft.drawString("Conectando MQTT...", 10, 10, 2);
    if (client.connect(refriID.c_str())) {
      client.subscribe(topic_alerta.c_str());
      tft.fillRect(0, 0, 320, 30, TFT_BLACK); // Limpiar mensaje
    } else {
      delay(5000);
    }
  }
}

// ==========================================
// 6. SETUP PRINCIPAL
// ==========================================
void setup() {
  // Configuración de Pines
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);
  
  // Usamos INPUT_PULLUP para el sensor magnético (se activa al separarse del GND)
  pinMode(DOOR_PIN, INPUT_PULLUP); 

  // Iniciar sensor DHT y Pantalla
  dht.begin();
  tft.init();
  tft.setRotation(1); // Orientación horizontal
  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);

  // Pantalla de inicio
  tft.drawCentreString("Iniciando Monitoreo...", 160, 120, 4);

  // Conexión Wi-Fi
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
  }
  
  tft.fillScreen(TFT_BLACK); // Limpiar pantalla al conectar
  tft.drawCentreString("WIFI CONECTADO", 160, 10, 2);

  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);
}

// ==========================================
// 7. BUCLE PRINCIPAL (LOOP)
// ==========================================
void loop() {
  if (!client.connected()) {
    reconnect();
  }
  client.loop();

  unsigned long now = millis();
  
  // Leer y enviar datos cada 10 segundos
  if (now - lastMsg > 10000) {
    lastMsg = now;

    // --- LECTURA FÍSICA DE SENSORES ---
    float tempFisica = dht.readTemperature();
    
    // El sensor magnético da HIGH (1) cuando se abre (imán lejos del GND)
    int puertaFisica = digitalRead(DOOR_PIN); 

    // Validar si el sensor DHT falló
    if (isnan(tempFisica)) {
      tft.setTextColor(TFT_RED, TFT_BLACK);
      tft.drawCentreString("Error leyendo DHT22!", 160, 120, 4);
      return; 
    }

    // --- ACTUALIZACIÓN DE PANTALLA TÁCTIL ---
    // Mostrar Temperatura
    tft.setTextColor(TFT_CYAN, TFT_BLACK);
    tft.drawCentreString("TEMPERATURA ACTUAL", 160, 50, 4);
    
    char tempStr[10];
    sprintf(tempStr, "%.1f C", tempFisica);
    tft.setTextColor(TFT_YELLOW, TFT_BLACK);
    tft.drawCentreString(tempStr, 160, 90, 7); // Tamaño de fuente grande

    // Mostrar estado de la Puerta
    tft.fillRect(0, 140, 320, 30, TFT_BLACK); // Limpiar zona de puerta
    tft.setTextColor(TFT_WHITE, TFT_BLACK);
    if (puertaFisica == 1) {
      tft.setTextColor(TFT_RED, TFT_BLACK);
      tft.drawCentreString("PUERTA ABIERTA", 160, 140, 4);
    } else {
      tft.setTextColor(TFT_GREEN, TFT_BLACK);
      tft.drawCentreString("PUERTA CERRADA", 160, 140, 4);
    }

    // --- CREACIÓN DEL JSON ---
    StaticJsonDocument<200> doc;
    doc["temperatura"] = tempFisica;
    doc["puerta"] = puertaFisica;
    
    char jsonBuffer[512];
    serializeJson(doc, jsonBuffer);

    // --- PUBLICACIÓN MQTT ---
    client.publish(topic_temp.c_str(), jsonBuffer);
  }
}