#include <SFE_BMP180.h>
#include <Wire.h>

/*http://www.hobbytronics.co.uk/arduino-external-eeprom*/
/*cat /dev/ttyUSB0 > test
screen /dev/ttyUSB0 57600
tail -f test */
/***********************************************************/
/*                   SYSTEM VARIABLES                      */
/*---------------------------------------------------------*/

//SYSTEM
#define VERBOSE     1
#define SAMPLETIME  3000
#define ERRORCODE   222
float vcc       = 5.0;
unsigned int baudRate  = 57600;
int sampleRate  = 2000;
int errorFlag   = 0;
unsigned long time        = 0;
unsigned long lastControlTime = 0;
byte controlTask    = 0;
byte controlLatch   = 0;
unsigned long lastRecordTime  = 0;
unsigned long lastEventTime   = 0;

//EEPROM
#define EEADDR    0x50      /*Address of 24LC256 eeprom chip*/
#define EEMAXADD  32768     /* max addres in bytes          */
const int dataStartAdd  = 20;
const int eventStartAdd = 3;
const int currAddPtr    = 0; 
const int currEventPtr  = 2;
int currEvent;
unsigned int currAdd;

//HUMIDITY
int humidityPin     = A1;
float humidityRaw   = 0.0;
float humidity      = 0.0;  /* % of relative humidity     */
byte humidityMem    = 0;

//PRESSURE-TEMPERATURE
SFE_BMP180 bmp180;
double tempC  = 0.0;        /* temperature in C           */
byte tempCMem = 0;
double press  = 0.0;        /* absolute pressure in mBar  */
byte pressMem = 0;
char statdel  = 0;

//LED-BUTTON
int ledPin    = 9;
int buttonPin = 8;



/***********************************************************/
/*                      SYSTEM INIT                        */
/*---------------------------------------------------------*/

/**
  *
  *
  */
void setup() {
  
  Serial.begin(57600);
  if(VERBOSE){Serial.println("BAROMETIC PRESSURE LOGGER");} 

  Wire.begin();                                               //TODO -- alberto.bortoni: this is probably already in bmp 180 init
  pinMode(ledPin, OUTPUT);
  digitalWrite(ledPin, LOW); 
  pinMode(buttonPin, INPUT);
  
  if (bmp180.begin()== 0){                                                   //TODO -- alberto.bortoni: mark with led error before while
    if(VERBOSE){Serial.println("BMP180 init fail\n\n");}
    while(1){} // Pause forever.
  }
  
  if(VERBOSE){Serial.println("system init success");}       //TODO -- alberto.bortoni: have a flag to enable or disable serial

  time = millis();
}
/*------------------------------------------------*/


/**
  *
  *
  */
