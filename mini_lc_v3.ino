// ======================================================
// LEVEL CROSSING CONTROLLER (FINAL + 5s BUZZER LIMIT)
// ======================================================


// ---------------- PIN CONFIG ----------------

const int sensorA = 2;
const int sensorB = 3;

const int button    = 4;
const int modeInput = 5;  // LOW = AUTO, HIGH = MANUAL

const int yellowLight = A0;
const int redLight1   = A2;
const int redLight2   = A1;
const int buzzer      = A3;
const int whiteLight  = A4;


// ---------------- RELAY LOGIC ----------------

const int RELAY_ON  = HIGH;
const int RELAY_OFF = LOW;


// ---------------- STATES ----------------

enum State {
  IDLE,
  STARTUP,
  RUNNING,
  BLOCKED
};

State state = IDLE;


// ---------------- DIRECTION ----------------

enum Direction {
  NONE,
  A_TO_B,
  B_TO_A
};

Direction direction = NONE;


// ---------------- VARIABLES ----------------

int occupancy = 0;


// ---------------- TIMING ----------------

unsigned long startupMillis = 0;
const unsigned long startupTime = 2000;

unsigned long blockMillis = 0;
const unsigned long blockTime = 5000;

unsigned long lastFlashMillis = 0;
const unsigned long flashInterval = 500;

const unsigned long whiteDelay = 120;

// 🔊 BUZZER TIMER (NEW)
unsigned long buzzerStartMillis = 0;
const unsigned long buzzerDuration = 5000; // 5 seconds

bool flashState = false;


// ---------------- LOCKOUT ----------------

const unsigned long pulseLockout = 40;

unsigned long lastTriggerA = 0;
unsigned long lastTriggerB = 0;


// ---------------- BUTTON ----------------

const unsigned long buttonDebounce = 50;

bool lastButtonReading = HIGH;
bool buttonState = HIGH;
unsigned long lastButtonChange = 0;

bool crossingForced = false;


// ======================================================
// SETUP
// ======================================================

void setup() {

  pinMode(sensorA, INPUT_PULLUP);
  pinMode(sensorB, INPUT_PULLUP);

  pinMode(button, INPUT_PULLUP);
  pinMode(modeInput, INPUT_PULLUP);

  pinMode(yellowLight, OUTPUT);
  pinMode(redLight1, OUTPUT);
  pinMode(redLight2, OUTPUT);
  pinMode(buzzer, OUTPUT);
  pinMode(whiteLight, OUTPUT);

  allOff();
}


// ======================================================
// MAIN LOOP
// ======================================================

void loop() {

  unsigned long now = millis();

  bool manualMode = digitalRead(modeInput) == HIGH;

  handleButton(now, manualMode);

  if (!manualMode) {
    checkSensorA(now);
    checkSensorB(now);
  }

  runStateMachine(now);

  updateBuzzer();
}


// ======================================================
// STATE MACHINE
// ======================================================

void runStateMachine(unsigned long now) {

  switch (state) {

    case IDLE:
      digitalWrite(whiteLight, RELAY_OFF);
      break;

    case STARTUP:

      if (now - startupMillis >= startupTime) {

        digitalWrite(yellowLight, RELAY_OFF);

        state = RUNNING;

        lastFlashMillis = now;
        flashState = false;

        applyFlash();
      }
      break;

    case RUNNING:

      if (occupancy <= 0) {

        occupancy = 0;

        allOff();

        state = BLOCKED;
        blockMillis = now;

        flashState = false;
        return;
      }

      if (now - lastFlashMillis >= flashInterval) {
        lastFlashMillis = now;
        flashState = !flashState;
        applyFlash();
      }

      break;

    case BLOCKED:

      digitalWrite(whiteLight, RELAY_OFF);

      if (now - blockMillis >= blockTime) {
        state = IDLE;
        direction = NONE;
      }
      break;
  }
}


