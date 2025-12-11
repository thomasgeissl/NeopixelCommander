#pragma once
#include <WiFi.h>


// Add this to your NeopixelCommander class
uint32_t ColorHSV(uint16_t hue, uint8_t sat = 255, uint8_t val = 255) {
  uint8_t r, g, b;
  
  // Convert HSV to RGB
  if (sat == 0) {
    r = g = b = val;
  } else {
    uint8_t region = hue / 43;
    uint8_t remainder = (hue - (region * 43)) * 6;
    
    uint8_t p = (val * (255 - sat)) >> 8;
    uint8_t q = (val * (255 - ((sat * remainder) >> 8))) >> 8;
    uint8_t t = (val * (255 - ((sat * (255 - remainder)) >> 8))) >> 8;
    
    switch (region) {
      case 0:  r = val; g = t;   b = p;   break;
      case 1:  r = q;   g = val; b = p;   break;
      case 2:  r = p;   g = val; b = t;   break;
      case 3:  r = p;   g = q;   b = val; break;
      case 4:  r = t;   g = p;   b = val; break;
      default: r = val; g = p;   b = q;   break;
    }
  }
  
  return ((uint32_t)r << 16) | ((uint32_t)g << 8) | b;
}

enum CommandType
{
  SET_PIXEL_COLOR,
  SET_COLOR,
  CLEAR,
  SHOW,
  SET_BRIGHTNESS,
  SET_CREDENTIALS
};

struct Command
{
  CommandType type;
  uint16_t index;
  uint8_t r;
  uint8_t g;
  uint8_t b;
  uint8_t brightness;
  uint32_t clientId;
  uint32_t commandId; // unique ID for this command
  String ssid;
  String password;
};


uint32_t macToNumber() {
  uint8_t mac[6];
  WiFi.macAddress(mac);

  uint32_t v = 0;
  for (int i = 0; i < 6; ++i) {
    v = (v << 5) - v + mac[i];
  }
  return v;
}