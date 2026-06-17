#include <ESP8266WiFi.h>
#include <WiFiManager.h>
#include <PubSubClient.h>
#include <OneWire.h>
#include <DallasTemperature.h>

#define LED_PIN 5
#define BOUTON_PIN 4
#define ONE_WIRE_BUS 12

OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);

WiFiClient espClient;
PubSubClient client(espClient);

const char* mqtt_server = "172.20.10.12";
const int mqtt_port = 1883;
const char* mqttUser = "toto";
const char* mqttPassword = "toto";

const char* mqtt_topic_led = "prise/led";
const char* mqtt_topic_temp = "prise/temp";
const char* mqtt_topic_led_state = "prise/etat_led";

int etatBouton = HIGH;

void publishLedState() {  
  const char* etat = digitalRead(LED_PIN) ? "1" : "0";
  client.publish(mqtt_topic_led_state, etat);
}

void callback(char* topic, byte* payload, unsigned int length) {
  String message;
  for (unsigned int i = 0; i < length; i++) {
    message += (char)payload[i];
  }

  if (String(topic) == mqtt_topic_led) {
    if (message == "1") {
      digitalWrite(LED_PIN, HIGH);
      Serial.println("led on par mqtt");
    } 
    else if (message == "0") {
      digitalWrite(LED_PIN, LOW);
      Serial.println("led off par mqtt");
    }

    publishLedState(); 
  }
}

void reconnectMQTT() {
  while (!client.connected()) {
    if (client.connect("MGCT_prise1_mqtt", mqttUser, mqttPassword)) {
      Serial.println("connecté !");
      client.subscribe(mqtt_topic_led);
      client.subscribe(mqtt_topic_temp);
    } else {
      delay(2000);
    }
  }
}

void setup() {
  Serial.begin(115200);
  pinMode(LED_PIN, OUTPUT);
  pinMode(BOUTON_PIN, INPUT_PULLUP);
  digitalWrite(LED_PIN, LOW);
  sensors.begin();

  WiFiManager wm;
  if (!wm.autoConnect("MGCT_prise1", "12345678")) {
    Serial.println("Échec de connexion WiFi !");
    ESP.restart();
  }

  Serial.println("Connecté au WiFi !");
  Serial.print("Adresse IP : ");
  Serial.println(WiFi.localIP());

  client.setServer(mqtt_server, mqtt_port);
  client.setCallback(callback);
}

unsigned long dernierEnvoi = 0;
const unsigned long intervalle = 30000;

unsigned long dernierEtatLed = 0;
const unsigned long intervalleEtatLed = 1000;

void loop() {
  if (!client.connected()) {
    reconnectMQTT();
  }
  client.loop();

  int etatActuel = digitalRead(BOUTON_PIN);

  if (etatActuel == LOW && etatBouton == HIGH) {
    digitalWrite(LED_PIN, !digitalRead(LED_PIN));
    publishLedState();
    Serial.println("led local");
    delay(300);
  }

  unsigned long maintenant = millis();

  if (maintenant - dernierEtatLed >= intervalleEtatLed) {
    dernierEtatLed = maintenant;
    publishLedState();
  }

  if (maintenant - dernierEnvoi >= intervalle) {
    dernierEnvoi = maintenant;
    Serial.print("Requesting temperatures...");
    sensors.requestTemperatures();
    Serial.println("DONE");

    Serial.print("Temperature for the device 1 (index 0) is: ");
    Serial.println(sensors.getTempCByIndex(0));

    sensors.requestTemperatures();
    float temperature = sensors.getTempCByIndex(0);  
    char tempString[8];
    dtostrf(temperature, 1, 2, tempString);
    client.publish(mqtt_topic_temp, tempString);
  }

  etatBouton = etatActuel;
}
