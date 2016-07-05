#include <SoftwareSerial.h>

SoftwareSerial mySerial(5, 4); // RX, TX

char mySerialBuffer[50];
int pos = 0;
int mySerialInteger;

int ledPin = 13;

void setup() {
  // Open serial communications and wait for port to open:
  Serial.begin(9600);

  // set the data rate for the SoftwareSerial port
  mySerial.begin(9600);

  pinMode(ledPin, OUTPUT);
}

void loop() { // run over and over
  readSerial();

  // send chars received from Serial Monitor to ESP 
  if (Serial.available()) {
    mySerial.write(Serial.read());
  }
}

void readSerial(){
  while(mySerial.available() > 0){
    char c = mySerial.read();    

    if(c == 'T'){ // received Toggle feed value
      // convert serial buffer string to int
      mySerialInteger = atoi(mySerialBuffer);

      if(mySerialInteger == 1){
        digitalWrite(ledPin, HIGH);
      } else if(mySerialInteger == 0){
        digitalWrite(ledPin, LOW);
      } 

      // reset the buffer array
      for(int i = 0; i <= pos; i++){
        mySerialBuffer[i] = '\0';
      }
      
      // reset the counter
      pos = 0;
  
    } else if(c == 'L'){ // received Toggle feed value
      // convert serial buffer string to int
      mySerialInteger = atoi(mySerialBuffer);

      // process data if needed
      Serial.print("Received: ");
      Serial.println(mySerialInteger);

      // reset the buffer array
      for(int i = 0; i <= pos; i++){
        mySerialBuffer[i] = '\0';
      }
      
      // reset the counter
      pos = 0;

    } else {
      if(c >= '0' && c <= '9'){
//        Serial.println(c);
        mySerialBuffer[pos] = c;
        pos++;
      }      
    }    
  }
}
