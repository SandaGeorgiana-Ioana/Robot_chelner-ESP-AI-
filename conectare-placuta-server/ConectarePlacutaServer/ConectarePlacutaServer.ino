#include <WiFi.h>
#include <PubSubClient.h>

const char* WIFI_SSID     = "iPhone - Sanda";
const char* WIFI_PASSWORD = "georgi2002";
const char* MQTT_BROKER   = "172.20.10.2";
const int   MQTT_PORT     = 1883;

const char* TOPIC_COMENZI    = "restaurant/comenzi";
const char* TOPIC_RASPUNSURI = "restaurant/raspunsuri";

WiFiClient   wifiClient;
PubSubClient mqtt(wifiClient);

void onMesajPrimit(char* topic, byte* payload, unsigned int length) {
  String mesaj = "";
  for (unsigned int i = 0; i < length; i++) {
    mesaj += (char)payload[i];
  }
  Serial.println("──────────────────────");
  Serial.print("Comanda primita: ");
  Serial.println(mesaj);
  Serial.println("──────────────────────");

  mqtt.publish(TOPIC_RASPUNSURI, "am plecat");
}

void conectareWiFi() {
  Serial.print("Conectare Wi-Fi");
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println();
  Serial.print("Conectat! IP: ");
  Serial.println(WiFi.localIP());
}

void conectareMQTT() {
  while (!mqtt.connected()) {
    Serial.print("Conectare MQTT...");
    if (mqtt.connect("esp32-robot")) {
      Serial.println(" OK!");
      mqtt.subscribe(TOPIC_COMENZI);
    } else {
      Serial.print(" eroare cod=");
      Serial.println(mqtt.state());
      delay(3000);
    }
  }
}

void setup() {
  Serial.begin(115200);
  conectareWiFi();
  mqtt.setServer(MQTT_BROKER, MQTT_PORT);
  mqtt.setCallback(onMesajPrimit);
  conectareMQTT();
}

void loop() {
  if (!mqtt.connected()) conectareMQTT();
  mqtt.loop();
}