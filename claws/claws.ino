/*
 * ESP32-S3 BLE HID Absolute Mouse — Word Hunter
 * Compatible with NimBLE-Arduino v2.x (h2zero)
 *
 * LIBRARY: NimBLE-Arduino by h2zero (install via Library Manager)
 * BOARD:   ESP32S3 Dev Module
 *          USB CDC On Boot: Enabled
 *          Upload Mode:     UART0 / Hardware CDC
 *
 */

#include <NimBLEDevice.h>
#include <NimBLEServer.h>
#include <NimBLEHIDDevice.h>
#include <ESPmDNS.h>
#include <WiFi.h>

/*
**CALIBRATION CONSTANTS**
  buncha vals here to figure out where to move the mouse
    - How do I calibrate?
       1. Hook the esp32 up to serial
       2. Ensure it is also connected to phone's Bluetooth
       3. Enter moveto commands in the format [x] [y]: so for example, try
              5000 5000
          - Keep choosing different numbers until the cursor ends up at the center of the top left letter
          - Record the (x,y) of this as (TL_X, TL_Y)
          - Keep choosing different numbers until the cursor ends up at the center of the letter 1 right and 1 below the top right letter
          - Record the DIFFERENCE of the (x,y) of this and the (x,y) of the top left letter as (INCR_X, INCR_Y)
       4. You're done! Run the bot and it should work finely and dandily :)
  I've left my scratchpad from generating these numbers here if it helps!
*/

// (x,y)
//   2300 4800 = (0,0)
//   2300 5650 = (0,1)
//   2300 6500 = (0,2)
//   2300 7350 = (0,3)

//   4100 4800 = (1,0)

//   +1800x
//   +850y

const int TL_X = 2300;
const int TL_Y = 4800;
const int INCR_X = 1800;
const int INCR_Y = 850;

/** 
</Calibration Constants> 
Nothing in the rest of this file should *need* to change if you're just moving to a different phone
could be wrong though, so this code should be somewhat well documented?
I'm going to break that rule in 2 lines though
*/

// ── HID Report Descriptor: Absolute Mouse (X/Y = 0..9999) ─────────────────
static const uint8_t hidDescriptor[] = { // tbh no idea how this was generated. Copilot apparently?
  // DEVICE Description
  0x05, 0x01,
  0x09, 0x02,
  0xA1, 0x01,
  0x85, 0x01,
  0x09, 0x01,
  0xA1, 0x00,
  // BUTTONS
  0x05, 0x09,
  0x19, 0x01,
  0x29, 0x05,
  0x15, 0x00,
  0x25, 0x01,
  0x95, 0x03,
  0x75, 0x01,
  0x81, 0x02,
  0x95, 0x01,
  0x75, 0x05,
  0x81, 0x03,
  // X/Y AXIS Section
  0x05, 0x01,
  0x09, 0x30,
  0x09, 0x31,
  0x16, 0x00, 0x00,
  0x26, 0x0F, 0x27,
  0x36, 0x00, 0x00,
  0x46, 0x0F, 0x27,
  0x75, 0x10,
  0x95, 0x02,
  0x81, 0x02,
  // End
  0xC0,
  0xC0
};

// ── Globals ────────────────────────────────────────────────────────────────
NimBLEServer*         pServer      = nullptr;
NimBLEHIDDevice*      pHID         = nullptr;
NimBLECharacteristic* pInput       = nullptr;
bool                  bleConnected = false;

uint16_t curX = 4999, curY = 4999;
uint8_t  curBtn = 0;

// Credit for the above ( after `static const uint8_t hidDescriptor[]` ) goes squarely to Github Copilot

const char* ssid = "Van (Candies Inside!)";
const char* password = "password";
WiFiServer server(80);

// Store HTTP request here
String request;

// ── Server Callbacks ───────────────────────────────────────────────────────
class ServerCB : public NimBLEServerCallbacks {
  void onConnect(NimBLEServer* pSrv, NimBLEConnInfo& info) override {
    bleConnected = true;
    Serial.println("\nBLE CONNECTED");
    pSrv->updateConnParams(info.getAddress(), 6, 8, 0, 400);
  }
  void onDisconnect(NimBLEServer* pSrv, NimBLEConnInfo& info, int reason) override {
    bleConnected = false;
    Serial.println("\nBLE DISCONNECTED");
    NimBLEDevice::startAdvertising();
  }
};

// ── HID helpers ───────────────────────────────────────────────────────────

