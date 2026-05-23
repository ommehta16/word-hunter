#include <HijelHID_BLEMouse.h>
#include <ESPmDNS.h>

HijelBLEMouse mouse("Cheatmouse","me");

void setup() {
  Serial.begin(115200);
  auto err = mdns_init();
  if (err) {
    Serial.println()
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
      mouse.click(MouseButton::Forward);
      return;
    }

    String start = a.substring(0,spaceInd);
    String end = a.substring(spaceInd+1,a.length());

    int dx = atoi(start.c_str());
    int dy = atoi(end.c_str());
    Serial.println(a);

    mouse.moveTo(dx,dy,100);
  }
}