// Send a "button" event to the edge on every press.
//
// "button" has no handlers: the edge can read its last value (1 after the
// first press, 0 before), and a write is rejected as not supported.

#include <Meo3.h>

const int buttonPin = 4;

MeoDevice meo("Classroom Button");

void setup() {
  pinMode(buttonPin, INPUT_PULLUP);
  meo.addCap("button");
  meo.begin();
}

void loop() {
  meo.loop();

  static int lastButton = HIGH;
  int button = digitalRead(buttonPin);
  if (button == LOW && lastButton == HIGH) {
    meo.sendEvent("button", 1);
  }
  lastButton = button;
}
