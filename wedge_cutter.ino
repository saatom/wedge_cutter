#include <LiquidCrystal.h>
#include <math.h>
#include <Servo.h>
#include "AccelStepper.h"

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
      lcd.print("_");
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
      lcd.print("_");
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
  int val = mm / (2.0 * 3.1415926 * 28.0) * static_cast<double>(stepsPerRevolution) * 1.11;
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
  lcd.clear();
  updateLCD();
  Serial.println("Cutting");
  myservo.write(180);
  delay(1200);
  myservo.write(0);
  dispense(-retLength);  //retract wedge a bit to prevent it from catching on the blade on the way up
  delay(500);
  dispense(retLength);
}

void setup() {
  Serial.begin(9600);

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
  lcd.print("Sw. Version 1.28");

  delay(3000);
  lcd.clear();
  updateLCD();
}

void clearLine() {
  tempvals[pos] = vals[pos];
  chr[pos] = 0;
  updateLCD();
  Serial.println("Clear");
}

void enterLine() {
  qty = 1;
  vals[pos] = tempvals[pos];
  chr[pos] = 0;
  updateLCD();
  Serial.println("Enter");
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
  if (chr[pos] == 3) {
    enterLine();
  } else {
    chr[pos] = chr[pos] + 1;
  }
  updateLCD();
}

void loop() {
  int key = -1;
  int pressed = 0;
  if (keydeb + keyDelay < millis()) {
    for (int i = 0; i < 4; i++) {
      digitalWrite(rows[i], LOW);
      for (int v = 0; v < 4; v++) {
        int val = digitalRead(cols[v]);
        if (val == LOW) {
          pressed = pressed + 1;
          key = keymap[v][i];
          keydeb = millis() + keyDelay;
          break;
        }
        if (key > -1) {
          break;
        }
      }
      digitalWrite(rows[i], HIGH);
    }
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
    } else if (key == 11) {  //enter button
      enterLine();
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
}
