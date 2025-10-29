//////////////////////////////////////////////////////////////////////////////////
// Test Arduino Uno sketch to interface with Nixdorf LK-3000 via cartridge port //
// Demonstrates: 16 character output to the DP-1414 modules                     //
//               keyboard scanning                                              //
//               key debouncing                                                 //
//               using [f] to obtain shifted keys                               //
// Brett Hallen, Oct 2025                                                       //
//////////////////////////////////////////////////////////////////////////////////

// Pin assignments for cartridge port
const int D_PINS[6] = {4, 2, 3, 5, 6, 7}; // D0 to D5 data bus
const int A_PINS[4] = {8, A3, 10, 11}; // A0 to A3 address bus
const int DWSTRB_PIN = A0; // Display write strobe (active low)
const int KEYSTRB_PIN = A1; // Keyboard strobe (active low)
const int CLR_PIN = A2; // Clear key input (active low)
const bool DEBUG_PRINTS = false; // Print debugging text to serial, slows everything down
const bool dispMode = false; // constant used for keyboard mode setting
const bool keybMode = true; // constant used for display mode setting
unsigned long lastDebounceTime = 0; // Last time a key was registered
const unsigned long debounceDelay = 150; // Debounce period in milliseconds
String lastKey = ""; // Track the last key pressed for state comparison
bool isFunctionKeyHeld = false; // Track if [f] key is held

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
  Serial.begin(115200);
  Serial.println("Nixdorf LK-3000 Interface Ready");

  // Set pins for display mode by default
  setMode(dispMode);
  if (DEBUG_PRINTS) Serial.println(">> Display mode set");

  // Initial clear display
  clearDisplay();
  if (DEBUG_PRINTS) Serial.println(">> Display cleared");

  // Test display with static text
  displayString("NIXDORF  LK-3000", 0); // Display initial message
  if (DEBUG_PRINTS) Serial.println(">> 'NIXDORF  LK-3000' displayed");
  delay(2000);
  displayString("0123456789ABCDEF", 0);
  if (DEBUG_PRINTS) Serial.println(">> '0123456789ABCDEF' displayed");
}

void loop() 
{
  // Scroll "HELLO WORLD!" circularly across the display
  //static String message = "HELLO WORLD!    HELLO WORLD!    "; // Repeated for seamless circular scroll
  //static String message = "0123456789ABCDEF";
  //static int offset = 0;
  //displayString(message, offset);
  //offset = (offset + 1) % message.length(); // Circular scroll
  //delay(100); // Scroll speed

  // Scan keyboard every loop
  scanKeyboard();
}

void setMode(bool keyboard)
{
  // Set pins for either reading the keyboard or outputting characters to display
  if (keyboard)
  {
    for (int i = 0; i < 6; i++) 
    {pinMode(D_PINS[i], INPUT_PULLUP);} // Active low keys
  }
  else
  {
    for (int i = 0; i < 6; i++) 
    {pinMode(D_PINS[i], OUTPUT);}
  }
  for (int i = 0; i < 4; i++) 
  {pinMode(A_PINS[i], OUTPUT);}

  pinMode(DWSTRB_PIN, OUTPUT);
  pinMode(KEYSTRB_PIN, OUTPUT);
  pinMode(CLR_PIN, INPUT_PULLUP);
  setStrobe(keybMode); // keyboard strobe usually active unless outputting to display
}

void setStrobe(bool keyboard)
{
  if (keyboard)
  {
    digitalWrite(DWSTRB_PIN, HIGH);
    digitalWrite(KEYSTRB_PIN, LOW); // keyboard strobe
  }
  else
  {
    digitalWrite(DWSTRB_PIN, LOW); // display strobe
    digitalWrite(KEYSTRB_PIN, HIGH);    
  }
}

