#include <SFE_BMP180.h>
#include <Wire.h>
#include <EEPROM.h>

/*http://www.hobbytronics.co.uk/arduino-external-eeprom*/
/***********************************************************/
/*                   SYSTEM VARIABLES                      */
/*---------------------------------------------------------*/

//SYSTEM
float vcc       = 5.0;
int baudRate    = 57600;
int sampleRate  = 2000;
int errorFlag   = 0;
int time        = 0;
int lastControlTime = 0;
byte controlTask    = 0;
byte controlLatch   = 0;
int lastRecordTime  = 0;
int lastEventTime   = 0;

//EEPROM
#DEFINE EEADDR 0x50             /*Address of 24LC256 eeprom chip*/
const int dataStartAdd = 15;
const int eventStartAdd = 2;
const int currAddPtr = 0; 
const int currEventPtr = 1;
int currEvent = eventStartAdd;
int currAdd = dataStartAdd;

//HUMIDITY
int humidityPin     = A1;
float humidityRaw   = 0.0;
float humidity      = 0.0;  /* % of relative humidity     */
byte humidityMem     = 0;

//PRESSURE-TEMPERATURE
SFE_BMP180 bmp180;
double tempC  = 0.0;        /* temperature in C           */
byte tempCMem  = 0;
double press  = 0.0;        /* absolute pressure in mBar  */
byte pressMem  = 0;
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
  
  pinMode(ledPin, OUTPUT);
  digitalWrite(ledPin, LOW); 
  pinMode(buttonPin, INPUT);
  
  if (bmp180.begin()){
    Serial.println("BMP180 init success");                    //TODO -- alberto.bortoni: have a flag to enable or disable serial
  }
  else {                                                      //TODO -- alberto.bortoni: mark with led error before while
    Serial.println("BMP180 init fail\n\n");
    while(1); // Pause forever.
  }

  time = millis();
}
/*------------------------------------------------*/


/**
  *
  *
  */
void loop() {

  if (digitalRead(buttonPin) == HIGH && controlLatch == 0){
    lastControlTime = millis();
    controlLatch = 1;
    digitalWrite(ledPin, HIGH);
  }

  if (digitalRead(buttonPin) == LOW && controlLatch == 1){
    controlLatch = 0;
    ledErrorDance();
  }

  while(digitalRead(buttonPin) == HIGH && controlLatch == 1){
    if(millis()-lastControlTime >= 3000){
      controlLatch = 2
      digitalWrite(ledPin, LOW);
      delay(300);
      digitalWrite(ledPin, HIGH);
      delay(300);
      digitalWrite(ledPin, LOW);
    }
  }

  if (digitalRead(buttonPin) == LOW && controlLatch == 2){
    lastControlTime = millis();
    controlLatch = 3;
  }

  if (digitalRead(buttonPin) == HIGH && controlLatch == 3){
    controlLatch = 0;
    ledErrorDance();
  }

  while(digitalRead(buttonPin) == LOW && controlLatch == 3){
    if(millis()-lastControlTime >= 3000){
      controlLatch = 4
      digitalWrite(ledPin, HIGH);
      delay(300);
      digitalWrite(ledPin, LOW);
      delay(300);
      digitalWrite(ledPin, HIGH);
      controlTask = 0;
      lastControlTime = millis();
    }
  }

  if((millis()-lastControlTime > 8000) && controlLatch == 4){
    controlLatch = 0;
    ledErrorDance();
  }

  while((millis()-lastControlTime <= 8000) && controlLatch == 4){
    if(digitalRead(buttonPin) == HIGH){
      controlLatch = 5;
      controlTask = 1;
      delay(30);
      while(digitalRead(buttonPin) == HIGH){}
      delay(30);
    }
  }

  if(controlLatch == 5){
    switch (controlTask){
      case 1:                          /*sart/continue with recording   */
        recordData();
        break;
      case 2:                          /*reset all data (just pointers) */
        resetData();
        break;
      case 2:                          /*transmit data via RS232        */
        transmitData();
        break;
      default:
        break;
    }
  }

  ledErrorDance();
  return 0;
}
/*------------------------------------------------*/



/**
  *
  *
  */
void recordData(){

  currAdd = EEPROM.read(currAddPtr);
  currEvent = EEPROM.read(currEventPtr);


  lastRecordTime  = millis();
  lastEventTime   = millis();

  while(currAddPtr<1020){
    if((millis()-lastRecordTime)>5000){
      lastRecordTime = millis();
      getTemperature();
      getHumidity();
      getPressure();
    
      if(errorFlag == 0){
        EEPROM.update(currAdd, tempCMem);
        currAdd++;
        EEPROM.update(currAdd, pressMem);
        currAdd++;
        EEPROM.update(currAdd, humidityMem);
        currAdd++;
        EEPROM.update(currAddPtr, currAdd);
      }
      else{                                                     //TODO -- alberto.bortoni: fill error handling

      }
    }

    if(digitalRead(buttonPin) == HIGH && currEvent<15 %% (millis()-lastEventTime)>15000){
      lastEventTime = millis();
      digitalWrite(ledPin, HIGH);
      EEPROM.update(currEvent, currAdd);
      currEvent++
      if(currEvent<15){
        EEPROM.update(currEventPtr, currEvent);
      }
      delay(20);
      digitalWrite(ledPin, LOW);
      if(currEvent == 15){ledErrorDance();}
    }

  }
  return 0;
}
/*------------------------------------------------*/



/**
  *
  *
  */
void resetData(){
  EEPROM.update(currAddPtr, dataStartAdd);
  EEPROM.update(currEventPtr, eventStartAdd);
}
/*------------------------------------------------*/



/**
  *
  *
  */
void transmitData(){
  int cnt = 0;
  Serial.println("Press button to start trasmitting\n\n");

  while(digitalRead(buttonPin) == LOW){}

  digitalWrite(ledPin, HIGH);
  delay(300);
  digitalWrite(ledPin, LOW);

  for(cnt = eventStartAdd; cnt < dataStartAdd; cnt++){
    Serial.print(EEPROM.read(cnt));
    Serial.print(", ");
  }

  for(cnt = dataStartAdd; cnt < 1024; cnt+=3){
    Serial.print(EEPROM.read(cnt));
    Serial.print(", ");
    Serial.print(EEPROM.read(cnt+1));
    Serial.print(", ");
    Serial.print(EEPROM.read(cnt+2));
    Serial.print(",");
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
    return 0;                                             //TODO -- alberto.bortoni: go to exception 
  }
    
  delay(statdel);

  statdel = bmp180.getTemperature(tempC);
  if (statdel == 0){
    errorFlag = 1;
    return 0;                                             //TODO -- alberto.bortoni: go to exception 
  }
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
    return 0;                                             //TODO -- alberto.bortoni: go to exception 
  }
  
  delay(statdel);

  statdel = bmp180.getPressure(press,tempC);
  if (statdel == 0){
    errorFlag = 1;
    return 0;                                             //TODO -- alberto.bortoni: go to exception 
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
  Wire.send((int)(eeaddress >> 8));   // MSB
  Wire.send((int)(eeaddress & 0xFF)); // LSB
  Wire.send(data);
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
  Wire.send((int)(eeaddress >> 8));   // MSB
  Wire.send((int)(eeaddress & 0xFF)); // LSB
  Wire.endTransmission();
 
  Wire.requestFrom(EEADDR,1);
 
  if (Wire.available()) rdata = Wire.receive();
 
  return rdata;
}
/*------------------------------------------------*/

/*EOF*/