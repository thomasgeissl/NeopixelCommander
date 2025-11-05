#include <NeopixelCommander.h>

NeopixelCommander neopixelCommander("neopixel_ap", "password", 5, 64, 127);

void setup() {
  Serial.begin(115200);
  neopixelCommander.begin();
  neopixelCommander.setExecuteStoredCodeAfterBootInactivity(60000);
}

void loop() {
  neopixelCommander.loop();
}
