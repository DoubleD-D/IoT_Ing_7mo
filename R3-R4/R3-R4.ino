#include <WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>

// ==========================================
// 1. CONFIGURACIÓN DE RED Y MQTT
// ==========================================
const char* ssid = "MEGACABLE-2.4G-ADA9";       // Pon tu red aquí
const char* password = "M4kh_41!j0*s33th4n_"; // Pon tu contraseña aquí
const char* mqtt_server = "192.168.100.16";      // Pon la IP de tu Raspberry Pi aquí

// ==========================================
// 2. IDENTIFICADOR DEL REFRIGERADOR
// ==========================================
// Cambiar este aprtado para cada placa refri"n"
String refriID = "refri4"; 

// Generación automática de tópicos basados en el ID
String topic_temp = "monitoreo/" + refriID + "/temperatura";
String topic_puerta = "monitoreo/" + refriID + "/puerta";
String topic_alerta = "monitoreo/" + refriID + "/alerta";

// ==========================================
// 3. PINES Y VARIABLES
// ==========================================
const int LED_PIN = 2; // LED integrado en el ESP32 DevKit

WiFiClient espClient;
PubSubClient client(espClient);

unsigned long lastMsg = 0;
float tempSimulada = 4.0;
int puertaSimulada = 0; // 0 = Cerrada, 1 = Abierta

// ==========================================
// 4. FUNCIÓN PARA RECIBIR ALERTAS (Suscripción)
// ==========================================
void callback(char* topic, byte* payload, unsigned int length) {
  String mensaje = "";
  for (int i = 0; i < length; i++) {
    mensaje += (char)payload[i];
  }
  
  // Si Node-RED manda un "ON", prendemos el LED
  if (mensaje == "ON") {
    digitalWrite(LED_PIN, HIGH);
  } else if (mensaje == "OFF") {
    digitalWrite(LED_PIN, LOW);
  }
}

// ==========================================
// 5. FUNCIÓN DE CONEXIÓN WI-FI Y MQTT
// ==========================================
void reconnect() {
  while (!client.connected()) {
    if (client.connect(refriID.c_str())) {
      // Si se conecta, nos suscribimos al tópico de alertas de este refri
      client.subscribe(topic_alerta.c_str());
    } else {
      delay(5000); // Esperar 5 segundos antes de reintentar
    }
  }
}

// ==========================================
// 6. SETUP PRINCIPAL
// ==========================================
void setup() {
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);

  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
  }

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
  // Enviar datos cada 10 segundos (10000 milisegundos)
  if (now - lastMsg > 10000) {
    lastMsg = now;

    // --- SIMULACIÓN DE DATOS ---
    // Generar temperatura aleatoria entre 2.0 y 10.0 grados
    tempSimulada = random(20, 100) / 10.0; 
    
    // Simular que la puerta se abre de vez en cuando (10% de probabilidad)
    if (random(0, 10) == 1) {
      puertaSimulada = 1;
    } else {
      puertaSimulada = 0;
    }

    // --- CREACIÓN DEL JSON ---
    StaticJsonDocument<200> doc;
    doc["temperatura"] = tempSimulada;
    doc["puerta"] = puertaSimulada;
    
    char jsonBuffer[512];
    serializeJson(doc, jsonBuffer);

    // --- PUBLICACIÓN MQTT ---
    client.publish(topic_temp.c_str(), jsonBuffer);
  }
}