// Send a single HID report
inline void sendReport() {
  if (!bleConnected || !pInput) return;

  uint8_t report[5]; // report is 5 bytes
  report[0] = curBtn; // first byte for button pressed
  report[1] =  curX & 0xFF; report[2] = (curX >> 8) & 0xFF; // 2nd and 3rd for x pos
  report[3] =  curY & 0xFF; report[4] = (curY >> 8) & 0xFF; // 4th and 5th for y pos
  pInput->setValue(report, 5);
  pInput->notify();
}

// Static methods for mouse manipulation
class Mouse {
  private:
    static const uint32_t clickDuration = 40;

  public: 
    // Press mouse button 1
    static void down() {
      curBtn |=  0b0001;
      sendReport();
    }

    // Release mouse button 1
    static void up() { 
      curBtn &= ~0b0001;
      sendReport();
    }

    // Click "back" mouse button
    static void backClick() {
      curBtn |= 0b1000;
      sendReport();
      
      delay(clickDuration);

      curBtn &= ~0b1000;
      sendReport();
    }

    // Click mouse button 1
    static void click() {
      down();
      delay(clickDuration);
      up();
    }

    /* Instantly move to `x`, `y`
     *  - `x`, `y` in a range from 0 to 9999, where (0,0) is top left and (9999,9999) is bottom right
     */
    static void moveTo(uint16_t x, uint16_t y) {
      curX = constrain(x, 0, 9999);
      curY = constrain(y, 0, 9999);
      sendReport();
    }
    
    static const int drag_steps = 6;
    static const int drag_delay = 8;

    // Smoothly move from `x1`, `y1` to `x2`, `y2` over `drag_steps` * `drag_delay` milliseconds
    static void drag(int x1, int y1, int x2, int y2) {
      int steps = drag_steps;
      int ms    = drag_delay;

      for (int i = 1; i <= steps; i++) {
        uint16_t nx = (uint16_t)(x1 + (int32_t)(x2 - x1) * i / steps);
        uint16_t ny = (uint16_t)(y1 + (int32_t)(y2 - y1) * i / steps);
        moveTo(nx, ny);
        delay(ms);
      }
    }

    // Like `drag` but from current coordinates
    // - move to `x`, `y` smoothly over some period of time
    static void dragTo(int x, int y) {
      return drag(curX, curY, x, y);
    }
};

// Start bluetooth absolute mouse
void startBLE() {
  NimBLEDevice::init("Word Hunter");
  NimBLEDevice::setPower(9);
  NimBLEDevice::setSecurityAuth(true, false, true);

  pServer = NimBLEDevice::createServer();
  pServer->setCallbacks(new ServerCB());

  pHID = new NimBLEHIDDevice(pServer);
  pHID->setManufacturer("Word Hunter");
  pHID->setPnp(0x02, 0x045E, 0x0040, 0x0110);
  pHID->setHidInfo(0x00, 0x01);
  pHID->setReportMap((uint8_t*)hidDescriptor, sizeof(hidDescriptor));
  pInput = pHID->getInputReport(1);

  NimBLEAdvertising* pAdv = NimBLEDevice::getAdvertising();
  pAdv->setAppearance(HID_MOUSE);
  pAdv->addServiceUUID(pHID->getHidService()->getUUID());
  pAdv->enableScanResponse(false);
  pAdv->start();

  Serial.println("BLE advertising");
}

void startWiFi() {
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {delay(500); Serial.print(".");}
  Serial.println("\nWiFi connected.");

  esp_err_t err = mdns_init();
  if (err) { Serial.println("MDNS Init failed " + err); return; }
  mdns_hostname_set("huntmouse");
  mdns_instance_name_set("huntmouse");

  // Start web server
  server.begin();
}

// ── Setup ──────────────────────────────────────────────────────────────────
void setup() {
  Serial.begin(115200);

  startBLE();
  startWiFi();
}

// ── Loop ───────────────────────────────────────────────────────────────────
void loop() {
  if (!bleConnected) return delay(10);

  if (Serial.available()) return debug();

  // @TODO add button condition here -> `start_hunt()`

  bool gotText = handleConnection();
  if (gotText) dohunt(request);
}

unsigned long currentTime = millis();
unsigned long previousTime = 0; 
const long timeoutTime = 2000; // Connection timeout in ms

void debug() {
  String inp = Serial.readStringUntil('\n');
  inp.trim();

  Serial.println("Got text: " + inp + "\n");

  // Click testing
  if (inp.equals("mb")) return Mouse::backClick();
  if (inp.equals("md")) return Mouse::down();
  if (inp.equals("mu")) return Mouse::up();
  if (inp.equals("mc")) return Mouse::click();

  // Enter any string not listed above to start word-hunting
  int spaceInd = inp.indexOf(' ');
  if (spaceInd == -1) return start_hunt();

  // Enter `[x] [y]` to move the mouse to those coordinates (within [0,9999] where 0 is left/top, 9999 is right/bottom)
  String x_str = inp.substring(0,spaceInd);
  String y_str = inp.substring(spaceInd+1,inp.length());

  int x = atoi(x_str.c_str());
  int y = atoi(y_str.c_str());

  Mouse::moveTo(x, y);
}

