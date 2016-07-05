// Tezi redove sa predi void setup()
Adafruit_IO_Feed toggleFeed = aio.getFeed("...ime na feed...");
unsigned int led_pin = 2;

void setup() {
  pinMode(led_pin, OUTPUT);
}

void loop() {
  // tezi redove sa sled proverkata na izpratenata stojnost (red 117 v primera)
  FeedData led_latest = toggleFeed.receive();
  if (led_latest.isValid()) {
    Serial.print(F("Received value from toggle: ")); Serial.println(led_latest);
    int i;
    if (led_latest.intValue(&i)) {
      if(i == 1){
        digitalWrite(led_pin, HIGH);
        Serial.println(F("Received ON command."));
      } else {
        digitalWrite(led_pin, LOW);
        Serial.println(F("Received OFF command."));
      }
    }
  }
  else {
    Serial.print(F("Failed to receive the latest toggle value!"));
  }
}
