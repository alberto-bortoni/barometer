/*
  Analog Input
 Demonstrates analog input by reading an analog sensor on analog pin 0 and
 turning on and off a light emitting diode(LED)  connected to digital pin 13. 
 The amount of time the LED will be on and off depends on
 the value obtained by analogRead(). 
 
 The circuit:
 * Potentiometer attached to analog input 0
 * center pin of the potentiometer to the analog pin
 * one side pin (either one) to ground
 * the other side pin to +5V
 * LED anode (long leg) attached to digital output 13
 * LED cathode (short leg) attached to ground
 
 * Note: because most Arduinos have a built-in LED attached 
 to pin 13 on the board, the LED is optional.
 
 
 Created by David Cuartielles
 modified 30 Aug 2011
 By Tom Igoe
 
 This example code is in the public domain.
 
 http://arduino.cc/en/Tutorial/AnalogInput
 
 */

int sensorPin = A1;    // select the input pin for the potentiometer
int ledPin = 9;      // select the pin for the LED
int sensorValue = 0;  // variable to store the value coming from the sensor
float vcc = 5.0;
float vout = 0.0;
float rh = 0.0;
float rhcomp = 0.0;
float tempc = 25.0;

void setup() {
  Serial.begin(57600);                           /* Card reader baud rate, datasheet, init serial */
  // declare the ledPin as an OUTPUT:
  pinMode(ledPin, OUTPUT); 
  Serial.println("Set up Complete!");           /* Transmit welcome msg                         */ 
}

void loop() {
  // read the value from the sensor:
  sensorValue = analogRead(sensorPin);
  vout = sensorValue*vcc/1023; 
  rh = vout*161.29/vcc - 25.4;
  rhcomp = rh/(1.0546 - 0.0026 * tempc); 

  
  Serial.print(rhcomp);      
  Serial.println("%");   
  delay(500);          
                  
}