bool handleConnection() {
  WiFiClient client = server.available();   // Listen for incoming clients
  if (!client) return false;

  Serial.println("Got connection request");
  currentTime = millis();
  unsigned long start = currentTime;
  previousTime = currentTime;

  request = "";
  while (client.connected() && currentTime - previousTime <= timeoutTime) {  // loop while the client's connected
    currentTime = millis();
    if (!client.available()) continue;
    
    // Ingest next byte
    char curr_byte = client.read();
    request += curr_byte;

    // Byte is the end of request
    if (curr_byte == '\n') {
      client.println("HTTP/1.1 200 OK");
      client.println("Connection: close");
      client.println("\n");
      break;
    }

    if (curr_byte == '\r') continue;
  }
  // Close the connection
  client.stop();
  return true;
}

unsigned long last_try = 0;
bool running = false;

// Run a loaded move sequence (i.e. DO the word-hunt!)
void dohunt(String head) {  
  // Debounce
  if ((millis() - last_try)/1000 < 10 || running) {
    last_try = millis();
    Serial.println("Blocked second attempt");
    return;
  }

  // Debounce / ~ blocking
  running = true;
  last_try = millis();
  int start_time = millis();

  // All paths: paths seperated by `_`, coordinates seperated by `~`, x/y seperated by `.`
  // In the format `y.x~y.x~y.x_y.x~y.x~y.x_`
  String remaining_words = head.substring(head.indexOf('/')+1,head.lastIndexOf('H')-1) + "_";
  Serial.println(remaining_words);
  
  // Process list of words
  while (remaining_words.length()) {
    // Process current word
    String word_path = remaining_words.substring(0,remaining_words.indexOf('_'));
    play_word(word_path);
    delay(50);

    // Get next word
    remaining_words = remaining_words.substring(remaining_words.indexOf('_')+1);

    // Stop if we've been going for 
    if (millis() - start_time > 1000*90) { // Word hunt game is ~90s long, i think
      Serial.println("Out of time!");
      running = false;
      return;
    }
  }
  running = false;
}

// Parsing helper functions
inline int get_coord_end(String& word_path) {
  int idx = word_path.indexOf('~');
  return idx == -1 ? word_path.length() : idx;
}

inline int get_coord_middle(String& word_path) {
  return word_path.indexOf('.');
}

// Play a single word, based on its single-word path
void play_word(String word_path) {
  Mouse::up();
  delay(50);
  
  Serial.println(word_path);
  bool started_word = false;
  while (word_path.length()) {
    // Parse next target x and y
    int coord_end = get_coord_end(word_path);
    int coord_middle = get_coord_middle(word_path);

    int ty = atoi( word_path.substring( 0             , coord_middle ).c_str() );
    int tx = atoi( word_path.substring( coord_middle+1, coord_end    ).c_str() );

    // Go to coordinate
    dragToCell(tx, ty);

    int nextWordStart = word_path.indexOf('~');
    if (nextWordStart == -1) nextWordStart = word_path.length()-1;

    // Shift word window ahead by one coordinate
    word_path = word_path.substring(nextWordStart+1);

    if (!started_word) {
      delay(50);
      Mouse::down();
      started_word = true;
      delay(50);
      continue;
    }
  }
  Mouse::up();
  // Smallest movement to make it a lillll bit natural for the phone (not sure if this works could be hallucinating but :shrug:)
  Mouse::dragTo(curX+1,curY+1);
  delay(50);
}

inline int cell_to_px_x(int cell_x) {
  return TL_X + INCR_X * cell_x;
}

inline int cell_to_px_y(int cell_y) {
  return TL_Y + INCR_Y * cell_y;
}

// `Mouse::moveTo` but in cell coordinates
void moveToCell(int x, int y) {
  Mouse::moveTo(
    cell_to_px_x(x),
    cell_to_px_y(y)
  );
}

// `Mouse::dragTo` but in cell coordinates
void dragToCell(int x, int y) {
  Mouse::dragTo(
    cell_to_px_x(x),
    cell_to_px_y(y)
  );
}

// Start the game by moving to the "start" button, clicking it, and back-clicking to activate the shortcut
void start_hunt() {
  // Move to start pos, click button, etc.

  delay(50);
  Mouse::moveTo(5000, 7000);
  delay(50);
  Mouse::click();
  Serial.println("ok cool");
  delay(300);
  Mouse::backClick();
}