void loop() {

  while(controlLatch == 0){                              /*wait until button gets pressed*/
    if (digitalRead(buttonPin) == HIGH){
      lastControlTime = millis();
      controlLatch = 1;
      digitalWrite(ledPin, HIGH);
      delay(30);
    }
  }

  while(controlLatch == 1){                              /*hold button for 3 sec*/
    if (digitalRead(buttonPin) == LOW){
      controlLatch = 0;
      ledErrorDance();
      continue;
    }

    if(millis()-lastControlTime >= 3000){
      controlLatch = 2;
      digitalWrite(ledPin, LOW);
      delay(150);
      digitalWrite(ledPin, HIGH);
      delay(150);
      digitalWrite(ledPin, LOW);
    }
  }

  while(controlLatch == 2){                              /*wait until button gets depressed*/
    if (digitalRead(buttonPin) == LOW){
      lastControlTime = millis();
      controlLatch = 3;
      delay(30);
    }
  }

  while(controlLatch == 3){                             /*leave depressed button for 3 seconds*/
    if (digitalRead(buttonPin) == HIGH){
      controlLatch = 0;
      ledErrorDance();
    }

    if(millis()-lastControlTime >= 3000){
      controlLatch = 4;
      digitalWrite(ledPin, HIGH);
      delay(300);
      digitalWrite(ledPin, LOW);
      delay(300);
      digitalWrite(ledPin, HIGH);
      controlTask = 0;
      lastControlTime = millis();
    }
  }

  while(controlLatch == 4){                             /*count button pressing*/
    if((millis()-lastControlTime > 8000) && controlTask == 0){
      controlLatch = 0;
      ledErrorDance();
      continue;
    }

    if((millis()-lastControlTime > 8000) && controlTask != 0){
      controlLatch = 5;
      continue;
    }

    while(millis()-lastControlTime <= 8000){
      if(digitalRead(buttonPin) == HIGH){
        digitalWrite(ledPin, LOW);
        controlTask++;
        delay(30);
        while(digitalRead(buttonPin) == HIGH){}
        digitalWrite(ledPin, HIGH);
        delay(30);
        digitalWrite(ledPin, LOW);
      }
    }
  }

  while(controlLatch == 5){                                               //TODO -- foreverloop?
    switch (controlTask){
      case 1:                          /*sart/continue with recording   */
        if(VERBOSE){Serial.println("start recording");}
        recordData();
        break;
      case 2:
        if(VERBOSE){Serial.println("start data transmission");}
        transmitData();                          /*reset all data (just pointers) */ //TODO -- alberto.bortoni: mclear to 0?
        break;
      case 3:                          /*transmit data via RS232        */
        if(VERBOSE){Serial.println("clearing data and pointers");}
        resetData();
        break;
      default:
        break;
    }

  ledErrorDance();
  if(VERBOSE){Serial.println("program terminated");}
  Serial.end();
  while(1){}  
  }
}
/*------------------------------------------------*/



/**
  *
  *
  */
void recordData(){

  currAdd = readEEPROM(currAddPtr)<<8 | readEEPROM(currAddPtr+1);
  currEvent = readEEPROM(currEventPtr);


  lastRecordTime  = millis();                                 //TODO -- alberto.bortoni: add clock wraping
  lastEventTime   = millis();

  while(currAdd<EEMAXADD){
    if((millis()-lastRecordTime)>SAMPLETIME){
      lastRecordTime = millis();
      getTemperature();
      getHumidity();
      getPressure();
    
      if(errorFlag == 0){
        writeEEPROM(currAdd, tempCMem);
        currAdd++;
        writeEEPROM(currAdd, pressMem);
        currAdd++;
        writeEEPROM(currAdd, humidityMem);
        currAdd++;
      }
      else{                                                     //TODO -- alberto.bortoni: fill error handling
        writeEEPROM(currAdd, ERRORCODE);
        currAdd++;
        writeEEPROM(currAdd, ERRORCODE);
        currAdd++;
        writeEEPROM(currAdd, ERRORCODE);
        currAdd++;
      }
      
      writeEEPROM(currAddPtr, (currAdd>>8 & 0xFF));
      writeEEPROM(currAddPtr+1, (currAdd & 0xFF));
    
      if(VERBOSE){
        Serial.print("Mem ");
        Serial.print(currAdd-3);
        Serial.print("\tT: ");
        Serial.print(tempCMem/5, DEC);
        Serial.print("\tP: ");
        Serial.print(pressMem);
        Serial.print("\tH: ");
        Serial.print(humidityMem);
        Serial.print("\tTime: ");
        Serial.println(millis());
      }
    }


    if(digitalRead(buttonPin) == HIGH && currEvent<15 && (millis()-lastEventTime)>15000){
      lastEventTime = millis();
      digitalWrite(ledPin, HIGH);
      writeEEPROM(currEvent, currAdd);
      currEvent++;
      writeEEPROM(currEventPtr, currEvent);
      delay(20);
      digitalWrite(ledPin, LOW);

      if(currEvent == 15){ledErrorDance();}

      if(VERBOSE){
        Serial.print("Event at: ");
        Serial.println(currAdd);
      }
    }
  }
  return;
}
/*------------------------------------------------*/



