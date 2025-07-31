#include <LiquidCrystal.h>
#include <math.h>
#include <Servo.h>
#include "AccelStepper.h"
#include <EEPROM.h>
#include <string>
#include <iostream>

#include "pico/stdlib.h"
#include "pico/flash.h"
using namespace std;

#define dirPin 10  //stepper setup
#define stepPin 9
#define stepsPerRevolution 1600
#define servoPin 14
#define wedgeMultiplier 1.017  //1.016  //amount to lengthen the wedge by
#define wedgeOffset 27         //27 //offset from original glass dimension
#define estopPin 28
#define enaPin 12
#define almPin 11


Servo myservo;
AccelStepper stepper = AccelStepper(1, stepPin, dirPin);

unsigned long stepperPos = 0;

//end stepper and servo setup
const int rs = 16, en = 17, d4 = 18, d5 = 19, d6 = 20, d7 = 21;  //lcd setup
LiquidCrystal lcd(rs, en, d4, d5, d6, d7);

int cols[4] = { 7, 6, 5, 4 };  //keypad setup
int rows[4] = { 3, 2, 1, 0 };

int vals[2] = { 0, 0 };      //length values stored for dispensing
int tempvals[2] = { 0, 0 };  //length values displayed on screen
int qty = 1;
int pos = 0;            //cursor position; 0 is top, 1 is bottom row of LCD
int keydeb = 0;         //debounce for key press
int keyDelay = 100;     //debounce delay on keypad in milliseconds
int chr[2] = { 0, 0 };  //which digit the input is on; e.g. chr[0] = 2 means that the next input is the hundreds place on the first row
int lastKey = -1;
int calMem = 0;  //EEPROM memory address of calibration number
double magicNumber = 1.0;
int mode = 0;  //Starts in normal mode

int keymap[4][4] = {  //mapping the keypad matrix to its associated buttons; 0-9 are raw inputs, 10-15 are used for special functions
  { 1, 2, 3, 12 },
  { 4, 5, 6, 13 },
  { 7, 8, 9, 14 },
  { 10, 0, 11, 15 }
};

double offset(double mm) {
  if (mm < 27) {
    return 0;
  } else {
    return (mm - wedgeOffset) * wedgeMultiplier;
  }
}

int digits(int value) {
  int dig = 1;
  while (floor((double)value / (pow(10, double(dig)))) > 0) {
    dig = dig + 1;
  }
  return dig;
}

void checkEstop() {
  int estop = digitalRead(estopPin);
  if (estop == LOW) {
    lcd.clear();
    lcd.setCursor(1, 0);
    lcd.print("RELEASE E-STOP");
    unsigned long last = millis();
    //int i = 0;
    do {
      delay(50);
    } while (millis() - 5000 < last && digitalRead(estopPin) == LOW);
    if (digitalRead(estopPin) == HIGH) {
      lcd.clear();
      lcd.setCursor(1, 0);
      lcd.print("RESUMING IN 2");
      delay(1000);
      lcd.clear();
      lcd.setCursor(1, 0);
      lcd.print("RESUMING IN 1");
      delay(1000);
    }
  }
}

void updateLCD() {
  int vdig = digits(tempvals[0]);
  int hdig = digits(tempvals[1]);
  int vact = offset(vals[0]);
  int hact = offset(vals[1]);
  int vdigact = digits(vact);
  int hdigact = digits(hact);

  lcd.setCursor(0, pos);
  lcd.print(">");
  lcd.setCursor(0, 1 - pos);
  lcd.print(" ");

  lcd.setCursor(1, 0);
  lcd.print(" V: ");
  for (int i = 0; i < 4 - vdig; i++) {
    if (chr[0] < 1) {
      lcd.print(" ");
    } else {
      lcd.print(" ");
    }
  }

  lcd.print(tempvals[0]);
  lcd.print(" (");
  //lcd.print(static_cast<int>(vals[0] * wedgeMultiplier));
  lcd.print(static_cast<int>(vact));
  lcd.print(")");
  for (int i = 0; i < 4 - vdigact; i++) {
    lcd.print(" ");
  }

  lcd.setCursor(1, 1);
  lcd.print(" H: ");
  for (int i = 0; i < 4 - hdig; i++) {
    if (chr[1] < 1) {
      lcd.print(" ");
    } else {
      //lcd.print("_");
      lcd.print(" ");
    }
  }

  lcd.print(tempvals[1]);
  lcd.print(" (");
  //lcd.print(static_cast<int>(vals[1] * wedgeMultiplier));
  lcd.print(static_cast<int>(hact));
  lcd.print(")");
  for (int i = 0; i < 4 - hdigact; i++) {
    lcd.print(" ");
  }
}

