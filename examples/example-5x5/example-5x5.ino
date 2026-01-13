#include <NeopixelCommander.h>

NeopixelCommander *neopixelCommander = nullptr;

void setup()
{
  Serial.begin(115200);
  delay(5000);

  //generate a deterministic ssid and password
  //based on mac address, persistent accross reboots and very likely not conflicting with other devices in the same range
  auto seed = macToNumber();
  auto pw_1 = (seed % 8) + 1;
  auto pw_10 = ((seed / 9) % 8) + 1;
  auto pw_100 = ((seed / 81) % 8) + 1;

  auto pw = pw_100 * 100 + pw_10 * 10 + pw_1 * 1;
  String ssidString = "neopixels-";
  ssidString += pw;

  String passwordString = "password-";
  passwordString += pw;

  Serial.print("pw: ");
  Serial.println(passwordString);

  neopixelCommander = new NeopixelCommander(ssidString.c_str(), passwordString.c_str(), 2, 25, 255);

  neopixelCommander->begin();
  neopixelCommander->setUseFallbackCredentialsAfterBootInactivity(60000);
  neopixelCommander->setExecuteStoredCodeAfterBootInactivity(120000);
}

void loop()
{
  neopixelCommander->loop();
}
