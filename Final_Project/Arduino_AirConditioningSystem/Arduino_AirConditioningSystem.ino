#include "DHT.h"
#include <IRremote.h>

#include "common_env.h"
#include "local_env.h"
#include "remote_env.h"

DHT dht(DHTPIN, DHTTYPE);
IRrecv irrecv(IR_PIN);
decode_results results;

void setup() {  
  dht.begin();
  irrecv.enableIRIn(); // Start the IR receiver
}

void loop() {
  return;
  float t = dht.readTemperature();

  if (irrecv.decode(&results)) {
    Serial.println(results.value, HEX);
    irrecv.resume(); // Receive the next value
  }
}
