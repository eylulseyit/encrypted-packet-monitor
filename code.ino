#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <DHT.h>

//DHT11
#define DHTPIN 13
#define DHTTYPE DHT11
DHT dht(DHTPIN, DHTTYPE);

//HC-SR04
#define TRIG_PIN 25
#define ECHO_PIN 26

//Buzzer
#define BUZZER_PIN 27

//LCD
LiquidCrystal_I2C lcd(0x27, 16, 2);

//Motors
const int MOTOR1_IN1 = 2;   // D2
const int MOTOR1_IN2 = 4;   // D4
const int MOTOR2_IN1 = 18;  // D18
const int MOTOR2_IN2 = 19;  // D19

unsigned long lastDisplayUpdate = 0;

void setup() {
  Serial.begin(115200);

  // LCD and DHT
  lcd.init();
  lcd.backlight();
  dht.begin();

  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);
  pinMode(BUZZER_PIN, OUTPUT);

  // Motor pins
  pinMode(MOTOR1_IN1, OUTPUT);
  pinMode(MOTOR1_IN2, OUTPUT);
  pinMode(MOTOR2_IN1, OUTPUT);
  pinMode(MOTOR2_IN2, OUTPUT);
}

void loop() {
  // Sensor calculations
  float temperature = dht.readTemperature();
  float humidity = dht.readHumidity();

  if (isnan(temperature) || isnan(humidity)) {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Sensor error!");
    return;
  }

  // Distance implementation
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);

  long duration = pulseIn(ECHO_PIN, HIGH, 30000); // timeout 30ms
  float distance = duration * 0.034 / 2;

  // distance control
  if (distance < 2 || distance > 300) {
    distance = 999;
  }

  bool alert = (distance < 10.0 && distance != 999) || (temperature > 35.0);
  digitalWrite(BUZZER_PIN, alert ? LOW : HIGH); // buzzer trigger boolean

  if (millis() - lastDisplayUpdate >= 1000) {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("TEMP(C): ");
    lcd.print(temperature, 1);

    lcd.setCursor(0, 1);
    lcd.print("HUM: ");
    lcd.print(humidity, 1);
    lcd.print("%");
    lcd.print(" D:");
    lcd.print(distance == 999 ? 0 : distance, 0); // invalid distance
    
    lastDisplayUpdate = millis();
  }

  static unsigned long lastMotorTime = 0;
  static int phase = 0;

  if (millis() - lastMotorTime >= 1000) {
    lastMotorTime = millis();

    // Motorları durdurma kontrolü
    if (distance < 10.0 && distance != 999) {
      // Motorları durdur
      digitalWrite(MOTOR1_IN1, LOW);
      digitalWrite(MOTOR1_IN2, LOW);
      digitalWrite(MOTOR2_IN1, LOW);
      digitalWrite(MOTOR2_IN2, LOW);
      Serial.println("Engel var! Motorlar durdu.");
      return; // Aşağıdaki motor fazlarını atla
    }

    switch (phase) {
      case 0: // forward
        digitalWrite(MOTOR1_IN1, HIGH);
        digitalWrite(MOTOR1_IN2, LOW);
        digitalWrite(MOTOR2_IN1, HIGH);
        digitalWrite(MOTOR2_IN2, LOW);
        Serial.println("İleri gidiyor");
        break;

      case 1: // stop
        digitalWrite(MOTOR1_IN1, LOW);
        digitalWrite(MOTOR1_IN2, LOW);
        digitalWrite(MOTOR2_IN1, LOW);
        digitalWrite(MOTOR2_IN2, LOW);
        Serial.println("Durdu");
        break;

      case 2: // back
        digitalWrite(MOTOR1_IN1, LOW);
        digitalWrite(MOTOR1_IN2, HIGH);
        digitalWrite(MOTOR2_IN1, LOW);
        digitalWrite(MOTOR2_IN2, HIGH);
        Serial.println("Geri gidiyor");
        break;

      case 3: // stop
        digitalWrite(MOTOR1_IN1, LOW);
        digitalWrite(MOTOR1_IN2, LOW);
        digitalWrite(MOTOR2_IN1, LOW);
        digitalWrite(MOTOR2_IN2, LOW);
        Serial.println("Durdu");
        break;
    }

    phase = (phase + 1) % 4;
  }
}

