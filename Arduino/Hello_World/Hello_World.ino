//////////////////////////////////////////////////////////////////////////////////
// Test Arduino Uno sketch to interface with Nixdorf LK-3000 via cartridge port //
// Scrolls "HELLO WORLD!", scans keyboard                                       //
// Brett Hallen, 6-Oct-2025                                                     //
//////////////////////////////////////////////////////////////////////////////////

// Pin assignments for cartridge port
const int D_PINS[6] = {2, 3, 4, 5, 6, 7}; // D0 to D5 data bus
const int A_PINS[4] = {8, 9, 10, 11}; // A0 to A3 address bus
const int DWSTRB_PIN = A0; // Display write strobe (active low)
const int KEYSTRB_PIN = A1; // Keyboard strobe (active low)
const int CLR_PIN = A2; // Clear key input (active low)

////////////////////////////////////////////
// LK-3000 Cartridge Port                 //
// Pin  1 = ~CLR button -> Arduino pin A2 //
// Pin  2 = ~KEYSTB -> Arduino pin A1     //
// Pin  3 = ~DWSTB -> Arduino pin A0      //
// Pin  4 = A3 -> Arduino pin 11          //
// Pin  5 = A2 -> Arduino pin 10          //
// Pin  6 = Vout                          //
// Pin  7 = Ground -> Arduino GND         //
// Pin  8 = +5V                           //
// Pin  9 = D5 -> Arduino pin 7           //
// Pin 10 = D4 -> Arduino pin 6           //
// Pin 11 = A0 -> Arduino pin 8           //
// Pin 12 = A1 -> Arduino pin 9           //
// Pin 13 = D0 -> Arduino pin 2           //
// Pin 14 = D3 -> Arduino pin 5           //
// Pin 14 = D2 -> Arduino pin 4           //
// Pin 16 = D1 -> Arduino pin 3           //
////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////
// Notes:                                                              //
// Pins 6 (Vout) and 8 (+5V) must be connected/shorted for the LK-3000 //
//     to power on.                                                    //
// Uses 6-bit data bus (D0-D5) for DL-1414 displays (ASCII 0x20-0x5F), //
//     with D5 controlling D6 via CD4503BE                             //
// D6 high when D5 low (buffer enabled, follows +5V inputs);           //
// D6 low when D5 high (buffer disabled, pulled low by 680Ω).          //
/////////////////////////////////////////////////////////////////////////

// Keyboard matrix (4 columns x 8 rows)
const String KEY_MAP[8][4] = 
{
// COL1     COL2     COL3    COL4
  {"A",     "B",     "C",    "D"},  // ROW1
  {"J",     "K",     "L",    "M"},  // ROW2
  {"S",     "T",     "U",    "V"},  // ROW3
  {"E",     "F",     "G",    "H"},  // ROW4
  {"N",     "O",     "P",    "Q"},  // ROW5
  {"W",     "X",     "Y",    "Z"},  // ROW6
  {"?",     "[bs]",  "R",    "I"},  // ROW7
  {"[def]", "[stp]", "[sp]", "[f]"} // ROW8
};

void setup() 
{
  Serial.begin(9600);
  Serial.println("LK-3000 Interface Ready");

  // Set pins for display mode by default
  setDisplayMode();

  // Initial clear display
  clearDisplay();

  // Test display with static text
  displayString("HELLO LK-3000   ", 0); // Display initial message
}

void loop() 
{
  // Scroll "HELLO WORLD! " across the display
  static String message = "HELLO WORLD!    "; // Padding for scrolling
  static int offset = 0;
  displayString(message, offset);
  offset = (offset + 1) % (message.length() - 16 + 1); // Scroll step
  delay(300); // Scroll speed

  // Scan keyboard every loop
  scanKeyboard();
}

// Set pins for display writing (outputs)
void setDisplayMode() 
{
  for (int i = 0; i < 6; i++) 
  {
    pinMode(D_PINS[i], OUTPUT);
  }
  for (int i = 0; i < 4; i++) 
  {
    pinMode(A_PINS[i], OUTPUT);
  }
  pinMode(DWSTRB_PIN, OUTPUT);
  digitalWrite(DWSTRB_PIN, HIGH); // Idle high
}

