/*
  Blink
  Turns on an LED on for one second, then off for one second, repeatedly.
 
  This example code is in the public domain.
 */
 
// Pin 13 has an LED connected on most Arduino boards.
// give it a name:
int led = 9;
int button = 8;
int buttonState = 0;

// the setup routine runs once when you press reset:
void setup() {                
  // initialize the digital pin as an output.
  pinMode(led, OUTPUT);
  pinMode(button, INPUT);  
  Serial.begin(57600);
}

// the loop routine runs over and over again forever:
void loop() {
  buttonState = digitalRead(button);
  
  if (buttonState == HIGH){
    digitalWrite(led, HIGH);
    Serial.println("pressed");
    delay(30);
  }
  else{
    digitalWrite(led, LOW);    // turn the LED off by making the voltage LOW
  }
}
