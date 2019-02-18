/* switch controller
 *
 * Pads of garage remote are connected to drain and source of NPN transistor operating as a switch.
 * Setting gate pin HIGH closes the switch, emulating a button press. The +ve lead of the remote
 * button must connect to the drain pin of the resistor.
 */

#include <ESP8266WiFi.h>
#include <PubSubClient.h>
extern "C" {
#include "user_interface.h"
}

#include "garagecontroller.h"
#include "song.h"

uint8_t piezoPin = D3;
uint8_t ledPin = D5;

const char* ssid = "mySSID";
const char* wifi_password = "myWiFiPassword";
const char* wifi_hostname = "GateController";

long lastReconnectAttempt = 0;
const char* mqtt_server = "cloudmqtt_broker_hostname";  // provided by cloudmqtt
unsigned int mqtt_port = 12345;  // provided by cloudmqtt
const char* mqtt_user = "myUser";  // as configured on cloudmqtt
const char* mqtt_password = "myPassword";  // as configured on cloudmqtt

const char* unique_client_name = wifi_hostname;
const char* subscribe_topics = "testgarage/#";
activator entry_gate = {"testgarage/entry", D1};
activator exit_gate = {"testgarage/exit", D2};

GarageController *gc;

WiFiClient espClient;
PubSubClient client(espClient);


note melody1[] = {
  note {NOTE_DS5, 8},
  note {NOTE_DS5, 8},
  note {NOTE_REST, 8},
  note {NOTE_DS5, 4},
  note {NOTE_B4, 8},
  note {NOTE_DS5, 8},
  note {NOTE_REST, 8},
  note {NOTE_FS5, 2},
};

note melody2[] = {
  note {NOTE_FS4, 8},
  note {NOTE_E5, 8},
  note {NOTE_REST, 8},
  note {NOTE_E5, 8},
  note {NOTE_E5, 8},
  note {NOTE_DS5, 6},
  note {NOTE_REST, 16},
  note {NOTE_CS5, 8},
  note {NOTE_B4, 4},
};


void delay_flash_led(unsigned int duration) {
  unsigned int ledFlashDelay = 100;
  for(unsigned int i = 0; (i * ledFlashDelay) < duration; i++) {
    digitalWrite(ledPin, i % 2 ? LOW : HIGH);
    delay(ledFlashDelay);
  }
  // End state is led off.
  digitalWrite(ledPin, LOW);
}

void callback(char* topic, byte* payload, unsigned int length) {
  gc->callback(topic, payload, length);
}

bool reconnect() {
  // Loop until we're reconnected
  Serial.print("Attempting MQTT connection...");
  // Attempt to connect
  if (client.connect(unique_client_name, mqtt_user, mqtt_password)) {
    Serial.println("connected");
    // Once connected, resubscribe
    client.subscribe(subscribe_topics);
  } else {
    Serial.print("failed, rc=");
    Serial.print(client.state());
  }
  return client.connected();
}

void setup() {
  Serial.begin(9600);
  Serial.println("Starting up");

  noTone(piezoPin);
  pinMode(ledPin, OUTPUT);
  digitalWrite(ledPin, LOW);

  std::vector<activator> gates;
  gates.push_back(entry_gate);
  gates.push_back(exit_gate);
  gc = new GarageController(gates);

  note pip = {NOTE_DS5, 8};
  play_song(piezoPin, &pip, 1);

  // Connect to Wifi.
  wifi_station_set_hostname((char *)wifi_hostname);
  Serial.printf("\n\n\nConnecting to %s\n", ssid);
  WiFi.begin(ssid, wifi_password);
  while (WiFi.status() != WL_CONNECTED) {
    delay_flash_led(500);
    Serial.print(">");
  }
  Serial.println(" connected");

  pip.freq *= 2;
  play_song(piezoPin, &pip, 1);

  // Configure MQTT client.
  client.setServer(mqtt_server, mqtt_port);
  client.setCallback(callback);
}

void loop() {
  if (!client.connected()) {
    long now = millis();
    if (now - lastReconnectAttempt > 5000) {
      lastReconnectAttempt = now;
      if(reconnect()) {
        lastReconnectAttempt = 0;
      }
    }
  }
  client.loop();
}
