//////////////////////////////////////////////////////////////////////////////////
// Test Arduino Uno sketch to interface with Nixdorf LK-3000 via cartridge port //
// Scrolls "HELLO WORLD!" circularly, scans keyboard                           //
// Brett Hallen, 6-Oct-2025, modified for circular scrolling                   //
//////////////////////////////////////////////////////////////////////////////////

// Pin assignments for cartridge port
const int D_PINS[6] = {2, 3, 4, 5, 6, 7}; // D0 to D5 data bus
const int A_PINS[4] = {8, 9, 10, 11}; // A0 to A3 address bus
const int DWSTRB_PIN = A0; // Display write strobe (active low)
const int KEYSTRB_PIN = A1; // Keyboard strobe (active low)
const int CLR_PIN = A2; // Clear key input (active low)
const bool DEBUG_PRINTS = true;
const int DISP_POS[16] = {0xF, 0xE, 0xD, 0xC, 0xB, 0xA, 0x9, 0x8, 0x7, 0x6, 0x5, 0x4, 0x3, 0x2, 0x1, 0x0}; // Address corresponding to position

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
// Pin 15 = D2 -> Arduino pin 4           //
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

//////////////////////////////////////////
// A4-A0 addresses for display offsets: //
// DIG15 = left-most character          //
// DIG0 = right-most character          //
// Val A3 A2 A1 A0     Position         //
//   0  L  L  L  L ... DIG15            //
//   1  L  L  L  H ... DIG14            //
//   2  L  L  H  L ... DIG13            //
//   3  L  L  H  H ... DIG12            //
//   4  L  H  L  L ... DIG11            //
//   5  L  H  L  H ... DIG10            //
//   6  L  H  H  L ... DIG9             //
//   7  L  H  H  H ... DIG8             //
//   8  H  L  L  L ... DIG7             //
//   9  H  L  L  H ... DIG6             //
//   A  H  L  H  L ... DIG5             //
//   B  H  L  H  H ... DIG4             //
//   C  H  H  L  L ... DIG3             //
//   D  H  H  L  H ... DIG2             //
//   E  H  H  H  L ... DIG1             //
//   F  H  H  H  H ... DIG0             //
//////////////////////////////////////////

// Keyboard matrix (4 columns x 8 rows)
// I *think* these are the special keys:
// [def] = define
// [bs] = backspace
// [stp] = step
// [sp] = space
// [f] = function
// [clr] = clear
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

// Additionally the keys have shifted, calculator-like functions
// Maybe accessed via [f] key as this doesn't have a shifted function
const String SHIFTED_KEY_MAP[8][4] = 
{
  // COL1     COL2     COL3      COL4
  {"[met]", "[us]",  "[x->m]", "[k]"}, // ROW1
  {"[c1]",  "[c2]",  "[rm]",   "%"},   // ROW2
  {"[exc]", "[+/-]", "[m+]",   "0"},   // ROW3
  {"7",     "8",     "9",      "÷"},   // ROW4
  {"4",     "5",     "6",      "+"},   // ROW5
  {"1",     "2",     "3",      "."},   // ROW6
  {"[p2]",  "[p1]",  "-",      "*"},   // ROW7
  {"[p4]",  "[p3]", "[=]",     ""}     // ROW8
};

void setup() 
{
  Serial.begin(9600);
  Serial.println("Nixdorf LK-3000 Interface Ready");

  // Set pins for display mode by default
  setDisplayMode();
  if (DEBUG_PRINTS) Serial.println(">> Display mode set");

  // Initial clear display
  clearDisplay();
  if (DEBUG_PRINTS) Serial.println(">> Display cleared");

  // Test display with static text
  displayString("HELLO LK-3000   ", 0); // Display initial message
  if (DEBUG_PRINTS) Serial.println(">> 'HELLO LK-3000' displayed");
  delay(2000);
}

void loop() 
{
  // Scroll "HELLO WORLD!" circularly across the display
  static String message = "HELLO WORLD!    HELLO WORLD!    "; // Repeated for seamless circular scroll
  static int offset = 0;
  displayString(message, offset);
  offset = (offset + 1) % message.length(); // Circular scroll
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
  // Ensure character is in DL-1414 range (0x20 to 0x5F)
  if (c < 0x20 || c > 0x5F) c = ' '; // Default to space if out of range
  
  // Map digit position to actual DL-1414 address
  int addr = DISP_POS[pos];
  
  if (DEBUG_PRINTS)
  {
    Serial.print(">> writeChar: pos = ");
    Serial.print(pos);
    Serial.print(", addr = ");
    Serial.print(addr);
    Serial.print(", c = ");
    Serial.println(c);
  }

  // Set address (A0-A3)
  digitalWrite(A_PINS[0], (addr & 0x01) ? HIGH : LOW);
  digitalWrite(A_PINS[1], (addr & 0x02) ? HIGH : LOW);
  digitalWrite(A_PINS[2], (addr & 0x04) ? HIGH : LOW);
  digitalWrite(A_PINS[3], (addr & 0x08) ? HIGH : LOW);

  // Set data (D0-D5)
  // Valid ASCII range is 0x20 (010 0000) to 0x5F (101 1111)
  // D6 is handled "automagically" by the LK-3000, always opposite to D5 value
  digitalWrite(D_PINS[0], (c & 0x01) ? HIGH : LOW);
  digitalWrite(D_PINS[1], (c & 0x02) ? HIGH : LOW);
  digitalWrite(D_PINS[2], (c & 0x04) ? HIGH : LOW);
  digitalWrite(D_PINS[3], (c & 0x08) ? HIGH : LOW);
  digitalWrite(D_PINS[4], (c & 0x10) ? HIGH : LOW);
  digitalWrite(D_PINS[5], (c & 0x20) ? HIGH : LOW);

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
    int idx = (i + offset) % str.length(); // Wrap around for circular scroll
    if (idx < str.length()) c = str.charAt(idx);
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

    if (DEBUG_PRINTS)
    {
      if (cols)
      {
        Serial.print(">> scanKeyboard: row = ");
        Serial.print(row);       
        Serial.print(", cols = ");
        Serial.println(cols);
      }
    }

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

  // Check clear key - special handling - CLR connects to GND
  pinMode(CLR_PIN, INPUT_PULLUP);
  if (digitalRead(CLR_PIN) == LOW) 
  {
    Serial.println("[clr] key pressed");
    clearDisplay();
  }
}