/**
  *
  *
  */
void resetData(){
  unsigned int k = 0;

  for(k = 0; k<EEMAXADD; k++){
    writeEEPROM(k, 0);
    if(VERBOSE && k%500 == 0){
      Serial.print("cleared until ");
      Serial.println(k);
    }
  }

  writeEEPROM(currAddPtr, 0);
  writeEEPROM(currAddPtr+1, dataStartAdd);
  writeEEPROM(currEventPtr, eventStartAdd);
}
/*------------------------------------------------*/



/**
  *
  *
  */
void transmitData(){
  unsigned int cnt = 0;

  currAdd = readEEPROM(currAddPtr)<<8 | readEEPROM(currAddPtr+1);
  currEvent = readEEPROM(currEventPtr);
  digitalWrite(ledPin, HIGH);

  Serial.println("Press button to start trasmitting");
  Serial.println("Temperature: degC*5 Pressure:kPa Humidity: relative %\n");

  while(digitalRead(buttonPin) == LOW){}

  digitalWrite(ledPin, LOW);
  delay(300);
  digitalWrite(ledPin, HIGH);

  for(cnt = eventStartAdd; cnt <= currEvent; cnt++){
    Serial.print(readEEPROM(cnt));
    Serial.print(",");
  }

  Serial.println();

  for(cnt = dataStartAdd; cnt < currAdd; cnt+=3){
    Serial.print(readEEPROM(cnt));
    Serial.print(",");
    Serial.print(readEEPROM(cnt+1));
    Serial.print(",");
    Serial.print(readEEPROM(cnt+2));
    Serial.println(",");
  }
}
/*------------------------------------------------*/



/**
  *
  *
  */
void getHumidity(){
  humidityRaw = analogRead(humidityPin)*vcc/1023; 
  humidity = (humidityRaw*161.29/vcc - 25.4)/(1.0546 - 0.0026 * tempC);
  humidityMem = byte(humidity);
}
/*------------------------------------------------*/



/**
  *
  *
  */
void getTemperature(){
  statdel = bmp180.startTemperature();
  if (statdel == 0){
    errorFlag = 1;
    return;                                             //TODO -- alberto.bortoni: go to exception 
  }
    
  delay(statdel);

  statdel = bmp180.getTemperature(tempC);
  if (statdel == 0){
    errorFlag = 1;
    return;                                             //TODO -- alberto.bortoni: go to exception 
  }

  if(tempC>50){tempC  = 50;}
  if(tempC<0){tempC   = 0;}

  tempCMem = byte(tempC*5);
}
/*------------------------------------------------*/



/**
  *
  *
  */
void getPressure(){
  statdel = bmp180.startPressure(3);
  if (statdel == 0){
    errorFlag = 1;
    return;                                             //TODO -- alberto.bortoni: go to exception 
  }
  
  delay(statdel);

  statdel = bmp180.getPressure(press,tempC);
  if (statdel == 0){
    errorFlag = 1;
    return;                                             //TODO -- alberto.bortoni: go to exception 
  }
  pressMem = byte(press/10);
}
/*------------------------------------------------*/



/**
  *
  *
  */
void ledErrorDance(){
  digitalWrite(ledPin, HIGH);
  delay(100);
  digitalWrite(ledPin, LOW);
  delay(100);
  digitalWrite(ledPin, HIGH);
  delay(100);
  digitalWrite(ledPin, LOW);
  delay(100);
  digitalWrite(ledPin, HIGH);
  delay(100);
  digitalWrite(ledPin, LOW);
  delay(100);
  digitalWrite(ledPin, HIGH);
  delay(100);
  digitalWrite(ledPin, LOW);
  delay(100);
  digitalWrite(ledPin, HIGH);
  delay(100);
  digitalWrite(ledPin, LOW);
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
byte readEEPROM(unsigned int eeaddress){
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