void dispense(double mm) {  //dispenses the wedge
  int val = mm / (2.0 * 3.1415926 * 28.0) * static_cast<double>(stepsPerRevolution) * magicNumber;
  stepper.setCurrentPosition(0);
  //stepperPos = stepperPos + val;
  //stepper.moveTo(stepperPos);
  stepper.moveTo(val);
  stepper.runToPosition();
  //stepper.runToNewPosition();
}

double retLength = 4;  //amount to retract wedge when cutting in millimters

void cut() {  //chops the wedge
  checkEstop();
  //lcd.clear();
  //updateLCD();
  myservo.write(180);
  delay(1200);
  myservo.write(0);
  dispense(-retLength);  //retract wedge a bit to prevent it from catching on the blade on the way up
  delay(500);
  dispense(retLength);
}

void setup() {
  Serial.begin(9600);
  EEPROM.begin(512);

  //EEPROM.write(calMem, 100);
  magicNumber = EEPROM.read(calMem)/100.0;

  myservo.attach(servoPin, 400, 2600);  //servo and stepper initialization
  stepper.setMaxSpeed(4000);
  stepper.setAcceleration(3200);  //stepper acceleration value
  stepper.setCurrentPosition(0);
  //stepper.setEnablePin(enaPin);
  myservo.write(0);
  pinMode(enaPin, OUTPUT);
  //digitalWrite(enaPin, HIGH);
  digitalWrite(enaPin, LOW);
  //pinMode(servoPin, OUTPUT);

  lcd.begin(16, 2);
  //lcd.cursor();
  pinMode(estopPin, INPUT_PULLUP);
  for (int i = 0; i < 4; i++) {
    pinMode(rows[i], OUTPUT);
    pinMode(cols[i], INPUT_PULLUP);
    //digitalWrite(cols[i], HIGH);
  }
  lcd.setCursor(4, 0);
  lcd.print("Welcome");
  lcd.setCursor(0, 1);
  lcd.print("Sw. Version 2.01");

  delay(3000);
  lcd.clear();
  updateLCD();
}

void enterLine() {
  qty = 1;
  vals[pos] = tempvals[pos];
  //chr[pos] = 0;
  updateLCD();
  Serial.println("Enter");
}

void clearLine() {
  //tempvals[pos] = vals[pos];
  tempvals[pos] = 0;
  chr[pos] = 0;
  enterLine();
  updateLCD();
  Serial.println("Clear");
}

void addNum(int number) {
  if (chr[pos] > 3) {
    chr[pos] = 0;
    tempvals[pos] = 0;
  } else if (chr[pos] == 0) {
    tempvals[pos] = 0;
  }
  tempvals[pos] = tempvals[pos] * 10 + number;

  Serial.println(tempvals[pos]);
  if (chr[pos] == 4) {
    //enterLine();
    clearLine();
  } else {
    chr[pos] = chr[pos] + 1;
  }
  enterLine();
  updateLCD();
}

