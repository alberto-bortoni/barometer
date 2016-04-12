#include <Wire.h>

int incomingByte = 0;   // for incoming serial data

//EEPROM
#define EEADDR    0x50      /*Address of 24LC256 eeprom chip*/
#define EEMAXADD  32768     /* max addres in bytes          */
const int dataStartAdd  = 20;
const int eventStartAdd = 4;
const int currAddPtr    = 0; 
const int currEventPtr  = 1;
unsigned int currEvent;
unsigned int currAdd;


void setup() {
  Serial.begin(57600);     // opens serial port, sets data rate to 9600 bps
  Serial.println("type the memory that you want to read \n");
}
/*------------------------------------------------*/


void loop() {

  if (Serial.available() > 0) {

    incomingByte = Serial.parseInt();

    if(incomingByte<0){incomingByte = 0;}
    if(incomingByte>EEMAXADD){incomingByte = EEMAXADD;}

	  Serial.print("Memory ");
	  Serial.print(incomingByte, DEC);
	  Serial.print(" is ");
    Serial.println(readEEPROM(incomingByte));
  }
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

/*EOF*/