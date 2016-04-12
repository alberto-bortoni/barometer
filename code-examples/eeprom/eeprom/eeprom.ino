
#define EEADDR 0x50
#include <Wire.h>

int led = 9;
int button = 8;
int k;
int add;


/**
  *
  *
  */
void setup() {                
  pinMode(led, OUTPUT);
  pinMode(button, INPUT);  
  Wire.begin();
  Serial.begin(57600);
  Serial.println("init success");
  
}
/*------------------------------------------------*/



/**
  *
  *
  */
void loop() {
  
  while(digitalRead(button) == LOW) {}
  digitalWrite(led, HIGH);

  add = readEEPROM(0);
  Serial.println(readEEPROM(0));
  
  for(k =add; k<(25+add); k++){
    writeEEPROM(k, k);
  }
  
  for(k =add; k<(25+add); k++){
    Serial.println(readEEPROM(k));
  }
  
  writeEEPROM(0, k);
  
  while(1) {}
}
/*------------------------------------------------*/



/**
  *
  *
  */
void writeEEPROM(unsigned int eeaddress, byte data){
  Wire.beginTransmission(EEADDR);
  Wire.write((int)(eeaddress >> 8));   // MSB
  Wire.write((int)(eeaddress & 0xFF)); // LSB
  Wire.write(data);
  Wire.endTransmission();
 
  delay(5);
}
/*------------------------------------------------*/

 

/**
  *
  *
  */
byte readEEPROM(unsigned int eeaddress ){
  byte rdata = 0xFF;
 
  Wire.beginTransmission(EEADDR);
  Wire.write((int)(eeaddress >> 8));   // MSB
  Wire.write((int)(eeaddress & 0xFF)); // LSB
  Wire.endTransmission();
 
  Wire.requestFrom(EEADDR,1);
 
  if (Wire.available()) rdata = Wire.read();
 
  return rdata;
}
/*------------------------------------------------*/
