# 1 "C:\\Users\\Павел\\Documents\\GitHub\\mosh-eng-case-1\\Base\\Base.ino"
# 2 "C:\\Users\\Павел\\Documents\\GitHub\\mosh-eng-case-1\\Base\\Base.ino" 2
# 3 "C:\\Users\\Павел\\Documents\\GitHub\\mosh-eng-case-1\\Base\\Base.ino" 2
# 4 "C:\\Users\\Павел\\Documents\\GitHub\\mosh-eng-case-1\\Base\\Base.ino" 2




RF24 radio(9, 10);

const byte ADDR_CMD[6] = "IN001";
const byte ADDR_TLM[6] = "OUT01";

bool enableOutput = true;
bool laserOn = true;

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


struct Data {
  int8_t tilt;
  int8_t pan;
  uint8_t mode;
} satdata;

void sendReset() {
  cmd.type = reset;
  Serial.println("[COMMAND]: RESET INITIALIZED");
  radio.stopListening();
  radio.write(&cmd, sizeof(cmd));
  radio.startListening();
  //Serial.println("[COMMAND]: RESET SUCCESSFUL");
  Serial.println("[OUTPUT]: Type 'o' to toggle output");
}

void sendCal(char axis, bool isPositive) {
  if (axis == 'x') {
    if (isPositive) {
      cmd.type = calxp;
    }
    else {
      cmd.type = calxn;
    }
  }
  else if (axis == 'y') {
    if (isPositive) {
      cmd.type = calyp;
    }
    else {
      cmd.type = calyn;
    }
  }
  radio.stopListening();
  radio.write(&cmd, sizeof(cmd));
  radio.startListening();
}

void toggleLaser() {
  laserOn = !laserOn;
  if (laserOn) {
    cmd.type = lason;
  }
  else {
    cmd.type = lasoff;
  }

  radio.stopListening();
  radio.write(&cmd, sizeof(cmd));
  radio.startListening();
}

void setup() {
  Serial.begin(115200);

  radio.begin();
  radio.setPALevel(RF24_PA_LOW);
  radio.setDataRate(RF24_1MBPS);
  radio.setChannel(76);

  radio.openWritingPipe(ADDR_CMD);
  radio.openReadingPipe(1, ADDR_TLM);
  radio.startListening();

  Serial.println("[OUTPUT]: System initialized! To reset type 'r'");
  Serial.println("[OUTPUT]: Type 'o' to toggle output");
}

char cal_ax = 'x'; // x либо y

void loop() {
  // отправка команды
  if (Serial.available()) {
    char ch = Serial.read();
    switch(ch) {
      case 'r':
      case 'R':
        sendReset();
        break;
      case 'o':
      case 'O':
      case '0':
        enableOutput = !enableOutput;
        break;
      case 'p':
        sendCal(cal_ax, true);
        break;
      case 'n':
        sendCal(cal_ax, false);
        break;
      case 'x':
        cal_ax = 'x';
        break;
      case 'y':
        cal_ax = 'y';
        break;
      case 'l':
        toggleLaser();
        break;
    }
  }

  // прием телеметрии
  if (radio.available() && enableOutput) {
    radio.read(&satdata, sizeof(satdata));
    if (satdata.mode == 0) {
      Serial.print("[OUTPUT]: HOMING");
    }
    else {
      Serial.print("[OUTPUT]: mode=");
      Serial.print(satdata.mode);
      Serial.print(" tilt=");
      Serial.print((int)satdata.tilt);
      Serial.print(" pan=");
      Serial.println((int)satdata.pan);
    }
  }
}
