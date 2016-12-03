/*****************************************************************/

/*   created by: alberto bortoni
 *   date:       2016-11-27
 *
 * hardware:
 *   - Arduino pro-mini 5V 16Mhz atmega 328Mhz
 *   - pressure sensor BMP180 on sparkfun SEN-11824 breakout
 *   - humidity sensor HIH-4030 on sparkfun SEN-09569 berakout
 *   - eeprom AT24C256
 *
 * the program saves data of temperature, pressure, and humidity
 * on the eeprom memory, a start and end pointer, retrieved form
 * the reserved space in the eeprom, tracks the data recorded.
 * the interval between samples is defined by SAMPLETIME in ms
 * In the header, change the baudrate and verbose output; this
 * outputs stuff when debugging.
 * the data transmitted contains a small header with explanations
 * for interpreting the data.
 * Currently, there is no error handling implementation.
 * using the program
 *   - one button interface, trun on with switch
 *   - press button for 5 sec, wait for light to trun on/off
 *   - unpress button for 5 sec, wait for light to turn on/off
 *   - press either:
 *     - 1 time, for logging; starts at pointer where it was left
 *     - 2 times, transmits data between start to end pointer
 *     - 3 times, resets data by reasigning the start pointer to
 *       the current pointer on eeprom
 *     - 4 times, clears all data on eeprom and sets to 0
 *
 *  other useful linux commands:
 *    cat /dev/ttyUSB0 > test
 *    tail -f test
 *    screen /dev/ttyUSB0 57600
 *
 *  Currently device consumes 48ma max, and it has a 1000mah batt
 *  roughly 18~20h of use
 *
 *  For full list of materials and other documentation visit
 *  my website at bortoni.mx
*/
/*---------------------------------------------------------------*/

#include <SFE_BMP180.h>
#include <Wire.h>

/***********************************************************/
/*                   SYSTEM VARIABLES                      */
/*---------------------------------------------------------*/

//SYSTEM
#define VERBOSE     0
#define BAUDRATE    57600;
#define SAMPLETIME  6000
#define ERRORCODE   222

float vcc       = 5.0;
int sampleRate  = 2000;
int errorFlag   = 0;
unsigned long time  = 0;
unsigned long lastControlTime = 0;
byte controlTask    = 0;
byte controlLatch   = 0;
unsigned long lastRecordTime  = 0;
unsigned long lastEventTime   = 0;

//EEPROM
#define EEADDR    0x50      /*Address of 24LC256 eeprom chip*/
#define EEMAXADD  32768     /* max addres in bytes          */
const int dataStartAdd  = 40;
const int eventStartAdd = 3;
const int currAddPtr    = 0;
const int currEventPtr  = 2;
unsigned int currEvent;
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

void setup() {

  Serial.begin(57600);
  if(VERBOSE){Serial.println("BAROMETIC PRESSURE LOGGER");}

  Wire.begin();
  pinMode(ledPin, OUTPUT);
  digitalWrite(ledPin, LOW);
  pinMode(buttonPin, INPUT);

  if (bmp180.begin()== 0){
    if(VERBOSE){Serial.println("BMP180 init fail\n\n");}
    while(1){} // Pause forever.
  }

  if(VERBOSE){Serial.println("system init success");}

  time = millis();
}
/*------------------------------------------------*/