// Write a character to a specific position (0-15)
void writeChar(byte pos, char c)
{
  // Convert lower case ASCII to upper case
  if (c > 0x60 && c < 0x7B)
  {
    if (DEBUG_PRINTS)
    {
      Serial.print(">> writeChar: converting lower case to upper case, c=");
      Serial.println(c);
    }
    c = c - 0x20;
  } 
  
  // Ensure character is in DL-1414 range (0x20 to 0x5F)
  if (c < 0x20 || c > 0x5F)
  {
    if (DEBUG_PRINTS)
    {
      Serial.print(">> writeChar: c out of range, c=");
      Serial.println(c);
    }
    c = ' '; // Default to space if out of range
  }
  
  // Map digit position to actual DL-1414 address
  int addr = ~pos & 0xF;
  if (DEBUG_PRINTS)
  {
    Serial.print(">> writeChar: pos=");
    Serial.print(pos);
    Serial.print(", addr=");
    Serial.println(addr);
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
  setStrobe(dispMode); // display strobe
  delayMicroseconds(30); 
  setStrobe(keybMode); // switch back to keyboard mode
}

// Display a 16-char string starting from offset in the message
void displayString(String str, int offset) 
{
  setMode(dispMode); // display mode
  for (int i = 0; i < 16; i++) 
  {
    char c = ' '; // Default space
    int idx = (i + offset);
    if (idx < str.length()) c = str.charAt(idx);
    if (DEBUG_PRINTS)
    {
      Serial.print(">> displayString: idx=");
      Serial.print(idx);
      Serial.print(", i=");
      Serial.print(i);
      Serial.print(", c=");
      Serial.println(c);
    }
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
  setMode(keybMode);
  digitalWrite(A_PINS[3], LOW); // A3 = 0 for keyboard (8 rows)
  bool keyPressed = false; // Track if any key is pressed in this scan

  // Check [f] key first (ROW8, COL4)
  digitalWrite(A_PINS[0], (7 & 0x01) ? HIGH : LOW); // ROW8 = 7
  digitalWrite(A_PINS[1], (7 & 0x02) ? HIGH : LOW);
  digitalWrite(A_PINS[2], (7 & 0x04) ? HIGH : LOW);
  setStrobe(keybMode);
  bool functionKeyPressed = (digitalRead(D_PINS[3]) == LOW); // COL4 via D3

  if (functionKeyPressed)
  {
    keyPressed = true;
    unsigned long currentTime = millis();
    if (!isFunctionKeyHeld && (currentTime - lastDebounceTime >= debounceDelay))
    {
      // Register [f] key press only on initial press
      Serial.println("Key pressed: [f]");
      clearDisplay();
      displayString("[f]", 0);
      lastKey = "[f]";
      lastDebounceTime = currentTime;
    }
    isFunctionKeyHeld = true; // Set flag to use SHIFTED_KEY_MAP
    if (DEBUG_PRINTS && currentTime - lastDebounceTime < debounceDelay)
    {
      Serial.println("Debouncing [f]");
    }
  }
  else
  {
    isFunctionKeyHeld = false; // Reset when [f] is released
  }

  // Scan the rest of the keyboard matrix
  for (int row = 0; row < 8; row++) 
  {
    // Set row address (A0-A2)
    digitalWrite(A_PINS[0], (row & 0x01) ? HIGH : LOW);
    digitalWrite(A_PINS[1], (row & 0x02) ? HIGH : LOW);
    digitalWrite(A_PINS[2], (row & 0x04) ? HIGH : LOW);

    // Pulse strobe
    setStrobe(keybMode);
    int cols = 0;
    if (digitalRead(D_PINS[0]) == LOW) cols |= 0x01; // COL1 via D0
    if (digitalRead(D_PINS[1]) == LOW) cols |= 0x02; // COL2 via D1
    if (digitalRead(D_PINS[2]) == LOW) cols |= 0x04; // COL3 via D2
    if (digitalRead(D_PINS[3]) == LOW) cols |= 0x08; // COL4 via D3

    if (cols && DEBUG_PRINTS)
    {
      Serial.print(">> scanKeyboard: row = ");
      Serial.print(row);       
      Serial.print(", cols = ");
      Serial.println(cols);
    }

    // Check for pressed keys
    for (int col = 0; col < 4; col++) 
    {
      if (cols & (1 << col)) 
      {
        // Skip [f] key here since it was handled above
        if (row == 7 && col == 3) continue;

        String key = isFunctionKeyHeld ? SHIFTED_KEY_MAP[row][col] : KEY_MAP[row][col];
        // Skip empty keys in SHIFTED_KEY_MAP
        if (key == "") continue;

        keyPressed = true;
        unsigned long currentTime = millis();
        // Check if enough time has passed since the last key registration
        if (currentTime - lastDebounceTime >= debounceDelay)
        {
          // Register the key press
          Serial.print("Key pressed: ");
          Serial.println(key);
          clearDisplay();
          displayString(key, 0);
          lastKey = key;
          lastDebounceTime = currentTime;
        }
        else if (key == lastKey && DEBUG_PRINTS)
        {
          Serial.print("Debouncing ");
          Serial.println(key);
        }
      }
    }
    delay(2); // Pause between column check
  }

  // Check clear key - special handling
  unsigned long currentTime = millis();
  if (digitalRead(CLR_PIN) == LOW) 
  {
    keyPressed = true;
    if (currentTime - lastDebounceTime >= debounceDelay)
    {
      Serial.println("[clr] key pressed");
      clearDisplay();
      displayString("[clr]", 0);
      lastKey = "[clr]";
      lastDebounceTime = currentTime;
    }
    else if (DEBUG_PRINTS)
    {
      Serial.println("Debouncing [clr]");
    }
  }

  // If no key is pressed in this scan, reset lastKey to allow immediate re-press
  if (!keyPressed && lastKey != "") lastKey = "";
}