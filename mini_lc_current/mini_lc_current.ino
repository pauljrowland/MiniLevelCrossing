// ---------------- PIN CONFIG ----------------

const int treadleA = 2;
const int treadleB = 3;

const int manualStartStopButton = 4;
const int switchAutoManual = 5;

const int yellowLight = A0;
const int redLight1   = A2;
const int redLight2   = A1;
const int yodAlarm    = A3;

// ---------------- RELAY LOGIC ----------------

const int RELAY_ON  = HIGH;
const int RELAY_OFF = LOW;

// ---------------- STATES ----------------

enum State { IDLE, STARTUP, LAMPTEST, RUNNING, BLOCKED };
State state = IDLE;

enum Direction { NONE, A_TO_B, B_TO_A };
Direction direction = NONE;

// ---------------- VARIABLES ----------------

int occupancy = 0;
unsigned long lampTestMillis = 0;
const unsigned long lampTestTime = 1000;

// ---------------- TIMING ----------------

unsigned long yellowMillis = 0;
const unsigned long startupTime = 3000;

unsigned long blockMillis = 0;
const unsigned long blockTime = 30000;

unsigned long lastFlashMillis = 0;
const unsigned long flashInterval = 500;

bool flashState = false;

// ---------------- BUTTON ----------------

const unsigned long manualStartStopButtonDebounce = 50;
bool lastmanualStartStopButtonReading = HIGH;
bool manualStartStopButtonState = HIGH;
unsigned long lastmanualStartStopButtonChange = 0;

bool crossingForced = false;

// ---------------- SENSOR ROBUSTNESS ----------------

const unsigned long bootIgnoreTime = 2000;
const unsigned long stableTime = 30;
const unsigned long pulseLockout = 40;

unsigned long bootTime;

// treadle A
bool treadleA_state = HIGH;
bool treadleA_armed = true;
unsigned long treadleA_lastChange = 0;
unsigned long lastTriggerA = 0;

// treadle B
bool treadleB_state = HIGH;
bool treadleB_armed = true;
unsigned long treadleB_lastChange = 0;
unsigned long lastTriggerB = 0;

// ---------------- SETUP ----------------

void setup() {

  pinMode(treadleA, INPUT_PULLUP);
  pinMode(treadleB, INPUT_PULLUP);

  pinMode(manualStartStopButton, INPUT_PULLUP);
  pinMode(switchAutoManual, INPUT_PULLUP);

  pinMode(yellowLight, OUTPUT);
  pinMode(redLight1, OUTPUT);
  pinMode(redLight2, OUTPUT);
  pinMode(yodAlarm, OUTPUT);

  bootTime = millis();

  allOff();
}

// ---------------- MAIN LOOP ----------------

void loop() {

  unsigned long now = millis();

  bool manualMode = digitalRead(switchAutoManual) == HIGH;

  handlemanualStartStopButton(now, manualMode);

  if (!manualMode) {

    if (state == IDLE || state == RUNNING) {
      checktreadleA(now);
      checktreadleB(now);
    }
  }

  runStateMachine(now);
  updateyodAlarm();
}

// ---------------- STATE MACHINE ----------------

void runStateMachine(unsigned long now) {

  switch (state) {

    case IDLE:
      break;

    case STARTUP:

      if (now - yellowMillis >= startupTime) {

        // Yellow goes off
        digitalWrite(yellowLight, RELAY_OFF);

        // Both reds come on immediately
        digitalWrite(redLight1, RELAY_ON);
        digitalWrite(redLight2, RELAY_ON);

        // Start 750 ms lamp test
        state = LAMPTEST;
        lampTestMillis = now;
      }

      break;

    case LAMPTEST:

      if (now - lampTestMillis >= lampTestTime) {

        // Start normal alternating sequence immediately.
        // Red 1 remains ON; Red 2 turns OFF.
        state = RUNNING;

        lastFlashMillis = now;
        flashState = true;

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

// ---------------- TREADLE A ----------------

void checktreadleA(unsigned long now) {

  if (now - bootTime < bootIgnoreTime) return;

  bool reading = digitalRead(treadleA);

  if (reading != treadleA_state) {
    treadleA_state = reading;
    treadleA_lastChange = now;
  }

  if ((now - treadleA_lastChange) > stableTime) {

    if (treadleA_state == LOW && treadleA_armed) {

      if (now - lastTriggerA > pulseLockout) {

        lastTriggerA = now;
        treadleA_armed = false;

        trigger(true);
      }
    }

    if (treadleA_state == HIGH) {
      treadleA_armed = true;
    }
  }
}

// ---------------- TREADLE B ----------------

void checktreadleB(unsigned long now) {

  if (now - bootTime < bootIgnoreTime) return;

  bool reading = digitalRead(treadleB);

  if (reading != treadleB_state) {
    treadleB_state = reading;
    treadleB_lastChange = now;
  }

  if ((now - treadleB_lastChange) > stableTime) {

    if (treadleB_state == LOW && treadleB_armed) {

      if (now - lastTriggerB > pulseLockout) {

        lastTriggerB = now;
        treadleB_armed = false;

        trigger(false);
      }
    }

    if (treadleB_state == HIGH) {
      treadleB_armed = true;
    }
  }
}

// ---------------- CORE LOGIC ----------------

void trigger(bool fromA) {

  if (state == IDLE) {

    state = STARTUP;
    yellowMillis = millis();

    digitalWrite(yellowLight, RELAY_ON);

    lastFlashMillis = millis();
    flashState = false;

    direction = fromA ? A_TO_B : B_TO_A;
    occupancy = 1;

    return;
  }

  if ((direction == A_TO_B && !fromA) ||
      (direction == B_TO_A && fromA)) {

    occupancy = 0;
  }
}

// ---------------- BUTTON ----------------

void handlemanualStartStopButton(unsigned long now, bool manualMode) {

  bool reading = digitalRead(manualStartStopButton);

  if (reading != lastmanualStartStopButtonReading) {
    lastmanualStartStopButtonChange = now;
  }

  if ((now - lastmanualStartStopButtonChange) > manualStartStopButtonDebounce) {

    if (manualStartStopButtonState == HIGH && reading == LOW) {

      if (manualMode) {

        crossingForced = !crossingForced;

        if (crossingForced) trigger(true);
        else emergencyStop();

      } else {
        emergencyStop();
      }
    }

    manualStartStopButtonState = reading;
  }

  lastmanualStartStopButtonReading = reading;
}

// ---------------- EMERGENCY STOP ----------------

void emergencyStop() {

  state = IDLE;
  occupancy = 0;
  direction = NONE;

  flashState = false;
  crossingForced = false;

  allOff();
}

// ---------------- LIGHT CONTROL ----------------

void applyFlash() {

  if (flashState) {
    digitalWrite(redLight1, RELAY_ON);
    digitalWrite(redLight2, RELAY_OFF);
  } else {
    digitalWrite(redLight1, RELAY_OFF);
    digitalWrite(redLight2, RELAY_ON);
  }
}

void updateyodAlarm() {

if (state == STARTUP || state == LAMPTEST || state == RUNNING)
    digitalWrite(yodAlarm, RELAY_ON);
  else
    digitalWrite(yodAlarm, RELAY_OFF);
}

void allOff() {

  digitalWrite(yellowLight, RELAY_OFF);
  digitalWrite(redLight1, RELAY_OFF);
  digitalWrite(redLight2, RELAY_OFF);
  digitalWrite(yodAlarm, RELAY_OFF);
}