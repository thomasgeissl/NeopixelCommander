#include <NeopixelCommander.h>

NeopixelCommander *neopixelCommander = nullptr;

void setup()
{
  Serial.begin(115200);

  //generate a deterministic ssid and password
  //based on mac address, persistent accross reboots and very likely not conflicting with other devices in the same range
  auto seed = macToNumber();
  auto pw_1 = (seed % 8) + 1;
  auto pw_10 = ((seed / 9) % 8) + 1;
  auto pw_100 = ((seed / 81) % 8) + 1;

  auto pw = pw_100 * 100 + pw_10 * 10 + pw_1 * 1;
  Serial.println("pw:");
  Serial.println(pw);
  String ssidString = "neopixels-";
  ssidString += pw;

  String passwordString = "password-";
  passwordString += pw;

  // neopixelCommander = new NeopixelCommander("how-soon-is-now?", "a!!mylov!n", 2, 64, 127);
  neopixelCommander = new NeopixelCommander(ssidString.c_str(), passwordString.c_str(), 2, 64, 127);

  neopixelCommander->begin();
  neopixelCommander->setUseFallbackCredentialsAfterBootInactivity(30000);
  neopixelCommander->setExecuteStoredCodeAfterBootInactivity(60000);
}

void loop()
{
  neopixelCommander->loop();
}
