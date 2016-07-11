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

char incommingMessageBuffer[MSG_BUFFER_LEN] = {0};
char outgoingMessageBuffer[MSG_BUFFER_LEN] = {0};

int connectedDevCountLocal = 0;
int connectedDevCountRemote = 0;
char connectedDevicesLocal[MAX_CONNECTED_DEVICES] = {0};
char connectedDevicesRemote[MAX_CONNECTED_DEVICES] = {0};

int leds[5] = { LED_0, LED_1, LED_2, LED_3, LED_4 };

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
  int t = (int)dht.readTemperature();
  generateMessage(READING, LOCAL_MASTER_ID, LOCAL_ESP_8266_ID, ("T" + String(t)).c_str());
  espSerial.write(outgoingMessageBuffer);
  
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
        if (results.value == 551485695) {
          requestedTemperatureMeasured++;
        } else if (results.value == 551518335) {
          requestedTemperatureMeasured--;
        }

        if (requestedTemperatureMeasured < REQ_TEMPERATURE_MIN) {
          //tone(BUZZER, BUZZ_FREQUENCY, BUZZ_DURATION);
          requestedTemperatureMeasured = REQ_TEMPERATURE_MIN;
        } else if (requestedTemperatureMeasured > REQ_TEMPERATURE_MAX) {
          //tone(BUZZER, BUZZ_FREQUENCY, BUZZ_DURATION);
          requestedTemperatureMeasured = REQ_TEMPERATURE_MAX;
        }

        generateMessage(READING, REMOTE_MASTER_ID, REMOTE_ESP_8266_ID, ("R" + String(requestedTemperatureMeasured)).c_str());
        espSerial.write(outgoingMessageBuffer);
        
        delay(IR_TIMEOUT);
        irrecv.resume(); // Receive the next value
      }
    }
    
    // turn LEDs on
    for (int i = 0; i < sizeof(leds); i++) {
      if ((requestedTemperatureMeasured >> i) && 1) {
        digitalWrite(leds[i], HIGH);
      } else {
        digitalWrite(leds[i], LOW);
      }
    }

    handleMessages();

    if (requestedTemperatureReceived < actualTemperatureMeasured) {
      digitalWrite(MOTOR, HIGH);
    } else {
      digitalWrite(MOTOR, LOW);
    }
  } else {
    digitalWrite(ON_LED, LOW);

    for (int i = 0; i < sizeof(leds); i++) {
      digitalWrite(leds[i], LOW);
    }
  }
}

// Allows no other messages in the meantime...
bool awaitConfirmation(int expectedReceiver, int expectedSender) {
  while (!espSerial.available()) {
    delay(50);
  }

  char startSymbol = espSerial.read();
  int messageType = (int)espSerial.read();
  int sender = (int)espSerial.read();
  int receiver = (int)espSerial.read();
  int dataLength = (int)espSerial.read();
  char endSymbol = espSerial.read();

  // All message attributes must be correct
  if (startSymbol != START_SYMBOL ||
      messageType != CONFIRM ||
      sender != expectedSender ||
      receiver != expectedReceiver ||
      dataLength != 0 ||
      endSymbol != END_SYMBOL) {
    return false;
  }

  return true;
}

void generateMessage(int messageType, int sender, int receiver, const char* data) {
  // Clear buffer
  int i;
  for (i = 0; i < MSG_BUFFER_LEN; i++) {
    outgoingMessageBuffer[i] = '\0';
  }

  // Fill in buffer
  outgoingMessageBuffer[0] = START_SYMBOL;
  outgoingMessageBuffer[1] = (char)messageType;
  outgoingMessageBuffer[2] = (char)sender;
  outgoingMessageBuffer[3] = (char)receiver;

  size_t dataLength = strlen(data);
  if (dataLength > MAX_DATA_LEN) {
    dataLength = MAX_DATA_LEN;
  }

  outgoingMessageBuffer[4] = (char)dataLength;

  for (i = 5; i < 5 + dataLength; i++) {
    outgoingMessageBuffer[i] = data[i];
  }

  outgoingMessageBuffer[i] = END_SYMBOL;
}

void readMessage(
  int messageType,
  int sender,
  int receiver,
  int dataLength,
  bool isLocal) {
  if (receiver != LOCAL_ESP_8266_ID && receiver != REMOTE_ESP_8266_ID) {
    // Forward
    return;
  }

  // only talk to your friends
  bool isValidSender = checkSender(sender, isLocal);
  if (!isValidSender) {
    return;
  }

  if (messageType == INFO) {
    Serial.println(incommingMessageBuffer);
    return;
  } else if (messageType == READING) {
    switch (incommingMessageBuffer[0]) {
      case 'R':
        requestedTemperatureReceived = (int)incommingMessageBuffer[1];
        break;
      case 'S':
        isSystemOn = incommingMessageBuffer[1] == '1';
        break;
    }
  } else if (messageType == REGISTER) {
    generateMessage(CONFIRM, receiver, sender, "");
    awaitConfirmation(receiver, sender);
  } else {
    // disregard commands or unknown types
  }
}

void handleMessages(void) {
  // Disregard all symbols until start symbol is received
  bool isMessageStarted = false;

  while (espSerial.available()) {
    char symbolRead = espSerial.read();

    if (!isMessageStarted && symbolRead == START_SYMBOL) {
      isMessageStarted = true;
    } else if (symbolRead != START_SYMBOL) {
      continue;
    }

    int messageType = (int)espSerial.read();
    int sender = (int)espSerial.read();
    int receiver = (int)espSerial.read();
    int dataLength = (int)espSerial.read();

    fillIncommingBuffer(dataLength);

    // TODO: check valid message format in advance
    char endSymbol = espSerial.read();
    isMessageStarted = false;

    readMessage(messageType, sender, receiver, dataLength, true);
    readMessage(messageType, sender, receiver, dataLength, false);
  }
}

void fillIncommingBuffer(int dataLength) {
  int i;
  // Clear buffer
  for (i = 0; i < MSG_BUFFER_LEN; i++) {
    incommingMessageBuffer[i] = '\0';
  }

  // TODO: check valid length
  // Fill buffer
  for (i = 0; i < dataLength; i++) {
    incommingMessageBuffer[i] = espSerial.read();
  }
}

bool checkSender(int sender, bool isLocal) {
  int connectedDevicesCount = isLocal ? connectedDevCountLocal : connectedDevCountRemote;

  for (int i = 0; i < connectedDevicesCount; i++) {
    if ((isLocal && connectedDevicesLocal[i] == sender)
        || (!isLocal && connectedDevicesRemote[i] == sender)) {
      return true;
    }
  }

  return false;
}