void loop() {

  while(controlLatch == 0){                 /*wait until button gets pressed*/
    if (digitalRead(buttonPin) == HIGH){
      lastControlTime = millis();
      controlLatch = 1;
      digitalWrite(ledPin, HIGH);
      delay(30);
    }
  }

  while(controlLatch == 1){                 /*hold button for 3 sec*/
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

  while(controlLatch == 2){                /*wait until button gets depressed*/
    if (digitalRead(buttonPin) == LOW){
      lastControlTime = millis();
      controlLatch = 3;
      delay(30);
    }
  }

  while(controlLatch == 3){                /*leave depressed button for 3 seconds*/
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

  while(controlLatch == 4){                /*count button pressing*/
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

  while(controlLatch == 5){
    digitalWrite(ledPin, LOW);
    delay(150);
    digitalWrite(ledPin, HIGH);
    delay(150);
    digitalWrite(ledPin, LOW);

    switch (controlTask){
      case 1:                          /*sart/continue with recording*/
        if(VERBOSE){Serial.println("start recording");}
        recordData();
        break;
      case 2:
        if(VERBOSE){Serial.println("start data transmission");}
        transmitData();
        break;
      case 3:                          /*just reset pointers       */
        if(VERBOSE){Serial.println("resetting data and pointers");}
        resetData();
        break;
      case 4:                          /*wipe out all data, clear  */
        if(VERBOSE){Serial.println("clearing data and pointers");}
        clearData();
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



void recordData(){

  currAdd   = readEEPROM(currAddPtr)<<8 | readEEPROM(currAddPtr+1);
  currEvent = readEEPROM(currEventPtr);

  lastRecordTime  = millis();
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
      else{
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
        Serial.print((currAdd - dataStartAdd)/3 - 1);
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


    if(digitalRead(buttonPin) == HIGH && currEvent<37 && (millis()-lastEventTime)>15000){
      lastEventTime = millis();
      digitalWrite(ledPin, HIGH);
      writeEEPROM(currEvent, (currAdd>>8 & 0xFF));
      writeEEPROM(currEvent+1, (currAdd & 0xFF));
      currEvent+=2;
      writeEEPROM(currEventPtr, currEvent);
      delay(20);
      digitalWrite(ledPin, LOW);

      if(currEvent == 37){ledErrorDance();}

      if(VERBOSE){
        Serial.print("Event at: ");
        Serial.println((currAdd - dataStartAdd)/3);
      }
    }

    if (currAdd%150 == 0){                  /* visual feedback to know device is working */
      digitalWrite(ledPin, HIGH);
      delay(300);
      digitalWrite(ledPin, LOW);
      }

  }
  return;
}
/*------------------------------------------------*/



void resetData(){

  writeEEPROM(currAddPtr, 0);
  writeEEPROM(currAddPtr+1, dataStartAdd);
  writeEEPROM(currEventPtr, eventStartAdd);
}
/*------------------------------------------------*/



void clearData(){
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



void transmitData(){
  unsigned int cnt = 0;

  currAdd = readEEPROM(currAddPtr)<<8 | readEEPROM(currAddPtr+1);
  currEvent = readEEPROM(currEventPtr);
  digitalWrite(ledPin, HIGH);

  Serial.println("Press button to start trasmitting");
  Serial.print("Temperature: degC*5; Pressure:kPa; Humidity: relative %; time sec: ");
  Serial.println(SAMPLETIME/1000);
  Serial.println();

  while(digitalRead(buttonPin) == LOW){}

  digitalWrite(ledPin, LOW);
  delay(300);
  digitalWrite(ledPin, HIGH);

  for(cnt = eventStartAdd; cnt < currEvent; cnt+=2){
    Serial.print(((readEEPROM(cnt)<<8 | readEEPROM(cnt+1)) - dataStartAdd)/3);
    Serial.print(",");
  }

  Serial.println();

  for(cnt = dataStartAdd; cnt < currAdd; cnt+=3){
    Serial.print(readEEPROM(cnt));
    Serial.print(",");
    Serial.print(readEEPROM(cnt+1));
    Serial.print(",");
    Serial.print(readEEPROM(cnt+2));
    Serial.println();
  }
}
/*------------------------------------------------*/



void getHumidity(){
  humidityRaw = analogRead(humidityPin)*vcc/1023;
  humidity = (humidityRaw*161.29/vcc - 25.4)/(1.0546 - 0.0026 * tempC);
  humidityMem = byte(humidity);
}
/*------------------------------------------------*/



void getTemperature(){
  statdel = bmp180.startTemperature();
  if (statdel == 0){
    errorFlag = 1;
    return;
  }

  delay(statdel);

  statdel = bmp180.getTemperature(tempC);
  if (statdel == 0){
    errorFlag = 1;
    return;
  }

  if(tempC>50){tempC  = 50;}
  if(tempC<0){tempC   = 0;}

  tempCMem = byte(tempC*5);
}
/*------------------------------------------------*/



void getPressure(){
  statdel = bmp180.startPressure(3);
  if (statdel == 0){
    errorFlag = 1;
    return;
  }

  delay(statdel);

  statdel = bmp180.getPressure(press,tempC);
  if (statdel == 0){
    errorFlag = 1;
    return;
  }
  pressMem = byte(press/10);
}
/*------------------------------------------------*/



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



void writeEEPROM(unsigned int eeaddress, byte data){
  Wire.beginTransmission(EEADDR);
  Wire.write((int)(eeaddress >> 8));   // MSB
  Wire.write((int)(eeaddress & 0xFF)); // LSB
  Wire.write(data);
  Wire.endTransmission();

  delay(5);
}
/*------------------------------------------------*/



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
