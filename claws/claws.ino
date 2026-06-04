#include <HijelHID_BLEMouse.h>
#include <ESPmDNS.h>
#include <WiFi.h>

// Wifi + server code from https://randomnerdtutorials.com/esp32-web-server-arduino-ide/

HijelBLEMouse mouse("Cheatmouse","me");

const char* ssid = "Van (Candies Inside!)";
const char* password = "password";

WiFiServer server(80);

// Variable to store the HTTP request
String header;

// Current time
unsigned long currentTime = millis();
// Previous time
unsigned long previousTime = 0; 
// Define timeout time in milliseconds (example: 2000ms = 2s)
const long timeoutTime = 2000;


void setup() {
  Serial.begin(115200);
  mouse.begin();

  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  // Print local IP address and start web server
  Serial.println("");
  Serial.println("WiFi connected.");

  esp_err_t err = mdns_init();
  if (err) {
      Serial.println("MDNS Init failed " + err);
      return;
  }

  mdns_hostname_set("cheatmouse");
  mdns_instance_name_set("cheatmouse");

  server.begin();
}

void loop() {
  if (!mouse.isPaired()) {
    delay(10);
    return;
  }
  if (Serial.available()) {
    String a = Serial.readString();
    int spaceInd = a.indexOf(' ');
    
    if (spaceInd == -1) {
      startCheat();
      return;
    }

    String start = a.substring(0,spaceInd);
    String end = a.substring(spaceInd+1,a.length());

    int dx = atoi(start.c_str());
    int dy = atoi(end.c_str());
    Serial.println(a);

    mouse.move(dx,dy);
  }

  WiFiClient client = server.available();   // Listen for incoming clients
  if (!client) return;
  // If a new client connects,
  currentTime = millis();
  unsigned long start = currentTime;
  previousTime = currentTime;
  Serial.println("New Client.");          // print a message out in the serial port
  String currentLine = "";                // make a String to hold incoming data from the client
  while (client.connected() && currentTime - previousTime <= timeoutTime) {  // loop while the client's connected
    currentTime = millis();
    if (client.available()) {             // if there's bytes to read from the client,
      char c = client.read();             // read a byte, then
      header += c;
      if (c == '\n') {                    // if the byte is a newline character
        // if the current line is blank, you got two newline characters in a row.
        // that's the end of the client HTTP request, so send a response:
        // HTTP headers always start with a response code (e.g. HTTP/1.1 200 OK)
        // and a content-type so the client knows what's coming, then a blank line:
        Serial.println(header);

        client.println("HTTP/1.1 200 OK");
        client.println("Connection: close");
        // The HTTP response ends with another blank line
        client.println();
        // Break out of the while loop
        break;
      } else if (c != '\r') {  // if you got anything else but a carriage return character,
        currentLine += c;      // add it to the end of the currentLine
      }
    }
  }
  // Clear the header variable
  header = "";
  // Close the connection
  client.stop();
  Serial.println(millis()-start);
  Serial.println("Client disconnected.\n");
}

void startCheat() {
  delay(50);
  mouse.move(0,10);
  delay(50);
  mouse.move(-100, 0);
  delay(50);
  mouse.move(31,0);
  delay(50);
  mouse.move(0,-100);
  delay(50);
  mouse.move(0,30);
  delay(50);
  mouse.move(0,30);
  delay(50);
  mouse.move(0,30);
  delay(1000);
  mouse.click(MouseButton::Left);
  delay(250);
  mouse.click(MouseButton::Forward);
  delay(200);
  mouse.move(-2,-12);
  delay(50);
}