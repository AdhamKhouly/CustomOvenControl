/* 
 * @file CustomOvenControl.ino
 * @version 1.0
 * @author Adham Elkhouly
 * @contact Adham_Elkhouly@aucegypt.edu
 *
 * @description
 * This Arduino code regulates a bakery electric oven's hot-air temperature based on user-set presets for temperature and baking time. It samples temperature at three locations, controls a fan, and activates up to three heating elements based on the temperature difference.
 */ 

#include <Keypad.h>
#include <LiquidCrystal.h>

// states for the oven
#define INPUT_PHASE 0
#define HEATING 1
#define COOLING 2
#define BAKING 3
#define END_OF_BAKING 4
#define VENTILATION 5

char ovenState = INPUT_PHASE; // Current state of the oven

// Function prototypes
void activateHeatingElements(char numElements);
void initializeHeatingElements();
void readInputs();
int calculateAverageTemperature();
void initializeLCD();
void initializeKeypad();
void initializeTemperatureSensors();
void initializeFan();
void turnFanClockwise();
void turnFanAntiClockwise();
void turnOffFan();
void performCooling();
void performBaking();
void performVentilation();
void performEndOfBaking();

// Input variables
String inputString = "";
long inputTemperature;
long inputTime;
char tempOrTime = 0; // 0 for temperature input, 1 for time input
long startTime1 = 0, startTime2 = 0;

// LCD and Keypad pin configurations 
const int rsPin = 12, enPin = 11, d4Pin = 5, d5Pin = 4, d6Pin = 3, d7Pin = 2;
LiquidCrystal lcd(rsPin, enPin, d4Pin, d5Pin, d6Pin, d7Pin);
const byte ROWS = 4;
const byte COLS = 4;
char hexaKeys[ROWS][COLS] = {
  {'1', '2', '3', 'A'},
  {'4', '5', '6', 'B'},
  {'7', '8', '9', 'C'},
  {'*', '0', '#', 'D'}
};
byte rowPins[ROWS] = {39, 41, 43, 45};
byte colPins[COLS] = {47, 49, 51, 53};
Keypad customKeypad = Keypad(makeKeymap(hexaKeys), rowPins, colPins, ROWS, COLS);

// time tracking
int elapsedTime = 0;

void setup() {
  pinMode(26, OUTPUT); // pin for a possible output (fan or buzzer)

  initializeTemperatureSensors();
  initializeHeatingElements();
  initializeFan();
  initializeLCD();
}

void loop() {
  //  State machine for oven operation
  switch (ovenState) {
    case INPUT_PHASE:
      readInputs();
      break;
    case HEATING:
      performHeating();
      break;
    case COOLING:
      performCooling();
      break;
    case BAKING:
      performBaking();
      break;
    case VENTILATION:
      performVentilation();
      break;
    case END_OF_BAKING:
      performEndOfBaking();
      break;
  }

  // update  time and switch to VENTILATION state if time is up
  if (ovenState != INPUT_PHASE && inputTime > 0) {
    startTime2 = millis();

    if (startTime2 - startTime1 >= 1000) {
      startTime1 = startTime2;
      inputTime--;

      if (inputTime == 0) {
        ovenState = VENTILATION;
      }
    }
  }
}

void initializeTemperatureSensors() {
  pinMode(A0, INPUT);
  pinMode(A1, INPUT);
  pinMode(A2, INPUT);
}

void initializeHeatingElements() {
  pinMode(52, OUTPUT);
  pinMode(50, OUTPUT);
  pinMode(48, OUTPUT);
}

void activateHeatingElements(char numElements) {
  switch (numElements) {
    case 0:
      digitalWrite(52, LOW);
      digitalWrite(50, LOW);
      digitalWrite(48, LOW);
      break;
    case 1:
      digitalWrite(52, HIGH);
      digitalWrite(50, LOW);
      digitalWrite(48, LOW);
      break;
    case 2:
      digitalWrite(52, HIGH);
      digitalWrite(50, HIGH);
      digitalWrite(48, LOW);
      break;
    case 3:
      digitalWrite(52, HIGH);
      digitalWrite(50, HIGH);
      digitalWrite(48, HIGH);
      break;
  }
}

void initializeFan() {
  pinMode(22, OUTPUT);
  pinMode(24, OUTPUT);
  turnOffFan();
}

void turnFanClockwise() {
  digitalWrite(22, HIGH);
  digitalWrite(24, LOW);
}

void turnOffFan() {
  digitalWrite(22, LOW);
  digitalWrite(24, LOW);
}

void turnFanAntiClockwise() {
  digitalWrite(22, LOW);
  digitalWrite(24, HIGH);
}

void initializeLCD() {
  lcd.begin(16, 2);
  lcd.print("Enter temp: ");
}

