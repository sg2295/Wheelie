#include <Wire.h>
#include <Servo.h>
//(Motor Controller Used: L293D)
//Pins for Left Motor
const int leftMotorPin1  = 6; //Corresponds to Pin 14 of L293D
const int leftMotorPin2  = 5; //Corresponds to Pin 10 of L293D
//Pins for right Motor
const int rightMotorPin1  = 3; //Corresponds to Pin  7 of L293D
const int rightMotorPin2  = 11; //Corresponds to Pin  2 of L293D

//Pin for piezo
const int piezoPin = 7;

//Pins for front LEDs
const int torchPins[] = {2, 4};

//Pin for IR sensor (Infrared Sensor Used: 2Y0A21 Sharp Sensor)
const int distanceSensorPin = A0;

//Pins for servos
const int servoVerticalPin = 13;
const int servoHorizontalPin = 12;
//Declare Servo Objects
Servo servoVertical; //Servo used to control the vertical position of the arm
Servo servoHorizontal; //Servo used to control the horizontal position of the arm

//Variables used to control the vertical Servo
const int passiveVerticalAngle = 25; //The default value the vertical Servo is pointing to
const int activeVerticalAngle = 130; //The value the vertical Servo is pointning to when the activateArm function is called
//Variables used to control the vertical Servo
const int passiveHorizontalAngle = 150; //The default value the horizontal Servo is pointing to
const int activeHorizontalAngle = 20; //The value the horizontal Servo is going to reach when activateArm function is called
bool armFlag = false;

//Variables used to manipulate motor speed when turning
const float turnMultiplier = 0.5347;
float rightTurnMultiplier = 1;
float leftTurnMultiplier = 1;

//Variable used to define the constant sound of the Piezo Speaker
const int piezoFrequency = 175;

void setup() {
  Wire.begin(8); //Use the I2C bus as a slave
  
  Wire.onReceive(receiveEvent); //Calls the receiveEvent function when bytes are transmited from the master to the slave
  Wire.onRequest(requestEvent); //Calls the requestEvent function when the master requests data from the slave
  
  //pinMode for Motor Pins
  pinMode(leftMotorPin1, OUTPUT);
  pinMode(leftMotorPin2, OUTPUT);
  pinMode(rightMotorPin1, OUTPUT);
  pinMode(rightMotorPin2, OUTPUT);
  
  //pinMode for Piezo Speaker
  pinMode(piezoPin, OUTPUT);
  
  //pinMode for Torches
  for(int i = 0; i < 2; i++){
    pinMode(torchPins[i], OUTPUT);
  }
  
  //pinMode for Infrared Distance Sensor
  pinMode(distanceSensorPin, INPUT);

  //Attach Servos to the Servo Pins
  servoVertical.attach(servoVerticalPin);
  servoHorizontal.attach(servoHorizontalPin);
  servoVertical.write(passiveVerticalAngle);
  servoHorizontal.write(passiveHorizontalAngle);
}

void loop() {
  if(armFlag){
    activateArm(); //Only called when the armFlag boolean is true
  }
}

/**
 * Called when data is requested from the master via the I2C Bus
 * Uses the Wire.write function to send the infrared distance sensor reading to the master
 */
void requestEvent(){
  Wire.write(getDistanceSensorReading());
}

/**
 * Called when data is received from the master via the I2C Bus
 * Uses the Wire.available function to find the number of bytes that can be read
 * Uses the Wire.read function to read the available bytes
 */
void receiveEvent(){
  if(Wire.available() == 6){ //Only accept transmitions of 6 bytes
    //The first three bytes received are used to set the direction and speed of the vehicle
    char driveMode = Wire.read();
    int driveSpeed = Wire.read();
    char driveDirection = Wire.read();
    driveModeSelection(driveMode, driveSpeed, driveDirection);

    //The fourth byte is used to active/deactivate the Piezo Speaker
    char hornState = Wire.read();
    setHornState(hornState);

    //The fifth byte is used to control the slave's LEDs
    char torchState = Wire.read();
    setTorchState(torchState);

    //The sixth byte is used to control the Servos
    char armState = Wire.read();
    setArmState(armState);
  }
}

/**
 * Gets the distance sensor reading from the infrared distance sensor
 * Maps the reading to a value acceptable to be transmitted to the master (0 - 255)
 * Returns the mapped reading to be transmitted over the I2C Bus to the master
 */
byte getDistanceSensorReading(){
  int distanceSensorReading = analogRead(distanceSensorPin); //Gets a value from 0 - 700
  distanceSensorReading = map(distanceSensorReading, 0, 700, 0, 255); //Maps the sensor reading to 0 - 255
  return (byte)distanceSensorReading; //Return the value in then form of a byte
}

/**
 * Sets the vehicle's direction and speed
 */
