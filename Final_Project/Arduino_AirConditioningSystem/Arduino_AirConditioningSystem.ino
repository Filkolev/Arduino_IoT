#include "DHT.h"
#include <IRremote.h>

#include "common_env.h"
#include "local_env.h"
#include "remote_env.h"

DHT dht(DHTPIN, DHTTYPE);
IRrecv irrecv(IR_PIN);
decode_results results;

int actualTemperatureMeasured;
int actualTemperatureReceived;

int requestedTemperatureMeasured = REQ_TEMPERATURE_DEFAULT;
int requestedTemperatureReceived = REQ_TEMPERATURE_DEFAULT;

bool isSystemOn = false;

int incommingBufferFilled = 0;
char incommingMessageBuffer[MSG_BUFFER_LEN] = {0};
char outgoingMessageBuffer[MSG_BUFFER_LEN] = {0};

int connectedDevCountLocal = 0;
int connectedDevCountRemote = 0;
char connectedDevicesLocal[MAX_CONNECTED_DEVICES] = {0};
char connectedDevicesRemote[MAX_CONNECTED_DEVICES] = {0};

void setup() {  
  // TODO: Setup serial communication
  dht.begin();
  irrecv.enableIRIn(); // Start the IR receiver
}

void loop() {    
  // Local
  // Read messages, handle them
  handleMessages();
  
  // read DHT, TODO: send to ESP
  int t = (int)dht.readTemperature();

  // Remote
  // Read messages, handle them
  handleMessages();
  
  // Read IR, TODO: send to ESP
  if (irrecv.decode(&results)) {
    Serial.println(results.value, HEX);
    irrecv.resume(); // Receive the next value
  }

  handleMessages();
  // Control motor, LEDs and buzzer
  
}

// Allows no other messages in the meantime...
bool awaitConfirmation(int expectedReceiver, int expectedSender) {
  char startSumbol = Serial.read();
  int messageType = (int)Serial.read();
  int sender = (int)Serial.read();
  int receiver = (int)Serial.read();
  int dataLength = (int)Serial.read();
  char endSymbol = Serial.read();

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

void handleMessages(void) {
  // Disregard all symbols until start symbol is received
  bool isMessageStarted = false;

  while (Serial.available()) {
    char symbolRead = Serial.read();

    if (!isMessageStarted && symbolRead == START_SYMBOL) {
      isMessageStarted = true;
    } else if (symbolRead != START_SYMBOL) {
      continue;
    }

    int messageType = (int)Serial.read();
    int sender = (int)Serial.read();
    int receiver = (int)Serial.read();
    int dataLength = (int)Serial.read();

    fillIncommingBuffer(dataLength);

    // TODO: check valid message format in advance
    char endSymbol = Serial.read();

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
    incommingMessageBuffer[i] = Serial.read();
  }
}

// void generateMessage(int messageType, int sender, int receiver, const char* data);

void readMessage(
  int messageType,
  int sender,
  int receiver,
  int dataLength,
  bool isLocal) {
  // Forward to receiver or Master if receiver is unknown
  if (receiver != LOCAL_ESP_8266_ID && receiver != REMOTE_ESP_8266_ID) {
    generateMessage(messageType, sender, receiver, "");
    Serial.println(outgoingMessageBuffer);
    return;
  }

  bool isValidSender = checkSender(sender, isLocal);

  // only talk to your friends
  if (!isValidSender) {
    return;
  }

  if (messageType == INFO) {
    // Forward to Master for printing
    generateMessage(messageType, sender, receiver, "");
    Serial.println(outgoingMessageBuffer);
    return;
  } else if (messageType == READING) {
    // TODO: allow other devices to register
    switch (incommingMessageBuffer[incommingBufferFilled]) {
      case 'T':
        // update real temperature
        break;
      case 'R':
        // update requested temperature
        break;
      default:
        // disregard
    }
  } else {
    // disregard commands, registers or unknown types
  }
}

bool checkSender(int sender, bool isLocal) {
  int connectedDevicesCount = isLocal ? connectedDevCountLocal : connectedDevCountRemote;
  char devices[] = isLocal ? connectedDevicesLocal : connectedDevicesRemote;

  for (int i = 0; i < connectedDevicesCount; i++) {
    if (devices[i] == sender) {
      return true;
    }
  }

  return false;
}
