// Push a temperature reading every 5 s and answer reads on demand.
//
// Values are int16, so decimals travel scaled x100: 23.4 °C is sent as 2340.

#include <Meo3.h>

MeoDevice meo("Temperature Sensor");

// Fake sensor, °C x100.
static int16_t readTemperature() {
  return random(2000, 3100);
}

void setup() {
  meo.addCap("temperature", nullptr, readTemperature);
  meo.begin();
}

void loop() {
  meo.loop();

  static unsigned long last = 0;
  if (millis() - last > 5000) {
    last = millis();
    meo.sendEvent("temperature", readTemperature());
  }
}
