#include <WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ILI9341.h>

// ==========================================
// 1. CONFIGURACIÓN DE RED Y MQTT
// ==========================================
const char* ssid = "MEGACABLE-2.4G-ADA9";
const char* password = "M4kh_41!j0*s33th4n_";
const char* mqtt_server = "192.168.100.16"; 

WiFiClient espClient;
PubSubClient client(espClient);

// ==========================================
// 2. CONFIGURACIÓN DE LA PANTALLA CYD
// ==========================================
#define TFT_CS   15
#define TFT_DC   2
#define TFT_RST  -1  
#define TFT_BL   21  

Adafruit_ILI9341 tft = Adafruit_ILI9341(TFT_CS, TFT_DC, TFT_RST);

// Variables en memoria (Temperaturas y Puertas)
float tempRefri2 = 0.0; String puertaRefri2 = "-";
float tempRefri3 = 0.0; String puertaRefri3 = "-";
float tempRefri4 = 0.0; String puertaRefri4 = "-";

// ==========================================
// 3. FUNCIÓN PARA DIBUJAR LA INTERFAZ BASE
// ==========================================
void dibujarInterfazFija() {
  tft.fillScreen(ILI9341_BLACK); 
  
  tft.setCursor(10, 10);
  tft.setTextColor(ILI9341_YELLOW);
  tft.setTextSize(2); 
  tft.println("MONITOREO FARMACIA");
  tft.drawLine(10, 30, 310, 30, ILI9341_WHITE); 

  tft.setTextColor(ILI9341_CYAN); 
  tft.setTextSize(2); 
  
  // Etiquetas fijas
  tft.setCursor(10, 60);  tft.print("R2:");
  tft.setCursor(10, 120); tft.print("R3:");
  tft.setCursor(10, 180); tft.print("R4:");
}

// ==========================================
// 4. FUNCIÓN PARA ACTUALIZAR NÚMEROS Y PUERTAS
// ==========================================
void actualizarDatos() {
  tft.setTextSize(2); 
  
  // --- REFRI 2 ---
  // Color Temperatura
  if(tempRefri2 > 8.0) tft.setTextColor(ILI9341_RED, ILI9341_BLACK);
  else tft.setTextColor(ILI9341_WHITE, ILI9341_BLACK);
  tft.setCursor(55, 60);
  tft.print(tempRefri2, 1); tft.print("C  ");

  // Color Puerta
  if(puertaRefri2 == "Abierta" || puertaRefri2 == "1") tft.setTextColor(ILI9341_RED, ILI9341_BLACK);
  else tft.setTextColor(ILI9341_GREEN, ILI9341_BLACK);
  tft.setCursor(150, 60);
  tft.print("["); tft.print(puertaRefri2); tft.print("]   ");


  // --- REFRI 3 ---
  if(tempRefri3 > 8.0) tft.setTextColor(ILI9341_RED, ILI9341_BLACK);
  else tft.setTextColor(ILI9341_WHITE, ILI9341_BLACK);
  tft.setCursor(55, 120);
  tft.print(tempRefri3, 1); tft.print("C  ");

  if(puertaRefri3 == "Abierta" || puertaRefri3 == "1") tft.setTextColor(ILI9341_RED, ILI9341_BLACK);
  else tft.setTextColor(ILI9341_GREEN, ILI9341_BLACK);
  tft.setCursor(150, 120);
  tft.print("["); tft.print(puertaRefri3); tft.print("]   ");


  // --- REFRI 4 ---
  if(tempRefri4 > 8.0) tft.setTextColor(ILI9341_RED, ILI9341_BLACK);
  else tft.setTextColor(ILI9341_WHITE, ILI9341_BLACK);
  tft.setCursor(55, 180);
  tft.print(tempRefri4, 1); tft.print("C  ");

  if(puertaRefri4 == "Abierta" || puertaRefri4 == "1") tft.setTextColor(ILI9341_RED, ILI9341_BLACK);
  else tft.setTextColor(ILI9341_GREEN, ILI9341_BLACK);
  tft.setCursor(150, 180);
  tft.print("["); tft.print(puertaRefri4); tft.print("]   ");
}

// ==========================================
// 5. FUNCIÓN PARA RECIBIR LOS DATOS MQTT
// ==========================================
void callback(char* topic, byte* payload, unsigned int length) {
  String jsonPayload = "";
  for (int i = 0; i < length; i++) {
    jsonPayload += (char)payload[i];
  }

  StaticJsonDocument<300> doc;
  DeserializationError error = deserializeJson(doc, jsonPayload);
  
  if (!error) {
    String topicStr = String(topic);
    
    // Extraemos datos (si no vienen, mantienen su valor anterior)
    float tempRx = doc["temperatura"] | -99.0;
    String puertaRx = doc["puerta"] | ""; 

    if (topicStr.indexOf("refri2") > 0) {
      if(tempRx != -99.0) tempRefri2 = tempRx;
      if(puertaRx != "") puertaRefri2 = puertaRx;
    } 
    else if (topicStr.indexOf("refri3") > 0) {
      if(tempRx != -99.0) tempRefri3 = tempRx;
      if(puertaRx != "") puertaRefri3 = puertaRx;
    } 
    else if (topicStr.indexOf("refri4") > 0) {
      if(tempRx != -99.0) tempRefri4 = tempRx;
      if(puertaRx != "") puertaRefri4 = puertaRx;
    }
    
    actualizarDatos();
  }
}

// ==========================================
// 6. FUNCIÓN DE CONEXIÓN MQTT
// ==========================================
void reconnect() {
  while (!client.connected()) {
    if (client.connect("MonitorCentralCYD")) {
      client.subscribe("monitoreo/+/temperatura"); // Ajusta el topic si es necesario
      dibujarInterfazFija(); 
      actualizarDatos();
    } else {
      delay(5000);
    }
  }
}

// ==========================================
// 7. SETUP PRINCIPAL
// ==========================================
void setup() {
  Serial.begin(115200);

  pinMode(TFT_BL, OUTPUT);
  digitalWrite(TFT_BL, HIGH);

  SPI.begin(14, 12, 13, 15);

  tft.begin();
  
  // Rotación 3: Horizontal con el conector USB a la derecha (como en tu foto)
  tft.setRotation(3); 
  
  // Borrado profundo forzado al arrancar para eliminar texto fantasma
  tft.fillScreen(ILI9341_BLACK);
  
  tft.setCursor(10, 100);
  tft.setTextColor(ILI9341_WHITE);
  tft.setTextSize(2);
  tft.println("Conectando WiFi...");

  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
  }

  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);
}

// ==========================================
// 8. LOOP
// ==========================================
void loop() {
  if (!client.connected()) {
    reconnect();
  }
  client.loop();
}