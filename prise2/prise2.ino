#include <ESP8266WiFi.h>
#include <WiFiManager.h>
#include <PubSubClient.h>


#define LED_PIN 5
#define BOUTON_PIN 4


WiFiClient espClient;
PubSubClient client(espClient);

const char* mqtt_server = "172.20.10.12";
const int mqtt_port = 1883;
const char* mqttUser = "toto";
const char* mqttPassword = "toto";

const char* mqtt_topic_led = "prise2/led";
const char* mqtt_topic_etat_led = "prise2/etat_led";

int etatBouton = HIGH;

void publishLedState() {  
  const char* etat = digitalRead(LED_PIN) ? "1" : "0";
  client.publish(mqtt_topic_etat_led, etat);
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
    if (client.connect("MGCT_prise2_mqtt", mqttUser, mqttPassword)) {
      Serial.println("connecté !");
      client.subscribe(mqtt_topic_led);
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

  WiFiManager wm;
  if (!wm.autoConnect("MGCT_prise2", "12345678")) {
    Serial.println("Échec de connexion WiFi !");
    ESP.restart();
  }

  Serial.println("Connecté au WiFi !");
  Serial.print("Adresse IP : ");
  Serial.println(WiFi.localIP());

  client.setServer(mqtt_server, mqtt_port);
  client.setCallback(callback);
}

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
  etatBouton = etatActuel;
}