void initializeKeypad() {
}

void readInputs() {
  if (tempOrTime == 0) {
    char customKey = customKeypad.getKey();
    if (customKey) {
      if (customKey >= '0' && customKey <= '9') {
        inputString += customKey;
        lcd.print(customKey);
      } else if (customKey == '#') {
        if (inputString.length() > 0) {
          inputTemperature = inputString.toInt();
          inputString = "";
          tempOrTime = 1;
          lcd.clear();
          lcd.print("Enter time:");
        }
      }
    }
  } else if (tempOrTime == 1) {
    char customKey = customKeypad.getKey();
    if (customKey) {
      if (customKey >= '0' && customKey <= '9') {
        inputString += customKey;
        lcd.print(customKey);
      } else if (customKey == '#') {
        startTime1 = millis();
        if (inputString.length() > 0) {
          inputTime = inputString.toInt();
          inputString = "";
          ovenState = HEATING;
          tempOrTime = 0;
          lcd.clear();
        }
      }
    }
  }
}

int calculateAverageTemperature() {
  double voltage1 = analogRead(A0) / 2.048;
  double voltage2 = analogRead(A1) / 2.048;
  double voltage3 = analogRead(A2) / 2.048;
  return static_cast<int>((voltage1 + voltage2 + voltage3) / 3 - 10);
}

void performHeating() {
  lcd.setCursor(0, 0);
  lcd.print("HEATING");
  lcd.setCursor(0, 1);
  lcd.print("Temp:");
  lcd.print(calculateAverageTemperature());
  lcd.print(" Time:");
  lcd.print(inputTime);

  if (calculateAverageTemperature() < inputTemperature) {
    turnFanClockwise();

    if ((inputTemperature - calculateAverageTemperature()) >= 30) {
      activateHeatingElements(3);
    } else if ((inputTemperature - calculateAverageTemperature()) >= 20) {
      activateHeatingElements(2);
    } else {
      activateHeatingElements(1);
    }
  } else if (calculateAverageTemperature() > inputTemperature + 5) {
    turnOffFan();
    ovenState = COOLING;
    lcd.clear();
  } else {
    ovenState = BAKING;
    lcd.clear();
  }
}

void performCooling() {
  lcd.setCursor(0, 0);
  lcd.print("COOLING");
  lcd.setCursor(0, 1);
  lcd.print("Temp:");
  lcd.print(calculateAverageTemperature());
  lcd.print(" Time:");
  lcd.print(inputTime);

  if (calculateAverageTemperature() > inputTemperature + 5) {
    turnFanAntiClockwise();
    activateHeatingElements(0);
  } else if (calculateAverageTemperature() < inputTemperature) {
    turnOffFan();
    ovenState = HEATING;
    lcd.clear();
  } else {
    ovenState = BAKING;
    lcd.clear();
  }
}

void performBaking() {
  lcd.setCursor(0, 0);
  lcd.print("BAKING");
  lcd.setCursor(0, 1);
  lcd.print("Temp:");
  lcd.print(calculateAverageTemperature());
  lcd.print(" Time:");
  lcd.print(inputTime);
  turnOffFan();
  activateHeatingElements(0);

  if (calculateAverageTemperature() > inputTemperature + 5) {
    ovenState = COOLING;
    lcd.clear();
  } else if (calculateAverageTemperature() < inputTemperature) {
    ovenState = HEATING;
    lcd.clear();
  }
}

void performVentilation() {
  lcd.setCursor(0, 0);
  lcd.print("VENTILATION");
  lcd.setCursor(0, 1);
  lcd.print("Temp:");
  lcd.print(calculateAverageTemperature());
  lcd.print(" Time:");
  lcd.print(inputTime);

  if (calculateAverageTemperature() >= 30) {
    turnFanAntiClockwise();
    activateHeatingElements(0);
  } else if (calculateAverageTemperature() < 30) {
    for (char i = 0; i < 5; i++) {
      digitalWrite(26, HIGH);
      delay(300);
      digitalWrite(26, LOW);
      delay(300);
    }
    ovenState = END_OF_BAKING;
    lcd.clear();
  }
}

void performEndOfBaking() {
  turnOffFan();
  activateHeatingElements(0);
  lcd.setCursor(0, 0);
  lcd.print("END_OF_BAKING");
  lcd.setCursor(0, 1);
  lcd.print("Temp:");
  lcd.print(calculateAverageTemperature());
  lcd.print(" Time:");
  lcd.print(inputTime);

  char customKey = customKeypad.getKey();
  if (customKey) {
    if (customKey >= '#') {
      lcd.clear();
      lcd.print("Enter temp: ");
      ovenState = INPUT_PHASE;
    }
  }
}
