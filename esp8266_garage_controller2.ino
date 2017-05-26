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

#include "pitches.h"


uint8_t switchPin1 = D1;  // Entry gate
uint8_t switchPin2 = D2;  // Exit gate
uint8_t piezoPin = D3;
uint8_t ledPin = D5;

const char* ssid = "mySSID";
const char* wifi_password = "myWiFiPassword";
const char* wifi_hostname = "GateController";

const char* mqtt_server = "cloudmqtt_broker_hostname";  // provided by cloudmqtt
unsigned int mqtt_port = 12345;  // provided by cloudmqtt
const char* mqtt_user = "myUser";  // as configured on cloudmqtt
const char* mqtt_password = "myPassword";  // as configured on cloudmqtt

WiFiClient espClient;
PubSubClient client(espClient);


typedef struct {
  int freq;
  int duration;
} note;

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

void play_song(int pin, note* melody, int num_notes) {
  // iterate over the notes of the melody:
  for (int thisNote = 0; thisNote < num_notes; thisNote++) {
    // to calculate the note duration, take one second
    // divided by the note type.
    //e.g. quarter note = 1000 / 4, eighth note = 1000/8, etc.
    int noteDuration = 1000 / melody[thisNote].duration;
    tone(pin, melody[thisNote].freq, noteDuration);

    // to distinguish the notes, set a minimum time between them.
    // the note's duration + 30% seems to work well:
    delay((int)(noteDuration * 1.30));
    // stop the tone playing:
    noTone(pin);
  }
}

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
  Serial.printf("Message arrived [%s] ", topic);
  for (int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
  }
  Serial.println();

  String topic_s(topic);
  if (topic_s.indexOf("garage/entry") != -1) {
    Serial.println("Opening garage entry");
    toggle_switch(switchPin1);
    play_song(piezoPin, melody1, sizeof(melody1)/sizeof(note));
  } else if (topic_s.indexOf("garage/exit") != -1) {
    Serial.println("Opening garage exit");
    toggle_switch(switchPin2);
    play_song(piezoPin, melody2, sizeof(melody2)/sizeof(note));
  } else {
    Serial.println("Unknown operation");
  }
}

void toggle_switch(uint8_t pin) {
  unsigned int toggle_time = 1000;
  Serial.printf("Toggling switch at pin %d\n", pin);
  digitalWrite(pin, HIGH);
  delay_flash_led(toggle_time);
  digitalWrite(pin, LOW);
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
      delay_flash_led(5000);
    }
  }
}

void setup() {
  noTone(piezoPin);
  digitalWrite(switchPin1, LOW);
  pinMode(switchPin1, OUTPUT);
  digitalWrite(switchPin2, LOW);
  pinMode(switchPin2, OUTPUT);
  pinMode(ledPin, OUTPUT);
  digitalWrite(ledPin, LOW);

  Serial.begin(115200);

  // Connect to Wifi.
  wifi_station_set_hostname((char *)wifi_hostname);
  Serial.printf("\n\n\nConnecting to %s\n", ssid);
  WiFi.begin(ssid, wifi_password);
  while (WiFi.status() != WL_CONNECTED) {
    delay_flash_led(500);
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
