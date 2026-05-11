// ---------------- PIN CONFIG ----------------

const int treadleA = 2;
const int treadleB = 3;

const int manualStartStopButton    = 4;
const int switchAutoManual = 5;  // LOW = AUTO, HIGH = MANUAL

const int yellowLight = A0;
const int redLight1   = A2;
const int redLight2   = A1;
const int yodalarm      = A3;

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

unsigned long yellowMillis = 0;
const unsigned long startupTime = 3000; // Yellow light duration

unsigned long blockMillis = 0;
const unsigned long blockTime = 5000; // 5 Seconds before crossing can be activated again

unsigned long lastFlashMillis = 0;
const unsigned long flashInterval = 500; // How often the reds flash

unsigned long yodalarmStartMillis = 0;
const unsigned long yodalarmDuration = 10000; // Yodalarm duration

bool flashState = false;

// ---------------- LOCKOUT ----------------

const unsigned long pulseLockout = 40;

unsigned long lastTriggerA = 0;
unsigned long lastTriggerB = 0;

// ---------------- manualStartStopButton ----------------

const unsigned long manualStartStopButtonDebounce = 50;

bool lastmanualStartStopButtonReading = HIGH;
bool manualStartStopButtonState = HIGH;
unsigned long lastmanualStartStopButtonChange = 0;

bool crossingForced = false;

// ======================================================
// SETUP
// ======================================================

void setup() {

  pinMode(treadleA, INPUT_PULLUP);
  pinMode(treadleB, INPUT_PULLUP);

  pinMode(manualStartStopButton, INPUT_PULLUP);
  pinMode(switchAutoManual, INPUT_PULLUP);

  pinMode(yellowLight, OUTPUT);
  pinMode(redLight1, OUTPUT);
  pinMode(redLight2, OUTPUT);
  pinMode(yodalarm, OUTPUT);

  allOff();
}

// ======================================================
// MAIN LOOP
// ======================================================

void loop() {

  unsigned long now = millis();

  bool manualMode = digitalRead(switchAutoManual) == HIGH;

  handlemanualStartStopButton(now, manualMode);

  if (!manualMode) {
    checktreadleA(now);
    checktreadleB(now);
  }

  runStateMachine(now);

  updateYodalarm();
}

// ======================================================
// STATE MACHINE
// ======================================================

void runStateMachine(unsigned long now) {

  switch (state) {

    case IDLE:
      break;

    case STARTUP:

      if (now - yellowMillis >= startupTime) {

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

      if (now - blockMillis >= blockTime) {
        state = IDLE;
        direction = NONE;
      }
      break;
  }
  
}

// ======================================================
// manualStartStopButton HANDLER
// ======================================================

void handlemanualStartStopButton(unsigned long now, bool manualMode) {

  bool reading = digitalRead(manualStartStopButton);

  if (reading != lastmanualStartStopButtonReading) {
    lastmanualStartStopButtonChange = now;
  }

  if ((now - lastmanualStartStopButtonChange) > manualStartStopButtonDebounce) {

    if (manualStartStopButtonState == HIGH && reading == LOW) {

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

    manualStartStopButtonState = reading;
  }

  lastmanualStartStopButtonReading = reading;
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

void checktreadleA(unsigned long now) {

  if (digitalRead(treadleA) == LOW) {
    if (now - lastTriggerA > pulseLockout) {
      lastTriggerA = now;
      trigger(true);
    }
  }
}

void checktreadleB(unsigned long now) {

  if (digitalRead(treadleB) == LOW) {
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
    yellowMillis = millis();

    yodalarmStartMillis = millis();

    digitalWrite(yellowLight, RELAY_ON);

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

void applyFlash() {

  unsigned long now = millis();
  unsigned long phaseTime = now - lastFlashMillis;

  if (flashState) {

    digitalWrite(redLight1, RELAY_ON);
    digitalWrite(redLight2, RELAY_OFF);


  } else {

    digitalWrite(redLight1, RELAY_OFF);
    digitalWrite(redLight2, RELAY_ON);

  }

}

void updateYodalarm() {

  if (state == STARTUP || state == RUNNING) {

    if (millis() - yodalarmStartMillis < yodalarmDuration) {
      digitalWrite(yodalarm, RELAY_ON);
    } else {
      digitalWrite(yodalarm, RELAY_OFF);
    }

  } else {
    digitalWrite(yodalarm, RELAY_OFF);
  }
}

// ======================================================
// ALL OFF
// ======================================================

void allOff() {

  digitalWrite(yellowLight, RELAY_OFF);
  digitalWrite(redLight1, RELAY_OFF);
  digitalWrite(redLight2, RELAY_OFF);
  digitalWrite(yodalarm, RELAY_OFF);
}
