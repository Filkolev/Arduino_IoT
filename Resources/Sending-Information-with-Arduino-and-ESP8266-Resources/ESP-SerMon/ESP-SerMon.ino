// Adafruit IO REST API access with ESP8266
//
// For use with ESP8266 Arduino from:
//   https://github.com/esp8266/Arduino
//
// Works great with ESP8266 modules like the Adafruit Huzzah ESP:
//  ----> https://www.adafruit.com/product/2471
//
// Written by Tony DiCola for Adafruit Industries.  
// MIT license, all text above must be included in any redistribution.
#include <ESP8266WiFi.h>
#include "Adafruit_IO_Client.h"


// Configure WiFi access point details.
#define WLAN_SSID  "SoftUni"
#define WLAN_PASS  "...password..."

// Configure Adafruit IO access.
#define AIO_KEY    "552d5606b37d4174af526878a4371538"


// Create an ESP8266 WiFiClient class to connect to the AIO server.
WiFiClient client;

// Create an Adafruit IO Client instance.  
Adafruit_IO_Client aio = Adafruit_IO_Client(client, AIO_KEY);

// Finally create instances of Adafruit_IO_Feed objects, one per feed.  
Adafruit_IO_Feed lineFeed = aio.getFeed("my_line_feed");
Adafruit_IO_Feed toggleFeed = aio.getFeed("my_toggle");

// Alternatively to access a feed with a specific key:
//Adafruit_IO_Feed testFeed = aio.getFeed("esptestfeed", "...esptestfeed key...");

// promenlivi za dannite ot Serial Monitor
char mySerialBuffer[50];
int pos = 0;
int mySerialInteger;
boolean readyToSend = false;

void setup() {
  // Setup serial port access.
  Serial.begin(9600); // 9600!!!
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

  Serial.println(F("ESP: WiFi connected"));  
  Serial.println(F("ESP: IP address: ")); Serial.println(WiFi.localIP());
  
  // Initialize the Adafruit IO client class (not strictly necessary with the
  // client class, but good practice).
  aio.begin();

  Serial.println(F("ESP: Ready!"));
}

void loop() {

  
  readSerial();
  

  if(readyToSend){

    if (lineFeed.send(mySerialInteger)) {
      Serial.print(mySerialInteger, DEC);
      Serial.println('W'); // W = wrote
    }
    else {
      Serial.print(F("1R")); // Error = failed to write
    }
    readyToSend = false;
  }

  // Now wait 10 seconds and read the current feed value.
//  Serial.println(F("Waiting 10 seconds and then reading the feed value."));
  delay(1000);

  FeedData latestLine = lineFeed.receive();
  if (latestLine.isValid()) {
    Serial.print(latestLine);
    Serial.println('L'); // L = latest value from Line feed
  }
  else {
    Serial.println(F("2R")); // Error 2 = failed to get value from Line feed 
  }

  FeedData latestToggle = toggleFeed.receive();
  if (latestToggle.isValid()) {
    Serial.print(latestToggle);
    Serial.println('T'); // T = latest value from Toggle feed
  }
  else {
    Serial.println(F("3R")); // Error 3 = failed to get value from Toggle feed
  }

  // Now wait 10 more seconds and repeat.
//  Serial.println(F("Waiting 10 seconds and then writing a new feed value."));
  delay(1000);
}

void readSerial(){
  Serial.println("reading serial");
  while(Serial.available() > 0){
    char c = Serial.read();
    Serial.print(c);

    if(c == '\n'){
      // convert serial buffer string to int
      mySerialInteger = atoi(mySerialBuffer);

      // reset the buffer array
      for(int i = 0; i <= pos; i++){
        mySerialBuffer[i] = '\0';
      }
      
      // reset the counter
      pos = 0;

      // send the value to Adafruit IO
      readyToSend = true; 
  
    } else {
      if(c >= '0' && c <= '9'){
        Serial.println(c);
        mySerialBuffer[pos] = c;
        pos++;
      }      
    }    
  }
}

