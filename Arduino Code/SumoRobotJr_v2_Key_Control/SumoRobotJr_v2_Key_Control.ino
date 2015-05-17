//<legalstuff> This work is licensed under a Creative Commons Attribution 4.0 International License. Visit http://creativecommons.org/licenses/by/4.0/ for details. </legalstuff>
//Arduino code for SumoBot Jr. v2, created by Bryce Peterson (Nickname: Pecacheu, Email: Pecacheu@gmail.com). (Since no code was ever written for the original)

// avr-libc library includes:
#include <avr/io.h>
#include <avr/interrupt.h>

#define NUM_MOTORS 2
#define STATUS_LED 13

//Pin Numbers:
#define DIR_1  8
#define STEP_1 9
#define SLP_1  10

#define CONFIG1_M0 3
#define CONFIG1_M1 2

#define DIR_2  5
#define STEP_2 6
#define SLP_2  7

#define CONFIG2_M0 12
#define CONFIG2_M1 11

//Globals: (Don't touch these!)
int savedStepMode = 1;
boolean savedBrakeStates[2] = {true, true};
volatile unsigned int motorPins[2] = {STEP_1, STEP_2};
volatile unsigned int driveMotorSpeed[2] = {0, 0};
volatile unsigned int driveMotorCount[2] = {0, 0};

void setup() {
  //Setup Pins:
  pinMode(STATUS_LED, OUTPUT);
  pinMode(STEP_1, OUTPUT);
  pinMode(DIR_1,  OUTPUT);
  pinMode(SLP_1, OUTPUT);
  digitalWrite(SLP_1, HIGH);
  pinMode(STEP_2, OUTPUT);
  pinMode(DIR_2,  OUTPUT);
  pinMode(SLP_2, OUTPUT);
  digitalWrite(SLP_2, HIGH);
  pinMode(CONFIG1_M0, OUTPUT);
  pinMode(CONFIG1_M1, OUTPUT);
  pinMode(CONFIG2_M0, OUTPUT);
  pinMode(CONFIG2_M1, OUTPUT);
  setStepMode(1); //Set Drivers to Full-Step Mode
  //Init Serial Comm:
  Serial.begin(9600); delay(500);
  //Setup Timer1:
  int timerScale = 1024; int clockSpeed_MHz = 16; int miliSeconds = 1;
  cli(); //Disable Global Interrupts
  TCCR1A = 0; TCCR1B = 0; //Set Entire TCCR1 Register to 0
  TCCR1B |= (1 << CS10); TCCR1B |= (1 << CS12); //Set to 1024 Scale Mode
  //Do The Math:
  double clockPeriod = clockSpeed_MHz*pow(10, 3) / timerScale;
  unsigned int timerCounts = (miliSeconds * clockPeriod) - 1;
  OCR1A = timerCounts; //Set Compare-Match Register to Desired Timer Count
  TCCR1B |= (1 << WGM12); //Turn on CTC Mode
  TIMSK1 |= (1 << OCIE1A); //Enable Timer Compare Interrupt
  sei(); //Enable Global Interrupts
}

void loop() {
  unsigned char cmd = 0; unsigned char data = 0;
  while(1) {
    //Read New Commands From Serial:
    if(Serial.available()) {
      while(Serial.available() > 0) {
        char newChar = Serial.read();
        if(cmd) { data = newChar; break; }
        else { cmd = newChar; }
      }
    }
    
    //Run Next Command:
    if(cmd && data) {
      if(cmd == 'A') { //Key On Event:
        Serial.print("ON RECV: "); Serial.println(data);
             if(data == 'U') { driveMotor(1, true, 128, false); driveMotor(2, true, 128, true);  } //Up Key
        else if(data == 'D') { driveMotor(1, true, 128, true);  driveMotor(2, true, 128, false); } //Down Key
        //else if(data == 'L') { driveMotor(1, true, 128, true);  driveMotor(2, true, 128, true);  } //Left Key
        //else if(data == 'R') { driveMotor(1, true, 128, true);  driveMotor(2, true, 128, true);  } //Right Key
      } else if(cmd == 'a' && data) { //Key Off Event:
        Serial.print("OFF RECV: "); Serial.println(data);
             if(data == 'U') { driveMotor(1, false, 1, false); driveMotor(2, false, 1, false); } //Up Key
        else if(data == 'D') { driveMotor(1, false, 1, false); driveMotor(2, false, 1, false); } //Down Key
        //else if(data == 'L') { driveMotor(1, false, 1, false); driveMotor(2, false, 1, false); } //Left Key
        //else if(data == 'R') { driveMotor(1, false, 1, false); driveMotor(2, false, 1, false); } //Right Key
      }
      cmd = 0; data = 0;
    }
    
    //digitalWrite(STATUS_LED, HIGH); delay(50);
    //digitalWrite(STATUS_LED,  LOW); delay(50);
    
//    driveMotor(1, true, 255, dir);
//    driveMotor(2, true, 255, dir);
//    for(int c=0; c<5; c++) {
//      digitalWrite(STATUS_LED, HIGH); delay(500);
//      digitalWrite(STATUS_LED,  LOW); delay(500);
//    }
//    driveMotor(1, false, 1, dir);
//    driveMotor(2, false, 1, dir);
    
    /*delay(500);
    
    if(count >= 10) {
      dir = !dir;
      count = 0;
      disengageMotor(1);
      disengageMotor(2);
      delay(2500);
      engageMotor(1);
      engageMotor(2);
      delay(500);
    }
    count++;*/
  }
}