// Set pins for keyboard reading (data pins input)
void setKeyboardMode() 
{
  for (int i = 0; i < 6; i++) 
  {
    pinMode(D_PINS[i], INPUT_PULLUP); // Active low keys
  }
  for (int i = 0; i < 4; i++) 
  {
    pinMode(A_PINS[i], OUTPUT);
  }
  pinMode(KEYSTRB_PIN, OUTPUT);
  digitalWrite(KEYSTRB_PIN, HIGH); // Idle high
}

// Write a character to a specific position (0-15)
void writeChar(byte pos, char c)
{
  // Ensure character is in DL1414 range (0x20 to 0x5F)
  if (c < 0x20 || c > 0x5F) c = ' '; // Default to space if out of range

  // Set address (A0-A3)
  digitalWrite(A_PINS[0], (pos & 0x01) ? HIGH : LOW);
  digitalWrite(A_PINS[1], (pos & 0x02) ? HIGH : LOW);
  digitalWrite(A_PINS[2], (pos & 0x04) ? HIGH : LOW);
  digitalWrite(A_PINS[3], (pos & 0x08) ? HIGH : LOW);

  // Set data (D0-D5, where D5 controls D6 via CD4503BE)
  // For 0x20-0x3F: D6=0, D5=1; for 0x40-0x5F: D6=1, D5=0
  bool d5 = (c >= 0x40) ? LOW : HIGH; // D5=0 for 0x40-0x5F, D5=1 for 0x20-0x3F
  c = c - 0x20; // Shift ASCII to 0x00-0x3F for D0-D4
  digitalWrite(D_PINS[0], (c & 0x01) ? HIGH : LOW);
  digitalWrite(D_PINS[1], (c & 0x02) ? HIGH : LOW);
  digitalWrite(D_PINS[2], (c & 0x04) ? HIGH : LOW);
  digitalWrite(D_PINS[3], (c & 0x08) ? HIGH : LOW);
  digitalWrite(D_PINS[4], (c & 0x10) ? HIGH : LOW);
  digitalWrite(D_PINS[5], d5); // D5 controls D6 (inverse)

  // Pulse strobe
  digitalWrite(DWSTRB_PIN, LOW);
  delayMicroseconds(10); // Short pulse
  digitalWrite(DWSTRB_PIN, HIGH);
}

// Display a 16-char string starting from offset in the message
void displayString(String str, int offset) 
{
  setDisplayMode();
  for (int i = 0; i < 16; i++) 
  {
    char c = ' '; // Default space
    if (i + offset < str.length()) c = str.charAt(i + offset);
    writeChar(i, c);
  }
}

// Clear the display (write spaces to all positions)
void clearDisplay() 
{
  for (int i = 0; i < 16; i++) 
  {
    writeChar(i, ' ');
  }
}

// Scan the keyboard matrix and print pressed keys to Serial
void scanKeyboard() 
{
  setKeyboardMode();
  digitalWrite(A_PINS[3], LOW); // A3 = 0 for keyboard (8 rows)

  for (int row = 0; row < 8; row++) 
  {
    // Set row address (A0-A2)
    digitalWrite(A_PINS[0], (row & 0x01) ? HIGH : LOW);
    digitalWrite(A_PINS[1], (row & 0x02) ? HIGH : LOW);
    digitalWrite(A_PINS[2], (row & 0x04) ? HIGH : LOW);

    // Pulse strobe
    digitalWrite(KEYSTRB_PIN, LOW);
    delayMicroseconds(10);
    int cols = 0;
    if (digitalRead(D_PINS[0]) == LOW) cols |= 0x01; // COL1 via D0
    if (digitalRead(D_PINS[1]) == LOW) cols |= 0x02; // COL2 via D1
    if (digitalRead(D_PINS[2]) == LOW) cols |= 0x04; // COL3 via D2
    if (digitalRead(D_PINS[3]) == LOW) cols |= 0x08; // COL4 via D3
    digitalWrite(KEYSTRB_PIN, HIGH);

    // Check for pressed keys
    for (int col = 0; col < 4; col++) 
    {
      if (cols & (1 << col)) 
      {
        String key = KEY_MAP[row][col];
        Serial.print("Key pressed: ");
        Serial.println(key);
      }
    }
  }

  // Check clear key
  pinMode(CLR_PIN, INPUT_PULLUP);
  if (digitalRead(CLR_PIN) == LOW) 
  {
    Serial.println("[clr] key pressed");
    clearDisplay(); // Clear display on CLR
  }

  // Restore display mode
  setDisplayMode();
}