// ======================================================
// BUTTON HANDLER
// ======================================================

void handleButton(unsigned long now, bool manualMode) {

  bool reading = digitalRead(button);

  if (reading != lastButtonReading) {
    lastButtonChange = now;
  }

  if ((now - lastButtonChange) > buttonDebounce) {

    if (buttonState == HIGH && reading == LOW) {

      if (manualMode) {

        crossingForced = !crossingForced;

        if (crossingForced) {
          trigger(true);
        } else {
          emergencyStop();
        }

      } else {
        emergencyStop(); // AUTO MODE RESET
      }
    }

    buttonState = reading;
  }

  lastButtonReading = reading;
}


// ======================================================
// EMERGENCY STOP
// ======================================================

void emergencyStop() {

  state = IDLE;

  occupancy = 0;
  direction = NONE;

  flashState = false;

  allOff();
}


// ======================================================
// SENSOR INPUTS
// ======================================================

void checkSensorA(unsigned long now) {

  if (digitalRead(sensorA) == LOW) {
    if (now - lastTriggerA > pulseLockout) {
      lastTriggerA = now;
      trigger(true);
    }
  }
}

void checkSensorB(unsigned long now) {

  if (digitalRead(sensorB) == LOW) {
    if (now - lastTriggerB > pulseLockout) {
      lastTriggerB = now;
      trigger(false);
    }
  }
}


// ======================================================
// CORE LOGIC
// ======================================================

void trigger(bool fromA) {

  if (state == IDLE) {

    state = STARTUP;
    startupMillis = millis();

    // 🔊 START BUZZER TIMER (NEW)
    buzzerStartMillis = millis();

    digitalWrite(yellowLight, RELAY_ON);
    digitalWrite(whiteLight, RELAY_OFF);

    lastFlashMillis = millis();
    flashState = false;

    direction = fromA ? A_TO_B : B_TO_A;
    occupancy = 1;

    return;
  }

  if (direction == NONE) {
    direction = fromA ? A_TO_B : B_TO_A;
  }

  if ((direction == A_TO_B && fromA) ||
      (direction == B_TO_A && !fromA)) {
    occupancy++;
  } else {
    occupancy--;
  }

  if (occupancy < 0) occupancy = 0;
}


// ======================================================
// FLASH (WHITE DELAYED)
// ======================================================

void applyFlash() {

  unsigned long now = millis();
  unsigned long phaseTime = now - lastFlashMillis;

  if (flashState) {

    digitalWrite(redLight1, RELAY_ON);
    digitalWrite(redLight2, RELAY_OFF);

    if (phaseTime >= whiteDelay) {
      digitalWrite(whiteLight, RELAY_ON);
    } else {
      digitalWrite(whiteLight, RELAY_OFF);
    }

  } else {

    digitalWrite(redLight1, RELAY_OFF);
    digitalWrite(redLight2, RELAY_ON);

    if (phaseTime >= whiteDelay) {
      digitalWrite(whiteLight, RELAY_ON);
    } else {
      digitalWrite(whiteLight, RELAY_OFF);
    }
  }
}


// ======================================================
// BUZZER (5 SECOND LIMIT)
// ======================================================

void updateBuzzer() {

  if (state == STARTUP || state == RUNNING) {

    if (millis() - buzzerStartMillis < buzzerDuration) {
      digitalWrite(buzzer, RELAY_ON);
    } else {
      digitalWrite(buzzer, RELAY_OFF);
    }

  } else {
    digitalWrite(buzzer, RELAY_OFF);
  }
}


// ======================================================
// ALL OFF
// ======================================================

void allOff() {

  digitalWrite(yellowLight, RELAY_OFF);
  digitalWrite(redLight1, RELAY_OFF);
  digitalWrite(redLight2, RELAY_OFF);
  digitalWrite(buzzer, RELAY_OFF);
  digitalWrite(whiteLight, RELAY_OFF);
}