//Motor Control Functions:
void driveMotor(unsigned int motorNum, boolean startStop, unsigned char motorSpeed, boolean dir) {
  int STEP_PIN; int DIR_PIN;
  if(motorNum == 1) { STEP_PIN = STEP_1; DIR_PIN = DIR_1; } 
  else if(motorNum == 2) { STEP_PIN = STEP_2; DIR_PIN = DIR_2; }
  else return;
  if(startStop && motorSpeed > 0) {
    engageMotor(motorNum);
    digitalWrite(DIR_PIN, dir);
    int updateDelay = -(motorSpeed/4)+64;
    driveMotorSpeed[motorNum-1] = updateDelay;
  } else {
    driveMotorSpeed[motorNum-1] = 0;
    driveMotorCount[motorNum-1] = 0;
  }
}

void stepMotor(unsigned int motorNum, unsigned int steps, boolean dir) {
  int STEP_PIN; int DIR_PIN;
  if(motorNum == 1) { STEP_PIN = STEP_1; DIR_PIN = DIR_1; } 
  else if(motorNum == 2) { STEP_PIN = STEP_2; DIR_PIN = DIR_2; }
  else return;
  engageMotor(motorNum);
  digitalWrite(DIR_PIN, dir);
  for(int stp=0; stp<steps; stp++) {
    digitalWrite(STEP_PIN, HIGH);
    delay(5);
    digitalWrite(STEP_PIN, LOW);
    delay(5);
  }
}

void disengageMotor(unsigned int motorNum) {
  int SLP_PIN;
  if(motorNum == 1) { SLP_PIN = SLP_1; } 
  else if(motorNum == 2) { SLP_PIN = SLP_2; }
  else return;
  if(savedBrakeStates[motorNum-1]) {
    digitalWrite(SLP_PIN, LOW);
    //pinMode(SLP_PIN, INPUT);
    savedBrakeStates[motorNum-1] = false;
  }
}

void engageMotor(unsigned int motorNum) {
  int SLP_PIN;
  if(motorNum == 1) { SLP_PIN = SLP_1; } 
  else if(motorNum == 2) { SLP_PIN = SLP_2; }
  else return;
  if(!savedBrakeStates[motorNum-1]) {
    //pinMode(SLP_PIN, OUTPUT);
    digitalWrite(SLP_PIN, HIGH);
    savedBrakeStates[motorNum-1] = true;
  }
}

//Step Modes for Pololu LV (White) Stepper Driver:
//1  == Full Step
//2  == Half Step
//8  == Eighth Step
//16 == Sixteenth Step
void setStepMode(int stepMode) {
  switch(stepMode) {
    case 2:
      digitalWrite(CONFIG1_M0, HIGH);
      digitalWrite(CONFIG1_M1, LOW);
      digitalWrite(CONFIG2_M0, HIGH);
      digitalWrite(CONFIG2_M1, LOW);
      savedStepMode = 2;
    break;
    case 8:
      digitalWrite(CONFIG1_M0, LOW);
      digitalWrite(CONFIG1_M1, HIGH);
      digitalWrite(CONFIG2_M0, LOW);
      digitalWrite(CONFIG2_M1, HIGH);
      savedStepMode = 8;
    break;
    case 16:
      digitalWrite(CONFIG1_M0, HIGH);
      digitalWrite(CONFIG1_M1, HIGH);
      digitalWrite(CONFIG2_M0, HIGH);
      digitalWrite(CONFIG2_M1, HIGH);
      savedStepMode = 16;
    break;
    default:
      digitalWrite(CONFIG1_M0, LOW);
      digitalWrite(CONFIG1_M1, LOW);
      digitalWrite(CONFIG2_M0, LOW);
      digitalWrite(CONFIG2_M1, LOW);
      savedStepMode = 1;
  }
}

ISR(TIMER1_COMPA_vect) {
  for(int i=0; i<NUM_MOTORS; i++) { if(driveMotorSpeed[i]) {
    driveMotorCount[i]++;
    if(driveMotorCount[i] >= driveMotorSpeed[i]) {
      driveMotorCount[i] = 0;
      digitalWrite(motorPins[i], HIGH);
      delayMicroseconds(100);
      digitalWrite(motorPins[i], LOW);
    }
  }}
}
