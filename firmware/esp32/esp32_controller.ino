// ESP32 PC Controller - Serial Output Version

// --- Pin Definitions ---
// Analog Stick
const int PIN_JOY_X = 34;
const int PIN_JOY_Y = 35;
const int PIN_JOY_SW = 32;

// Push Buttons
const int PIN_BTN_1 = 13;
const int PIN_BTN_2 = 12;
const int PIN_BTN_3 = 14;
const int PIN_BTN_4 = 27;
const int PIN_BTN_5 = 26;

// LEDs
const int PIN_LED_1 = 33; // Green LED
const int PIN_LED_2 = 25; // Red LED

const unsigned long CONNECTED_TIMEOUT_MS = 500;
bool controllerConnected = false;
unsigned long lastConnectionTime = 0;
char connectionBuffer[32];
int connectionIndex = 0;

void setConnectionLeds(bool connected) {
  // Assuming common cathode LEDs: HIGH to light, LOW to turn off
  digitalWrite(PIN_LED_1, connected ? HIGH : LOW);  // Green on when connected
  digitalWrite(PIN_LED_2, connected ? LOW : HIGH);  // Red on when not connected
}

void handleSerialInput() {
  while (Serial.available()) {
    char c = Serial.read();
    if (c == '\r') {
      continue;
    }
    if (c == '\n') {
      connectionBuffer[connectionIndex] = '\0';
      if (connectionIndex > 0 && strcmp(connectionBuffer, "CONNECTED") == 0) {
        controllerConnected = true;
        lastConnectionTime = millis();
        Serial.println("DEBUG: Received CONNECTED");
      }
      connectionIndex = 0;
    } else if (connectionIndex < sizeof(connectionBuffer) - 1) {
      connectionBuffer[connectionIndex++] = c;
    } else {
      connectionIndex = 0;
    }
  }
}

void updateConnectionStatus() {
  if (controllerConnected && millis() - lastConnectionTime > CONNECTED_TIMEOUT_MS) {
    controllerConnected = false;
  }
  setConnectionLeds(controllerConnected);
}

void setup() {
  // We use a fast baud rate for low-latency controller input
  Serial.begin(115200);

  // Initialize Joystick Button with internal pull-up
  pinMode(PIN_JOY_SW, INPUT_PULLUP);

  // Initialize Push Buttons with internal pull-ups
  pinMode(PIN_BTN_1, INPUT_PULLUP);
  pinMode(PIN_BTN_2, INPUT_PULLUP);
  pinMode(PIN_BTN_3, INPUT_PULLUP);
  pinMode(PIN_BTN_4, INPUT_PULLUP);
  pinMode(PIN_BTN_5, INPUT_PULLUP);

  // Initialize LEDs
  pinMode(PIN_LED_1, OUTPUT);
  pinMode(PIN_LED_2, OUTPUT);

  // Startup Sequence: Flash LEDs to confirm the board is running
  digitalWrite(PIN_LED_1, HIGH);
  digitalWrite(PIN_LED_2, HIGH);
  delay(500);
  digitalWrite(PIN_LED_1, LOW);
  digitalWrite(PIN_LED_2, LOW);
  delay(500);
  // Test green LED: should light for 1 second if common anode
  digitalWrite(PIN_LED_1, LOW);
  delay(1000);
  digitalWrite(PIN_LED_1, HIGH);
}

void loop() {
  handleSerialInput();
  updateConnectionStatus();

  // Read Analog Stick (ESP32 ADC is 12-bit: values range from 0 to 4095)
  int joyX = analogRead(PIN_JOY_X);
  int joyY = analogRead(PIN_JOY_Y);

  // Read Buttons
  // We use `!` to invert the reading.
  // Because of INPUT_PULLUP, a pressed button reads LOW (0). Inverting makes
  // pressed = 1.
  int joySw = !digitalRead(PIN_JOY_SW);
  int btn1 = !digitalRead(PIN_BTN_1);
  int btn2 = !digitalRead(PIN_BTN_2);
  int btn3 = !digitalRead(PIN_BTN_3);
  int btn4 = !digitalRead(PIN_BTN_4);
  int btn5 = !digitalRead(PIN_BTN_5);

  // Format the data as a comma-separated string: X,Y,SW,B1,B2,B3,B4,B5
  // Example output: 2048,2048,0,1,0,0,0,0
  Serial.print(joyX);
  Serial.print(",");
  Serial.print(joyY);
  Serial.print(",");
  Serial.print(joySw);
  Serial.print(",");
  Serial.print(btn1);
  Serial.print(",");
  Serial.print(btn2);
  Serial.print(",");
  Serial.print(btn3);
  Serial.print(",");
  Serial.print(btn4);
  Serial.print(",");
  Serial.println(btn5);

  // Run at ~50Hz (20ms delay). This prevents flooding the Serial port
  // while keeping latency very low for gaming.
  delay(20);
}
