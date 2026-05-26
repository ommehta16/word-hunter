#include <HijelHID_BLEMouse.h>
#include <ESPmDNS.h>

HijelBLEMouse mouse("Cheatmouse","me");

void setup() {
  Serial.begin(115200);
  auto err = mdns_init();
  if (err) {
    Serial.println();
  }
  mouse.begin();
}

void loop() {
  // put your main code here, to run repeatedly:
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
}

void startCheat() {
  // mouse.click(MouseButton::Forward);
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