#ifndef GarageController_h
#define GarageController_h

#include <vector>

#include <PubSubClient.h>

typedef struct {
  String topic;
  int switchPin;
} activator;

class GarageController {
public:
  GarageController(std::vector<activator> gates);
  void callback(const char* topic, unsigned char* payload, unsigned int length);
private:
  GarageController();
  void toggle_switch(int pin);
  std::vector<activator> _gates;
  unsigned int toggle_time = 1000;
};


GarageController::GarageController(std::vector<activator> gates):
  _gates(gates) {
    for (auto& g : _gates) {
      digitalWrite(g.switchPin, LOW);
      pinMode(g.switchPin, INPUT);
    }
}

void GarageController::callback(const char* topic, byte* payload, unsigned int length) {
  Serial.printf("Message arrived [%s] ", topic);
  for (int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
  }
  Serial.println();

  String topic_s(topic);

  for (auto &g : _gates) {
    if (topic_s.indexOf(g.topic) != -1) {
      Serial.printf("Activation by topic %s\n", g.topic.c_str());
      toggle_switch(g.switchPin);
      //    play_song(piezoPin, melody1, sizeof(melody1)/sizeof(note));
      return;
    }
  }

  Serial.printf("Unknown topic '%s'\n", topic);
}

void GarageController::toggle_switch(int pin) {
  Serial.printf("Toggling switch at pin %d\n", pin);
  pinMode(pin, OUTPUT);
  digitalWrite(pin, HIGH);
  delay(toggle_time);
  digitalWrite(pin, LOW);
  pinMode(pin, INPUT);
  Serial.printf("Closing switch at pin %d\n", pin);
}

#endif /* GarageController_h */
