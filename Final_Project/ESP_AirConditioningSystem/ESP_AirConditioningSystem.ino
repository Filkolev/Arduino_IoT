#include <ESP8266WiFi.h>
#include "Adafruit_IO_Client.h"

// WiFi access point details
#define WLAN_SSID  "...your SSID..."
#define WLAN_PASS  "...your password..."

// Adafruit IO access details
#define AIO_KEY    "...your Adafruit IO key value ..."

WiFiClient client;
Adafruit_IO_Client aio = Adafruit_IO_Client(client, AIO_KEY);

Adafruit_IO_Feed testFeed = aio.getFeed("esptestfeed");

unsigned int count = 0;

void setup() {
  Serial.begin(115200);
  delay(10);
  Serial.println(); Serial.println();
  Serial.println(F("Adafruit IO ESP8266 test!"));

  // Connect to WiFi access point.
  Serial.print("Connecting to ");
  Serial.println(WLAN_SSID);

  WiFi.begin(WLAN_SSID, WLAN_PASS);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println();

  Serial.println("WiFi connected");  
  Serial.println("IP address: "); Serial.println(WiFi.localIP());
  
  aio.begin();

  Serial.println(F("Ready!"));
}

void loop() {
  count += 1;
  if (testFeed.send(count)) {
    Serial.print(F("Wrote value to feed: ")); Serial.println(count, DEC);
  }
  else {
    Serial.println(F("Error writing value to feed!"));
  }

  Serial.println(F("Waiting 10 seconds and then reading the feed value."));
  delay(10000);

  FeedData latest = testFeed.receive();
  if (latest.isValid()) {
    Serial.print(F("Received value from feed: ")); Serial.println(latest);
    
    int i;
    if (latest.intValue(&i)) {
      Serial.print(F("Value as an int: ")); Serial.println(i, DEC);
    }
  }
  else {
    Serial.print(F("Failed to receive the latest feed value!"));
  }

  Serial.println(F("Waiting 10 seconds and then writing a new feed value."));
  delay(10000);
}
