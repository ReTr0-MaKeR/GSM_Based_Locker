#include <Keypad.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <Servo.h>
#include <SoftwareSerial.h>

// Initialize the LCD with the I2C address (e.g., 0x27 or 0x3F for your display)
LiquidCrystal_I2C lcd(0x27, 16, 2);

// Define password settings
const char correctPassword[] = "123456";
char inputPassword[7]; // To hold a 6-digit password plus null terminator
int passwordIndex = 0;

// Define keypad settings
const byte ROWS = 4; // four rows
const byte COLS = 4; // four columns
char keys[ROWS][COLS] = {
  {'1','2','3','A'},
  {'4','5','6','B'},
  {'7','8','9','C'},
  {'*','0','#','D'}
};
byte rowPins[ROWS] = {9, 10, 11, 12}; // connect to the row pinouts of the keypad
byte colPins[COLS] = {4, 6, 7, 8}; // connect to the column pinouts of the keypad

Keypad customKeypad = Keypad(makeKeymap(keys), rowPins, colPins, ROWS, COLS);

// Servo setup
Servo myServo;
const int servoPin = 5;
bool isUnlocked = false;

// Buzzer
const int buzzerPin = 13;

// SIM800L setup
SoftwareSerial mySerial(3, 2); // SIM800L Tx & Rx connected to Arduino pins 3 & 2
int count= 1;
void setup() {
  Serial.begin(9600);
  mySerial.begin(9600);

  lcd.begin(16, 2);
  lcd.backlight();
  lcd.setCursor(1, 0);
  lcd.print("Locker Started");

  pinMode(buzzerPin, OUTPUT);

  myServo.attach(servoPin);
  myServo.write(150); // Set servo to idle position at 0 degrees

  // Initialize SIM800L
  mySerial.println("AT");
  delay(1000);
  mySerial.println("AT+CMGF=1"); // Configure for TEXT mode
  delay(1000);
  lcd.clear();
  lcd.setCursor(2, 0);
  lcd.print("Enter PIN:");
}

void loop() {
  char customKey = customKeypad.getKey();

  if (customKey) {
    if (customKey == '*') {
      // Start new password entry
      passwordIndex = 0;
      lcd.clear();
      lcd.setCursor(2, 0);
      lcd.print("Enter PIN");
    } else if (customKey == '#') {
      // Check entered password
      inputPassword[passwordIndex] = '\0'; // Null-terminate the password

      if (strcmp(inputPassword, correctPassword) == 0) {
        lcd.clear();
        lcd.setCursor(1, 0);
        lcd.print("Access Granted");
        digitalWrite(buzzerPin, HIGH);
        delay(200);
        digitalWrite(buzzerPin, LOW);
        delay(200);
        myServo.write(0); // Unlock servo
        count = 1;
        isUnlocked = true;
      } else {
        lcd.clear();
        lcd.setCursor(2, 0);
        lcd.print("Access Denied");
        lcd.setCursor(2, 1);
        lcd.print("Wrong attempt: ");
        lcd.setCursor(0, 1);
        lcd.print("Wrong attempt: ");
        lcd.setCursor(15, 1);
        lcd.print(count);
        count = count+1;
        // Flash LED for incorrect entry
        for (int i = 0; i < 5; i++) {
          digitalWrite(buzzerPin, HIGH);
          delay(200);
          digitalWrite(buzzerPin, LOW);
          delay(200);
        }
        Serial.print("Count: ");
        Serial.print(count);
        Serial.println("");
        if(count == 4)
        {
          sendSMS();// Send SMS notification
          count = 1;
        }
        
      }
      delay(1000); // Pause before resetting
      lcd.clear();
      lcd.setCursor(2, 0);
      lcd.print("Enter PIN:");
      passwordIndex = 0; // Reset password index for next entry
    } else if (customKey == 'C' && isUnlocked) {

      // Reset servo to idle position when 'C' is pressed and it's unlocked

      myServo.write(150); // Lock servo
      isUnlocked = false;
      lcd.clear();
      lcd.setCursor(3, 0);
      lcd.print("Door Locked");
      delay(1000);
      lcd.clear();
      lcd.setCursor(2, 0);
      lcd.print("Enter PIN:");
    } else {
      // Record each digit if not * or #
      if (passwordIndex < 6) {
        inputPassword[passwordIndex] = customKey;
        passwordIndex++;
        lcd.setCursor(passwordIndex, 1);
        lcd.print('*'); // Display * for each entry
      }
    }
  }
}

// Function to send SMS alert and display status on LCD
void sendSMS() {
  lcd.clear();
  lcd.print("Sending SMS...");
  delay(500);

  mySerial.println("AT+CMGS=\"+8801821318824\""); // Replace with target phone number
  delay(1000);
  mySerial.print("Alert!!! Unauthorized access attempt detected!");
  mySerial.write(26); // ASCII code for Ctrl+Z to send the message
  delay(2000); // Wait for message to send

 

  // Display responses from SIM800L
  delay(500);
  lcd.clear();
  lcd.print("SIM800L Resp:");
  lcd.setCursor(0, 1);

  // Show first response message (up to 16 chars) on the second line of the LCD
  if (mySerial.available()) {
    char c = mySerial.read();
    int count = 0;
    while (mySerial.available() && count < 16) {
      lcd.print(c);
      c = mySerial.read();
      count++;
    }
  }
  delay(2000); // Pause to allow reading the response
  
  // Display SMS sending status
  lcd.clear();
  lcd.setCursor(2, 0);
  lcd.print("SMS Sent");
  delay(2000); 
  lcd.clear();
  lcd.setCursor(2, 0);
  lcd.print("Enter PIN:");
}
