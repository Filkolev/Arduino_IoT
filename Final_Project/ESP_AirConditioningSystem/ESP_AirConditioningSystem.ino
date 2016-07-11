#include <ESP8266WiFi.h>

#include "Adafruit_IO_Client.h"

#include "common_env.h"
#include "local_env.h"
#include "remote_env.h"

WiFiClient client;
Adafruit_IO_Client aio = Adafruit_IO_Client(client, AIO_KEY);

Adafruit_IO_Feed systemStatusFeed = aio.getFeed("toggle_air_system_button_feed");
Adafruit_IO_Feed requestedTemperatureFeed = aio.getFeed("requested_temperature_feed");
Adafruit_IO_Feed actualTemperatureFeed = aio.getFeed("actual_temperature_feed");

int actualTemperatureMeasured;

int requestedTemperatureMeasured = REQ_TEMPERATURE_DEFAULT;
int requestedTemperatureReceived = REQ_TEMPERATURE_DEFAULT;

bool isSystemOn = false;

char incommingMessageBuffer[MSG_BUFFER_LEN] = {0};
char outgoingMessageBuffer[MSG_BUFFER_LEN] = {0};

int connectedDevCountLocal = 0;
int connectedDevCountRemote = 0;
char connectedDevicesLocal[MAX_CONNECTED_DEVICES] = {0};
char connectedDevicesRemote[MAX_CONNECTED_DEVICES] = {0};

void setup() {
  Serial.begin(115200); // TODO: extract baud rate in macro

  WiFi.begin(WLAN_SSID, WLAN_PASS);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
  }

  aio.begin();

  registerWithMasterLocal();
  registerWithMasterRemote();
}

void registerWithMasterLocal(void) {
  registerWithDevice(LOCAL_ESP_8266_ID, LOCAL_MASTER_ID, true);
}

void registerWithMasterRemote(void) {
  registerWithDevice(REMOTE_ESP_8266_ID, REMOTE_MASTER_ID, false);
}

// bool local only needed because system simulates local and remote setups
void registerWithDevice(int deviceId, int other, bool local) {
  if (local && connectedDevCountLocal >= MAX_CONNECTED_DEVICES) {
    return; // capacity exceeded, disregard request
  }

  if (!local && connectedDevCountRemote >= MAX_CONNECTED_DEVICES) {
    return; // capacity exceeded, disregard request
  }

  generateMessage(REGISTER, deviceId, other, "");
  Serial.println(outgoingMessageBuffer);

  // Wait for response; disregard wrong responses for now
  awaitConfirmation(deviceId, other);

  // Three-way handshake - acknowlegde reponse by Master
  generateMessage(CONFIRM, deviceId, other, "");
  Serial.println(outgoingMessageBuffer);

  if (local) {
    connectedDevicesLocal[connectedDevCountLocal++] = other;
  } else {
    connectedDevicesRemote[connectedDevCountRemote++] = other;
  }
}

bool awaitConfirmation(int expectedReceiver, int expectedSender) {
  while (!Serial.available()) {
    delay(50);
  }
  
  char startSymbol = Serial.read();
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

void loop() {
  handleMessages();

  // read system on/off state from adafruit
  FeedData systemState = systemStatusFeed.receive();
  if (systemState.isValid()) {
    char *state = systemState;
    isSystemOn = String(state).equals("ON");
    Serial.println(isSystemOn);
  }

  handleMessages();

  Serial.println(actualTemperatureMeasured);

  // upload real temperature to adafruit
  actualTemperatureFeed.send(actualTemperatureMeasured);

  handleMessages();

  // TODO: move this
  // if on - send requested temperature to adafruit then read it
  if (isSystemOn) {
    requestedTemperatureFeed.send(requestedTemperatureMeasured);

    handleMessages();

    FeedData requestedTemperatureData = requestedTemperatureFeed.receive();
    if (requestedTemperatureData.isValid()) {
      requestedTemperatureData.intValue(&requestedTemperatureReceived);
      Serial.println(requestedTemperatureReceived); // Send to arduino
    }
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
    switch (incommingMessageBuffer[0]) {
      case 'T':
        actualTemperatureMeasured = (int)incommingMessageBuffer[1];         
        actualTemperatureFeed.send(actualTemperatureMeasured);
        break;
      case 'R':
        // update requested temperature
        requestedTemperatureMeasured = (int)incommingMessageBuffer[1];        
        break;
    }
  } else {
    // disregard commands, registers or unknown types
  }
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
    isMessageStarted = false;

    readMessage(messageType, sender, receiver, dataLength, true);
    readMessage(messageType, sender, receiver, dataLength, false);
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

