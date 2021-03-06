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

void setup() {
  Serial.begin(BAUD_RATE);

  WiFi.begin(WLAN_SSID, WLAN_PASS);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
  }

  aio.begin();
}

void loop() {
  handleMessages();

  // read system on/off state from adafruit
  FeedData systemState = systemStatusFeed.receive();
  if (systemState.isValid()) {
    char *state = systemState;
    isSystemOn = String(state).equals("ON");

    
      Serial.print('S');
      Serial.println(isSystemOn ? '1' : '0');
      
      
    
  }
  
  handleMessages();

  // upload real temperature to adafruit
  // actualTemperatureFeed.send(actualTemperatureMeasured);

  handleMessages();

  // TODO: move this
  // if on - send requested temperature to adafruit then read it
  if (isSystemOn) {
    // requestedTemperatureFeed.send(requestedTemperatureMeasured);

    handleMessages();

    // FeedData requestedTemperatureData = requestedTemperatureFeed.receive();
    //    if (requestedTemperatureData.isValid()) {
    //      requestedTemperatureData.intValue(&requestedTemperatureReceived);
    //      Serial.print("R" + String((char)requestedTemperatureReceived));
    //    }

    Serial.print('R');
    Serial.println((char)requestedTemperatureReceived);
  }
}

void handleMessages(void) {
  while (Serial.available()) {
    String msg = "";
    msg += (char)Serial.read();
    msg += (char)Serial.read();

    switch (msg[0]) {
      case 'T':
        actualTemperatureMeasured = (int)msg[1];
        //actualTemperatureFeed.send(actualTemperatureMeasured);
        break;
      case 'R':
        // update requested temperature
        requestedTemperatureMeasured = (int)msg[1];
        //requestedTemperatureFeed.send(requestedTemperatureMeasured);
        requestedTemperatureReceived = requestedTemperatureMeasured;
        break;
    }
  }

  delay(200);
}
