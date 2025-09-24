// Nixdorf LK-3000 test program
// Brett Hallen, 24-Sept-2025
// Pin definitions
const int CLR_PIN = 2;      // CLR - active low
const int KEYSTB_PIN = 3;   // KEYSTB - active low
const int DWSTB_PIN = 4;    // DWSTB - active low
const int ADDR_PINS[] = {5, 6, 7}; // A0 to A2 (for ROW1 to ROW8)
const int DATA_PINS[] = {8, 9, 10, 11}; // D0 to D3 (for COL1 to COL4, D4-D5 unused)

// Keyboard matrix mapping (ROW1 to ROW8, COL1 to COL4)
const char keyMap[8][4] = {
  {'A', 'B', 'C', 'D'},      // ROW1
  {'J', 'K', 'L', 'M'},      // ROW2
  {'S', 'T', 'U', 'V'},      // ROW3
  {'E', 'F', 'G', 'H'},      // ROW4
  {'N', 'O', 'P', 'Q'},      // ROW5
  {'W', 'X', 'Y', 'Z'},      // ROW6
  {'?', '[', 'R', 'I'},      // ROW7 ([BS] as '[', for backspace)
  {'[', '[', '_', '['}       // ROW8 ([DEF], [STP], [SP], [F], using '[' for special keys)
};

void setup() {
  // Initialize serial communication
  Serial.begin(9600);

  // Configure control pins
  pinMode(CLR_PIN, INPUT_PULLUP); // CLR as input to detect button press
  pinMode(KEYSTB_PIN, OUTPUT);
  pinMode(DWSTB_PIN, OUTPUT);

  // Configure address pins as outputs
  for (int i = 0; i < 3; i++) {
    pinMode(ADDR_PINS[i], OUTPUT);
    digitalWrite(ADDR_PINS[i], HIGH); // Pulled high by default
  }

  // Configure data pins as inputs (only D0-D3 for COL1-COL4)
  for (int i = 0; i < 4; i++) {
    pinMode(DATA_PINS[i], INPUT_PULLUP); // Use internal pull-ups
  }

  // Set initial states
  digitalWrite(KEYSTB_PIN, HIGH); // KEYSTB inactive
  digitalWrite(DWSTB_PIN, HIGH);  // DWSTB inactive
}

void loop() {
  // Check if CLR button is pressed
  if (digitalRead(CLR_PIN) == LOW) {
    Serial.println("[CLR]");
    delay(200); // Debounce delay
    while (digitalRead(CLR_PIN) == LOW); // Wait for release
  }

  // Poll through addresses 0 to 7 (ROW1 to ROW8, A0-A2)
  for (int addr = 0; addr < 8; addr++) {
    // Set address pins
    digitalWrite(ADDR_PINS[0], (addr & 0x01) ? HIGH : LOW);
    digitalWrite(ADDR_PINS[1], (addr & 0x02) ? HIGH : LOW);
    digitalWrite(ADDR_PINS[2], (addr & 0x04) ? HIGH : LOW);

    // Activate KEYSTB
    digitalWrite(KEYSTB_PIN, LOW);
    delayMicroseconds(10); // Allow signals to stabilize

    // Read data bus (D0-D3 for COL1-COL4)
    for (int col = 0; col < 4; col++) {
      if (digitalRead(DATA_PINS[col]) == LOW) {
        // Key pressed, get character from keyMap
        char key = keyMap[addr][col];
        if (key == '[') 
        {
          // Handle special keys
          if (addr == 7 && col == 0) Serial.print("[DEF]");
          else if (addr == 7 && col == 1) Serial.print("[STP]");
          else if (addr == 7 && col == 2) Serial.print("[SP]");
          else if (addr == 7 && col == 3) Serial.print("[F]");
        }
        else
        {
          Serial.print(key);
        }
        delay(200); // Debounce delay
        while (digitalRead(DATA_PINS[col]) == LOW); // Wait for key release
      }
    }

    // Deactivate KEYSTB
    digitalWrite(KEYSTB_PIN, HIGH);
  }

  delay(10); // Short delay between polls
}