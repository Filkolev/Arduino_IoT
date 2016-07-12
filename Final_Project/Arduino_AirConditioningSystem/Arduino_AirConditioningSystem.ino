#include "DHT.h"
#include <IRremote.h>
#include <SoftwareSerial.h>

#include "common_env.h"
#include "local_env.h"
#include "remote_env.h"

DHT dht(DHTPIN, DHTTYPE);
IRrecv irrecv(IR_PIN);
SoftwareSerial espSerial(RX, TX); // RX, TX
decode_results results;

int actualTemperatureMeasured;
long currentMillis;
long previousReading = 0;

int requestedTemperatureMeasured = REQ_TEMPERATURE_DEFAULT;
int requestedTemperatureReceived = REQ_TEMPERATURE_DEFAULT;

bool isSystemOn = false;

int leds[5] = { LED_0, LED_1, LED_2, LED_3, LED_4 };

String msg;

void setup() {
  Serial.begin(BAUD_RATE);
  espSerial.begin(BAUD_RATE);
  dht.begin();
  irrecv.enableIRIn(); // Start the IR receiver

  pinMode(ON_LED, OUTPUT);

  for (int i = 0; i < sizeof(leds); i++) {
    pinMode(leds[i], OUTPUT);
  }

  pinMode(BUZZER, OUTPUT);
  pinMode(MOTOR, OUTPUT);
}

void loop() {
  // Local
  // Read messages, handle them
  handleMessages();
  

  // read DHT
  actualTemperatureMeasured = (int)dht.readTemperature();
  msg = "T" + String((char)actualTemperatureMeasured);
  espSerial.write(msg.c_str());

  //Serial.print("requested: "); Serial.println(requestedTemperatureReceived);
  //Serial.print("actual: "); Serial.println(actualTemperatureMeasured);

  // Remote
  // Read messages, handle them
  handleMessages();

  if (isSystemOn) {
    // turn green LED on
    digitalWrite(ON_LED, HIGH);

    // Read IR
    currentMillis = millis();
    if (currentMillis - previousReading >= IR_TIMEOUT) {
      previousReading = currentMillis;

      if (irrecv.decode(&results)) {
        if (results.value == UP) {
          requestedTemperatureMeasured++;
        } else if (results.value == DOWN) {
          requestedTemperatureMeasured--;
        }

        if (requestedTemperatureMeasured < REQ_TEMPERATURE_MIN) {
          //tone(BUZZER, BUZZ_FREQUENCY, BUZZ_DURATION);
          digitalWrite(BUZZER, HIGH);
          delay(100);
          digitalWrite(BUZZER, LOW);
          requestedTemperatureMeasured = REQ_TEMPERATURE_MIN;
        } else if (requestedTemperatureMeasured > REQ_TEMPERATURE_MAX) {
          //tone(BUZZER, BUZZ_FREQUENCY, BUZZ_DURATION);
          digitalWrite(BUZZER, HIGH);
          delay(100);
          digitalWrite(BUZZER, LOW);
          requestedTemperatureMeasured = REQ_TEMPERATURE_MAX;
        }

        msg = "R" + String((char)requestedTemperatureMeasured);
        espSerial.write(msg.c_str());

        delay(IR_TIMEOUT);
        irrecv.resume(); // Receive the next value
      }
    }

    // turn LEDs on
    for (int i = 0; i < sizeof(leds); i++) {
      if ((requestedTemperatureMeasured >> i) & 1) {
        digitalWrite(leds[i], HIGH);
      } else {
        digitalWrite(leds[i], LOW);
      }
    }

    handleMessages();
    if (requestedTemperatureReceived < actualTemperatureMeasured) {
      // digitalWrite(MOTOR, HIGH);
      digitalWrite(BUZZER, HIGH);

    } else {
      // digitalWrite(MOTOR, LOW);
      digitalWrite(BUZZER, LOW);

    }
  } else {
    digitalWrite(ON_LED, LOW);
    digitalWrite(BUZZER, LOW);

    for (int i = 0; i < sizeof(leds); i++) {
      digitalWrite(leds[i], LOW);
    }
  }
}

void handleMessages(void) {
  if (espSerial.available()) {
    char symbolRead = espSerial.read();
    Serial.print(symbolRead);
    
    switch (symbolRead) {
      case 'R':
        requestedTemperatureReceived = (int)espSerial.read();
        Serial.println(requestedTemperatureReceived);
        break;
      case 'S':
        isSystemOn = espSerial.read() == '1';
        Serial.println(isSystemOn);
        break;
    }
    delay(200);
  }
}