int getKey() {
  //int pressed = 0;
  int key = -1;
  for (int i = 0; i < 4; i++) {
    digitalWrite(rows[i], LOW);
    for (int v = 0; v < 4; v++) {
      int val = digitalRead(cols[v]);
      if (val == LOW) {
        //pressed = pressed + 1;
        key = keymap[v][i];
        break;
      }
      if (key > -1) {
        break;
      }
    }
    digitalWrite(rows[i], HIGH);
  }
  if (keydeb + keyDelay < millis() && key != lastKey) {
    keydeb = millis() + keyDelay;
    lastKey = key;
    return key;
  } else {
    lastKey = key;
    return -1;
  }
}

int calibrate() {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Calibration ");
  magicNumber = EEPROM.read(calMem)/100.0;
  lcd.print(magicNumber);
  lcd.setCursor(0,1);
  lcd.print("Press # to cont.");
  unsigned long startedMill = millis();
  while(1==1){
    int key = getKey();
    if(key == 11){
      break;
    }
    else if(key == 10 || millis() > startedMill + 5000){
      updateLCD();
      return 0;
    }
    delay(50);
  }
  lcd.clear();
  lcd.setCursor(2,0);
  lcd.print("Disp. Sample");

  dispense(125.0);
  cut();
  lcd.clear();
  lcd.setCursor(1, 0);
  lcd.print("Input Length:");
  lcd.setCursor(6, 1);
  //numstr=to_string(magicNumber);
  int chars = 3;
  int newval = 0;
  for(int i=0; i<chars; i++) {
    while(1==1) {
      int key = getKey();
      if(key > -1 && key < 10){ //number input
        newval = newval + key*pow(10,(2-i));
        lcd.print(key);
        break;
      }
      else if(key == 10) { //cancel
        updateLCD();
        return 0;
      }
    }
  }
  magicNumber = (125.0/static_cast<int>(newval))*magicNumber;
  EEPROM.write(calMem, ceil(magicNumber*100.0));
  EEPROM.commit();
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("Set cal to ");
  lcd.print(magicNumber);
  delay(2000);
  updateLCD();
  return 1;
}

void loop() {
  int key = getKey();
  if (key > -1 && key < 10) {  //normal number input
    Serial.println(key);
    addNum(key);
  } else if (key == 12) {  //swap line button
    Serial.println("A");
    chr[pos] = 0;
    vals[pos] = tempvals[pos];
    pos = 1 - pos;
    updateLCD();
  } else if (key == 14) {  //cut button
    Serial.println("C");
    //digitalWrite(enaPin, LOW);
    delay(200);
    cut();
    //digitalWrite(enaPin, HIGH);
  } else if (key == 13) {  //dispense button
    checkEstop();
    if (digitalRead(estopPin) == HIGH) {
      lcd.clear();
      lcd.setCursor(3, 0);
      lcd.print("DISPENSING");
      lcd.setCursor(4, 1);
      lcd.print("QTY: ");
      lcd.print(qty);
      //digitalWrite(enaPin, LOW);
      //delay(800);
      dispense(offset(vals[0]));
      cut();
      dispense(offset(vals[0]));
      cut();
      dispense(offset(vals[1]));
      cut();
      dispense(offset(vals[1]));
      cut();
      //digitalWrite(enaPin, HIGH);
      qty = qty + 1;
    }
    lcd.clear();
    updateLCD();
  } else if (key == 11) {  //Menu button
    //enterLine();
    calibrate();
  } else if (key == 10) {  //clear button
    clearLine();
  } else if (key == 15) {  //single dispense button
    checkEstop();
    if (digitalRead(estopPin) == HIGH) {
      lcd.clear();
      lcd.setCursor(3, 0);
      lcd.print("DISPENSING");
      lcd.setCursor(4, 1);
      lcd.print("SINGLE ");
      if (pos == 0) {
        lcd.print("V");
      } else {
        lcd.print("H");
      }
      //digitalWrite(enaPin, LOW);
      //delay(800);
      dispense(offset(vals[pos]));
      cut();
    }
    lcd.clear();
    updateLCD();
  }
}
