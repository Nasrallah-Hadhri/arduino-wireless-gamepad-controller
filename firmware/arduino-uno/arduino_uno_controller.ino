// ============================================================
//  Arduino Uno Game Controller
//  Sends joystick + button data over Serial at 115200 baud.
//  LED feedback: Green = connected, Red = not connected.
//  Serial format: X,Y,SW,B1,B2,B3,B4
// ============================================================

// --- Pin Definitions ---
// Analog Joystick
const int PIN_JOY_X  = A0;   // VRx
const int PIN_JOY_Y  = A1;   // VRy
const int PIN_JOY_SW = 2;    // Joystick button (internal pull-up)

// Push Buttons (active LOW with internal pull-ups)
const int PIN_BTN_1 = 3;
const int PIN_BTN_2 = 4;
const int PIN_BTN_3 = 5;
const int PIN_BTN_4 = 6;

// Status LEDs (active HIGH, each via 220 Ω resistor)
const int PIN_LED_GREEN = 7;  // Controller mode active
const int PIN_LED_RED   = 8;  // Controller mode inactive

// --- Connection Heartbeat ---
const unsigned long CONNECTED_TIMEOUT_MS = 500;
bool controllerConnected = false;
unsigned long lastConnectionTime = 0;
char connectionBuffer[32];
int connectionIndex = 0;

// ----------------------------------------------------------
//  LED helpers
// ----------------------------------------------------------
void setConnectionLeds(bool connected) {
  digitalWrite(PIN_LED_GREEN, connected ? HIGH : LOW);
  digitalWrite(PIN_LED_RED,   connected ? LOW  : HIGH);
}

// ----------------------------------------------------------
//  Read "CONNECTED\n" heartbeat from the Python host
// ----------------------------------------------------------
void handleSerialInput() {
  while (Serial.available()) {
    char c = Serial.read();
    if (c == '\r') continue;               // ignore CR

    if (c == '\n') {
      connectionBuffer[connectionIndex] = '\0';
      if (connectionIndex > 0 && strcmp(connectionBuffer, "CONNECTED") == 0) {
        controllerConnected = true;
        lastConnectionTime = millis();
      }
      connectionIndex = 0;
    } else if (connectionIndex < (int)sizeof(connectionBuffer) - 1) {
      connectionBuffer[connectionIndex++] = c;
    } else {
      connectionIndex = 0;                 // overflow → reset
    }
  }
}

// ----------------------------------------------------------
//  Timeout: if no heartbeat within 500 ms → disconnected
// ----------------------------------------------------------
void updateConnectionStatus() {
  if (controllerConnected && millis() - lastConnectionTime > CONNECTED_TIMEOUT_MS) {
    controllerConnected = false;
  }
  setConnectionLeds(controllerConnected);
}

// ===========================================================
//  SETUP
// ===========================================================
void setup() {
  Serial.begin(115200);

  // Joystick button
  pinMode(PIN_JOY_SW, INPUT_PULLUP);

  // Push buttons
  pinMode(PIN_BTN_1, INPUT_PULLUP);
  pinMode(PIN_BTN_2, INPUT_PULLUP);
  pinMode(PIN_BTN_3, INPUT_PULLUP);
  pinMode(PIN_BTN_4, INPUT_PULLUP);

  // LEDs
  pinMode(PIN_LED_GREEN, OUTPUT);
  pinMode(PIN_LED_RED,   OUTPUT);

  // --- Startup flash sequence ---
  digitalWrite(PIN_LED_GREEN, HIGH);
  digitalWrite(PIN_LED_RED,   HIGH);
  delay(300);
  digitalWrite(PIN_LED_GREEN, LOW);
  digitalWrite(PIN_LED_RED,   LOW);
  delay(200);
  // Flash green briefly to confirm it works
  digitalWrite(PIN_LED_GREEN, HIGH);
  delay(300);
  digitalWrite(PIN_LED_GREEN, LOW);
  // Start with red ON (not connected yet)
  digitalWrite(PIN_LED_RED, HIGH);
}

// ===========================================================
//  MAIN LOOP  (~50 Hz)
// ===========================================================
void loop() {
  handleSerialInput();
  updateConnectionStatus();

  // --- Read Analog Joystick ---
  // Arduino Uno ADC is 10-bit: values 0–1023, center ≈ 512
  int joyX = analogRead(PIN_JOY_X);
  int joyY = analogRead(PIN_JOY_Y);

  // --- Read Buttons (invert because INPUT_PULLUP: pressed = LOW) ---
  int joySw = !digitalRead(PIN_JOY_SW);
  int btn1  = !digitalRead(PIN_BTN_1);
  int btn2  = !digitalRead(PIN_BTN_2);
  int btn3  = !digitalRead(PIN_BTN_3);
  int btn4  = !digitalRead(PIN_BTN_4);

  // --- Send CSV: X,Y,SW,B1,B2,B3,B4 ---
  // Example: 512,510,0,0,1,0,0
  Serial.print(joyX);
  Serial.print(',');
  Serial.print(joyY);
  Serial.print(',');
  Serial.print(joySw);
  Serial.print(',');
  Serial.print(btn1);
  Serial.print(',');
  Serial.print(btn2);
  Serial.print(',');
  Serial.print(btn3);
  Serial.print(',');
  Serial.println(btn4);

  delay(20);  // ~50 Hz update rate
}
