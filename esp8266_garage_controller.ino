/* Relay web server
 *
 * Relay module is active low. We toggle pinmode INPUT(inactive)/OUTPUT(active) to switch states.
 * esp8266 module pins are INPUT on power up, so default relay state is inactive.
 * Switch should be connected via the NO relay contact.
 */

#include <ESP8266WiFi.h>
#include <PubSubClient.h>
extern "C" {
#include "user_interface.h"
}

uint8_t relayPin1 = D1;  // Entry gate
uint8_t relayPin2 = D2;  // Exit gate
uint8_t ledPin = D3;

const char* ssid = "mySSID";
const char* wifi_password = "myWiFiPassword";
const char* wifi_hostname = "GateController";

const char* mqtt_server = "cloudmqtt_broker_hostname";  // provided by cloudmqtt
unsigned int mqtt_port = 12345;  // provided by cloudmqtt
const char* mqtt_user = "myUser";  // as configured on cloudmqtt
const char* mqtt_password = "myPassword";  // as configured on cloudmqtt

WiFiClient espClient;
PubSubClient client(espClient);


void callback(char* topic, byte* payload, unsigned int length) {
  Serial.printf("Message arrived [%s] ", topic);
  for (int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
  }
  Serial.println();

  std::string topic_s(topic);
  if (topic_s.find("garage/entry") != std::string::npos) {
    Serial.println("Opening garage entry");
    toggle_relay(relayPin1);
  } else if (topic_s.find("garage/exit") != std::string::npos) {
    Serial.println("Opening garage exit");
    toggle_relay(relayPin2);
  } else {
    Serial.println("Unknown operation");
  }
}

void toggle_relay(uint8_t pin) {
  unsigned int toggle_time = 1000;
  unsigned int ledFlashDelay = 100;
  Serial.printf("Toggling relay at pin %d\n", pin);
  pinMode(pin, OUTPUT);
  for(unsigned int i = 0; (i * ledFlashDelay) < toggle_time; i++) {
    digitalWrite(ledPin, i % 2 ? LOW : HIGH);
    delay(ledFlashDelay);
  }
  digitalWrite(ledPin, LOW);
  pinMode(pin, INPUT);
}

void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Attempt to connect
    if (client.connect("ESP8266Client", mqtt_user, mqtt_password)) {
      Serial.println("connected");
      // Once connected, resubscribe
      client.subscribe("garage/#");
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}

void setup() {
  pinMode(relayPin1, INPUT);
  pinMode(relayPin2, INPUT);
  digitalWrite(ledPin, LOW);
  pinMode(ledPin, OUTPUT);

  Serial.begin(115200);
  delay(10);

  // Connect to Wifi.
  wifi_station_set_hostname((char *)wifi_hostname);
  Serial.printf("\n\n\nConnecting to %s\n", ssid);
  WiFi.begin(ssid, wifi_password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(">");
  }
  Serial.println(" connected");

  // Configure MQTT client.
  client.setServer(mqtt_server, mqtt_port);
  client.setCallback(callback);
}

void loop() {
  if (!client.connected()) {
    reconnect();
  }
  client.loop();
}