void driveModeSelection(char driveMode, int driveSpeed, char driveDirection){
  if(driveMode == 'F'){ //'F' is used to signify forwards
    editTurnMultiplier(driveDirection); //Edit the turn multipliers to adjust for a Right/Left turn command
    forward(driveSpeed);
  }else if(driveMode == 'B'){ //'B' is used to signify backwards
    editTurnMultiplier(driveDirection);
    backward(driveSpeed);
  }else if(driveMode == 'N'){ //'N' is used for neutral
    neutral();
  }
}

/**
 * Changes the right/left turn multipliers to make the vehicle turn
 */
void editTurnMultiplier(char driveDirection){
  if(driveDirection == 'S'){ //'S' signifies straight (no turns)
      rightTurnMultiplier = 1;
      leftTurnMultiplier = 1;
    }else if(driveDirection == 'R'){ //'R' signifies a right turn
      rightTurnMultiplier = turnMultiplier; //Used to decrease the right motor's speed
      leftTurnMultiplier = 1;
    }else if(driveDirection == 'L'){ //'L' signifies a left turn
      rightTurnMultiplier = 1;
      leftTurnMultiplier = turnMultiplier; //Used to decrease the right motor's speed
    }
}

/**
 * Use analogWrite to set the speed of the corresponding motor pins to make the vehicle move forwards
 * Called when 'F' is received from the master
 */
void forward(int driveSpeed){
    analogWrite(leftMotorPin1, driveSpeed * leftTurnMultiplier); //Multiply the given driveSpeed by the turn multiplier to compensate for any turn
    analogWrite(leftMotorPin2, 0);
    analogWrite(rightMotorPin1, 0);
    analogWrite(rightMotorPin2, driveSpeed * rightTurnMultiplier);
}

/**
 * Use analogWrite to set the speed of all motor pins to zero, stopping the vehicle from moving
 * Called when 'N' is received from the master
 */
void neutral(){
  analogWrite(leftMotorPin1, 0);
  analogWrite(leftMotorPin2, 0);
  analogWrite(rightMotorPin1, 0);
  analogWrite(rightMotorPin2, 0);
}

/**
 * Use analogWrite to set the speed of the corresponding motor pins to make the vehicle move backwards
 * Called when 'B' is received from the master
 */
void backward(int driveSpeed){
  analogWrite(leftMotorPin1, 0);
  analogWrite(leftMotorPin2, driveSpeed * rightTurnMultiplier); //Multiply the given driveSpeed by the turn multiplier to compensate for any turn
  analogWrite(rightMotorPin1, driveSpeed * leftTurnMultiplier);
  analogWrite(rightMotorPin2, 0);
}

/**
 * Used to control the Piezo Speaker
 * Uses the tone and noTone functions to activate/deactive the Piezo Speaker
 * When an 'H' is received play a sound on the Piezo
 */
void setHornState(char hornState){
  if(hornState == 'H'){ //'H' corresponds to the turning on of the Piezo Speaker
    tone(piezoPin, piezoFrequency);
  }else if(hornState == 'h'){ //'h' corresponds to the turning off of the Piezo Speaker
    noTone(piezoPin);
  }
}

/**
 * Used to turn the LEDs ON or OFF
 * When a 'T' is sent by the master turn the LEDs ON
 */
void setTorchState(char torchState){
  if(torchState == 'T'){ //'T' signifies the turning ON of the LEDs
    for(int i = 0; i < 2; i++){
      digitalWrite(torchPins[i], HIGH);
    }
  }else if(torchState == 't'){ //'t' signifies the turning OFF of the LEDs
    for(int i = 0; i < 2; i++){
      digitalWrite(torchPins[i], LOW);
    }
  }
}

/**
 * Changes the value of a boolean variable (armFlag) to true if an 'A' is received
 * or to false when an 'a' is received
 */
void setArmState(char armState){
  //armFlag is used to call the activateArm() function in the loop
  if(armState == 'A'){
    armFlag = true;
  }else if(armState == 'a'){
    armFlag = false;
  }
}

/**
 * Called when an armFlag is true
 * Used to activate the Servos' movements
 */
void activateArm(){
  servoVertical.write(activeVerticalAngle); //Change the angle of the Vertical Servo to its Active value (Lowers the arm)
  delay(200);
  //Gradually increment the angle at which the Horizontal Servo is pointing to:
  for(int angle = passiveHorizontalAngle; angle >= activeHorizontalAngle; angle--){ //(Moves the arm Horizontally)
    servoHorizontal.write(angle); //Change the angle of the Horizontal Servo from its Passive to its Active value
    delay(4);
  }
  delay(50);
  for(int angle = activeHorizontalAngle; angle <= passiveHorizontalAngle; angle++){
    servoHorizontal.write(angle); //Change the angle of the Horizontal Servo from its Active to its Passive value
    delay(4);
  }
  delay(200);
  servoHorizontal.write(passiveHorizontalAngle);
  servoVertical.write(passiveVerticalAngle); //Change the angle of the Vertical Servo to its Passive value (Raises the arm)
}
