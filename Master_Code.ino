#include <Wire.h> //For communication

//Analog input pins for selecting the driving mode
const int forwardButtonPin = A0;
const int neutralButtonPin = A1;
const int backwardButtonPin = A2;
const int driveModeInterruptPin = 2; //Interrupt pin for any driving mode button press

//Analog input pin for drive speed control
const int potentiometerPin = A3;

//Pins for driving mode LEDs
const int backwardLedPin = 10;
const int forwardLedPin = 11;

//Pin for torch button press
const int torchButtonInterruptPin = 3;

//Pin for horn button press
const int hornButtonPin = 8;

//Pin for torch LED
const int torchLedPin = 9;

//Pins for left / right button press
const int rightButtonPin = 5;
const int leftButtonPin = 7;

//Minimum distance reading for arm command to be sent
const int minDistanceReading = 125;

//Number of milliseconds to delay between sending
const int sendDelayTime = 100;

//Commands to be sent which are changed in interrupt functions
volatile byte driveModeCommand = 'N';
volatile byte torchCommand = 't';

//Counts the number of times in a row which the distance sensor reading is above the minimum
int distanceCount = 0;

//Stores the current potentiometer reading
int potentiometerValue = 0;

//Stores the current distance sensor reading from the vehicle
int distanceSensorValue = 0;

void setup() {
  Wire.begin(); //Uses the I2C bus as a master

  //Sets pin modes to input for the control buttons and potentiometer
  pinMode(forwardButtonPin, INPUT);
  pinMode(neutralButtonPin, INPUT);
  pinMode(backwardButtonPin, INPUT);
  pinMode(driveModeInterruptPin, INPUT);
  pinMode(potentiometerPin, INPUT);
  pinMode(leftButtonPin, INPUT);
  pinMode(rightButtonPin, INPUT);
  pinMode(hornButtonPin, INPUT);
  pinMode(torchButtonInterruptPin, INPUT);

  //Sets pin modes to output for drive mode and torch LEDs
  pinMode(torchLedPin, OUTPUT);
  pinMode(backwardLedPin, OUTPUT);
  pinMode(forwardLedPin, OUTPUT);

  //Attaches an interrupt service routine function to each interrupt pin. The interrupts occur when the buttons are pressed
  attachInterrupt(digitalPinToInterrupt(driveModeInterruptPin), driveModeInterrupt, FALLING);
  attachInterrupt(digitalPinToInterrupt(torchButtonInterruptPin), torchInterrupt, FALLING);
}

void loop() {
  getDistanceSensorValue(); //Gets a distance sensor reading from the vehicle
  transmitData(); //Transmits data to control the vehicle
}

//Function to get distance sensor reading from the vehicle
void getDistanceSensorValue() {
  Wire.requestFrom(8, 1); //Requests 1 byte from the vehicle
  if (Wire.available() == 1) { //Reads the transmission if there is 1 byte available to read
    distanceSensorValue = Wire.read();
  }
}

/**
 * Function transmits all commands
 */
void transmitData() {
  Wire.beginTransmission(8); //Begins a transmission to the vehicle

  //Sends 6 bytes (1 per command) to the vehicle
  Wire.write(driveModeCommand); //To control forward / neutral / backward
  Wire.write(getDriveSpeedCommand()); //To control speed of movement
  Wire.write(getDriveDirectionCommand()); //To control left / right
  Wire.write(getHornCommand()); //To control if horn is on / off
  Wire.write(torchCommand); //To control if torch is on / off
  Wire.write(getArmCommand()); //To turn arm on (if not already on) / off

  delay(sendDelayTime); //Gives the vehicle time to process commands
  Wire.endTransmission(); //Ends transmission
}

/**
 * Returns the drive speed command to send to vehicle
 */
byte getDriveSpeedCommand() {
  int tempDriveSpeedCommand = 0;
  //If in neutral, speed = 0. If not, get potentiometer reading and map it to range 0-255
  if (driveModeCommand != 'N') {
    potentiometerValue = analogRead(potentiometerPin);
    tempDriveSpeedCommand = map(potentiometerValue, 0, 1023, 0, 255);
  }
  return tempDriveSpeedCommand; //Return the result as a byte
}

/**
 * Returns the direction command byte to send to the vehicle (left / right / straight)
 */
byte getDriveDirectionCommand() {
  if (digitalRead(leftButtonPin) == LOW) { //If left button pressed, return 'L'
    return 'L';
  } else if (digitalRead(rightButtonPin) == LOW) { //If right button pressed, return 'R'
    return 'R';
  } else { //If neither button pressed, return 'S'
    return 'S';
  }
}

/**
 * Return command byte to turn horn on / off
 */
byte getHornCommand() {
  if (digitalRead(hornButtonPin) == LOW) { //If horn button is pressed, return 'H'
    return 'H';
  } else { //If not pressed, return 'h'
    return 'h';
  }
}

/**
 * Return command byte to turn arm on / off
 */
byte getArmCommand() {
  if (distanceSensorValue >= minDistanceReading) { //If an object is within range
    if (distanceCount < 2) { //If object hasn't been within range 2 times in a row, return 'a'
      distanceCount++;
      return 'a';
    } else { //If object has been within range 3 times in a row, return 'A'
      distanceCount = 0;
      return 'A';
    }
  } else { //If object not in range, return 'a'
    distanceCount = 0;
    return 'a';
  }
}

/**
 * Function to handle interrupt when any drive mode button is pressed
 */
void driveModeInterrupt() {
  //Detects which button was pressed using individual pins
  if (digitalRead(forwardButtonPin) == LOW) { //If forward button pressed, set command to 'F' and turn on forward LED
    driveModeCommand = 'F';
    digitalWrite(forwardLedPin, HIGH);
    digitalWrite(backwardLedPin, LOW);
  } else if (digitalRead(neutralButtonPin) == LOW) { //If neutral button pressed, set command to 'N' and turn off both LEDs
    driveModeCommand = 'N';
    digitalWrite(forwardLedPin, LOW);
    digitalWrite(backwardLedPin, LOW);
  } else if (digitalRead(backwardButtonPin) == LOW) { //If backward button pressed, set command to 'B' and turn on backward LED
    driveModeCommand = 'B';
    digitalWrite(forwardLedPin, LOW);
    digitalWrite(backwardLedPin, HIGH);
  }
}

/**
 * Function to handle interrup if torch button is pressed
 */
void torchInterrupt() {
  if (torchCommand == 't') { //If current command is 't', change to 'T' and turn on controller torch LED
    torchCommand = 'T';
    digitalWrite(torchLedPin, HIGH);
  } else { //If command isn't 't', change to 't' and turn off controller torch LED
    torchCommand = 't';
    digitalWrite(torchLedPin, LOW);
  }
}
