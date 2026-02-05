#include <SPI.h>
#include <nRF24L01.h>
#include <RF24.h>
#include <Servo.h>
#include <EEPROM.h>

// Пины
#define CE_PIN   9
#define CSN_PIN  10
#define SERVO_TILT_PIN  7
#define SERVO_PAN_PIN   8
#define LASER_PIN  4

// Радио
RF24 radio(CE_PIN, CSN_PIN);
const byte ADDR_CMD[6] = "IN001";
const byte ADDR_TLM[6] = "OUT01";

// Параметры скана
const int ANG_MIN = -30;
const int ANG_MAX =  30;
const float STEP = 0.05; // 0..1
const int DELAY_MS = 200;

const unsigned int address_center_x = 10;
const unsigned int address_center_y = 11;

// Сервы (0 градусов по нашей системе = 90 градусов на серве)
byte CENTER_X = 90;
byte CENTER_Y = 120;

Servo sTilt;
Servo sPan;

// Пакет телеметрии
struct Data {
  int8_t tilt;
  int8_t pan;
  uint8_t mode;
} satdata;

enum CommandType {
  reset = 1,
  calxp = 2, // Calibrate X Positive
  calxn = 3, // Calibrate X Negative
  calyp = 4, // Calibrate Y Positive
  calyn = 5, // Calibrate Y Negative
  lason = 6,
  lasoff = 7
};

struct Command {
  CommandType type;
} cmd;

void setAngles(int tiltDeg, int panDeg) {
  sTilt.write((int)CENTER_X + tiltDeg);
  sPan.write((int)CENTER_Y + panDeg);
  satdata.tilt = tiltDeg;
  satdata.pan  = panDeg;
}

void sendTelemetry(uint8_t mode) {
  satdata.mode = mode;

  radio.stopListening();
  radio.write(&satdata, sizeof(satdata));
  radio.startListening();
}

void goHome() {
  setAngles(0, 0);
  sendTelemetry(0);
}

float lerp(float value1, float value2, float t) {
  return value1 + t * (value2 - value1); 
}

void scan(int start_angle_left, int end_angle_left,
          int start_angle_right, int end_angle_right,
          int telemetry_data) {
  for (float t = 0; t <= 1; t += STEP) {
      setAngles(
        (int) lerp(start_angle_left, end_angle_left, t), 
        (int) lerp(start_angle_right, end_angle_right, t)
      );
      sendTelemetry(telemetry_data);
      delay(DELAY_MS);
  }
}

void setup() {
  pinMode(LASER_PIN, OUTPUT);
  digitalWrite(LASER_PIN, HIGH);

  sTilt.attach(SERVO_TILT_PIN);
  sPan.attach(SERVO_PAN_PIN);

  // Радио
  radio.begin();
  radio.setPALevel(RF24_PA_LOW);
  radio.setDataRate(RF24_1MBPS);
  radio.setChannel(76);

  radio.openReadingPipe(1, ADDR_CMD);
  radio.openWritingPipe(ADDR_TLM);
  radio.startListening();

  CENTER_X = EEPROM.read(address_center_x);
  CENTER_Y = EEPROM.read(address_center_y);

  if (CENTER_X >= 250 || CENTER_Y >= 250) {
    CENTER_X = 90;
    CENTER_Y = 120;
    EEPROM.write(address_center_x, CENTER_X);
    EEPROM.write(address_center_y, CENTER_Y);
  }

  goHome();
}

void loop() {
  if (radio.available()) {
    radio.read(&cmd, sizeof(cmd));

    switch(cmd.type) {
      case reset:
        scan(ANG_MIN, ANG_MAX, 0, 0, 1);
        scan(0, 0, ANG_MIN, ANG_MAX, 2);
        scan(ANG_MIN, ANG_MAX, ANG_MIN, ANG_MAX, 3);
        scan(ANG_MAX, ANG_MIN, ANG_MIN, ANG_MAX, 4);

        goHome();
        break;
      case calxp:
        CENTER_X++;
        EEPROM.update(address_center_x, CENTER_X);
        goHome();
        break;
        case calxn:
        CENTER_X--;
        EEPROM.update(address_center_x, CENTER_X);
        goHome();
        break;
        case calyp:
        CENTER_Y++;
        EEPROM.update(address_center_y, CENTER_Y);
        goHome();
        break;
        case calyn:
        CENTER_Y--;
        EEPROM.update(address_center_y, CENTER_Y);
        goHome();
        break;
      case lason:
        digitalWrite(LASER_PIN, HIGH);
        break;
      case lasoff:
        digitalWrite(LASER_PIN, LOW);
        break;
    }
  }
}
