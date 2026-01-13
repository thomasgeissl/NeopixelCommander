
#pragma once

#include <WiFi.h>
#include <Preferences.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <Adafruit_NeoPixel.h>
#include <ArduinoJson.h>
#include "./helpers.h"

#include "TinyJS.h"
#include "TinyJS_Functions.h"

#define COMMANDS_PER_LOOP 10
#define DEBUG_LOGGING true

class NeopixelCommander
{
public:
  NeopixelCommander(const String &fallbackSsid, const String &fallbackPassword, uint8_t pin, uint16_t numPixels, uint8_t brightness)
      : _fallbackSsid(fallbackSsid), _fallbackPassword(fallbackPassword), _pin(pin), _numPixels(numPixels), _brightness(brightness),
        _server(80), _ws("/ws"), _strip(numPixels, pin, NEO_GRB + NEO_KHZ800),
        _js(new CTinyJS()),
        _connectTimeoutMs(15000), _lastPing(0)
  {
  }

  void setConnectTimeout(uint32_t ms) { _connectTimeoutMs = ms; }

  void begin()
  {
    Serial.begin(115200);
    delay(5000);
    if (DEBUG_LOGGING)
    {
      Serial.println("Starting NeopixelCommander");
    }

    initNeopixels();
    initWifi();
    loadStoredJsCodeFromPreferences();
    initWebServer();
    initJs();

    IPAddress ip = (WiFi.getMode() & WIFI_AP) ? WiFi.softAPIP() : WiFi.localIP();
    if (DEBUG_LOGGING)
    {
      Serial.printf("WebSocket endpoint: ws://%s/ws\n", ip.toString().c_str());
      Serial.printf("HTTP ping endpoint: http://%s/ping\n", ip.toString().c_str());
    }
  }

  void loop()
  {
    checkSerialCommands();
    _ws.cleanupClients();
    if (_executeStoredCode && _lastPing == 0 && (millis() > _executeStoredCodeAfterBootInactivityDuration))
    {
      Serial.print("no ping in ");
      Serial.print(_executeStoredCodeAfterBootInactivityDuration / 1000);
      Serial.println("s ... executing stored JS code");
      _executeStoredCode = false;
      if (_storedJsCode.length() > 0)
      {
        _js->execute(_storedJsCode.c_str());
      }
    }

    if (_useFallbackCredentials && _lastPing == 0 && (millis() > _useFallbackCredentialsAfterBootInactivityDuration))
    {
      Serial.print("no ping in "); 
      Serial.print(_useFallbackCredentialsAfterBootInactivityDuration / 1000);
      Serial.println("s ... using fallback credentials");
      initAPMode();
      _useFallbackCredentials = false;
    }

    // Process multiple commands per loop to keep up with incoming rate

    int processed = 0;
    while (queueStart != queueEnd && processed < COMMANDS_PER_LOOP)
    {
      Command cmd = commandQueue[queueStart];
      queueStart = (queueStart + 1) % QUEUE_SIZE;

      // Execute the command
      switch (cmd.type)
      {
      case SET_PIXEL_COLOR:
        _strip.setPixelColor(cmd.index, cmd.r, cmd.g, cmd.b);
        break;
      case SET_COLOR:
        for (uint16_t i = 0; i < _numPixels; ++i)
        {
          _strip.setPixelColor(i, _strip.Color(cmd.r, cmd.g, cmd.b));
        }
        break;
      case CLEAR:
        _strip.clear();
        break;
      case SHOW:
        _strip.show();
        break;
      case SET_BRIGHTNESS:
        _strip.setBrightness(cmd.brightness);
        break;
      }

      // Send acknowledgment AFTER the command is executed
      AsyncWebSocketClient *client = _ws.client(cmd.clientId);
      if (client && client->status() == WS_CONNECTED)
      {
        char ackMsg[64];
        snprintf(ackMsg, sizeof(ackMsg), "{\"status\":\"ok\",\"ack\":%u}", cmd.commandId);
        client->text(ackMsg);
      }

      processed++;
    }
  }

  // --- initWifi() simplified for Strings ---
  bool initWifi()
  {
    Serial.println("Initializing WiFi...");
    Serial.println("Loading saved network credentials...");

    if (!loadNetworkCredentialsFromPreferences())
    {
      Serial.println("No saved credentials, using fallback");
      _ssid = _fallbackSsid;
      _password = _fallbackPassword;
    }
    else
    {
      Serial.println("Loaded saved credentials");
    }

    Serial.printf("SSID: '%s'\n", _ssid.c_str());
    Serial.printf("Password: '%s'\n", _password.c_str());

    Serial.printf("Trying STA connect to '%s'\n", _ssid.c_str());

    WiFi.mode(WIFI_STA);
    WiFi.begin(_ssid.c_str(), _password.c_str());

    uint32_t start = millis();
    bool staConnected = false;

    while (millis() - start < _connectTimeoutMs)
    {
      if (WiFi.status() == WL_CONNECTED)
      {
        staConnected = true;
        break;
      }
      delay(250);
      if (DEBUG_LOGGING)
        Serial.print(".");
    }

    if (staConnected)
    {
      _wifiState = WIFI_STATE_CONNECTED;

      if (DEBUG_LOGGING)
        Serial.printf("\nConnected as STA. IP: %s\n", WiFi.localIP().toString().c_str());
    }
    else
    {
      if (DEBUG_LOGGING)
        Serial.printf("\nSTA connect failed after %u ms. Starting SoftAP with SSID '%s'\n",
                      (unsigned)_connectTimeoutMs, _ssid.c_str());

      if (_password.length() >= 8)
      {
        WiFi.mode(WIFI_AP_STA);
        if (!WiFi.softAP(_ssid.c_str(), _password.c_str()))
        {
          if (DEBUG_LOGGING)
            Serial.println("softAP() failed. Falling back to open AP.");
          WiFi.softAP(_ssid.c_str());
        }
      }
      else
      {
        if (DEBUG_LOGGING)
          Serial.println("Password too short for WPA2. Starting open AP.");
        WiFi.mode(WIFI_AP);
        WiFi.softAP(_ssid.c_str());
      }

      delay(500);
      if (DEBUG_LOGGING)
        Serial.printf("SoftAP active. AP IP: %s\n", WiFi.softAPIP().toString().c_str());
    }

    return true;
  }

  void initAPMode()
  {
    _wifiState = WIFI_STATE_AP_MODE;

    // Use fallback SSID/password for the AP
    String apSSID = _fallbackSsid;
    String apPassword = _fallbackPassword;

    Serial.printf("Starting Access Point with SSID: '%s'\n", apSSID.c_str());

    if (apPassword.length() >= 8)
    {
      WiFi.mode(WIFI_AP);
      if (!WiFi.softAP(apSSID.c_str(), apPassword.c_str()))
      {
        if (DEBUG_LOGGING)
        {
          Serial.println("softAP() with password failed. Starting open AP.");
        }
        WiFi.softAP(apSSID.c_str());
      }
      else
      {
        if (DEBUG_LOGGING)
        {
          Serial.printf("AP started with WPA2 password\n");
        }
      }
    }
    else
    {
      if (DEBUG_LOGGING)
      {
        Serial.println("Password too short for WPA2 (needs 8+ chars). Starting open AP.");
      }
      WiFi.mode(WIFI_AP);
      WiFi.softAP(apSSID.c_str());
    }

    delay(500);

    if (DEBUG_LOGGING)
    {
      Serial.printf("✓ AP active: '%s'\n", apSSID.c_str());
      Serial.printf("  IP Address: %s\n", WiFi.softAPIP().toString().c_str());
      Serial.printf("  Connect to this network and visit: http://%s\n", WiFi.softAPIP().toString().c_str());
    }
  }

  bool initNeopixels()
  {
    _strip.setBrightness(_brightness);
    _strip.begin();
    _strip.show();
    return true;
  }

  bool initWebServer()
  {
    _ws.onEvent([this](AsyncWebSocket *server, AsyncWebSocketClient *client,
                       AwsEventType type, void *arg, uint8_t *data, size_t len)
                { this->_onWsEvent(server, client, type, arg, data, len); });

    _server.addHandler(&_ws);

    // Enable CORS for all routes
    DefaultHeaders::Instance().addHeader("Access-Control-Allow-Origin", "*");
    DefaultHeaders::Instance().addHeader("Access-Control-Allow-Methods", "GET, POST, PUT, DELETE, OPTIONS");
    DefaultHeaders::Instance().addHeader("Access-Control-Allow-Headers", "Content-Type");

    // Handle OPTIONS requests for CORS preflight
    _server.onNotFound([](AsyncWebServerRequest *request)
                       {
      if (request->method() == HTTP_OPTIONS) {
        request->send(200);
      } else {
        request->send(404);
      } });

    // HTTP Pixel count endpoint
    _server.on("/api/pixelCount", HTTP_GET, [this](AsyncWebServerRequest *request)
               {
      char response[64];
      snprintf(response, sizeof(response), "{\"status\":\"ok\",\"pixelCount\":%u}", _numPixels);
      request->send(200, "application/json", response); });

    // HTTP Ping endpoint
    _server.on("/ping", HTTP_GET, [this](AsyncWebServerRequest *request)
               { 
                _lastPing = millis();
                request->send(200, "application/json", "{\"status\":\"ok\",\"message\":\"pong\"}"); });

    _server.on("/ping", HTTP_POST, [this](AsyncWebServerRequest *request)
               { 
                _lastPing = millis();
                request->send(200, "application/json", "{\"status\":\"ok\",\"message\":\"pong\"}"); });

    _server.on("/api/setColor", HTTP_POST, [this](AsyncWebServerRequest *request)
               {
      if (request->hasParam("r", true) && request->hasParam("g", true) && request->hasParam("b", true))
      {
        int r = request->getParam("r", true)->value().toInt();
        int g = request->getParam("g", true)->value().toInt();
        int b = request->getParam("b", true)->value().toInt();
        this->setColor(r, g, b);
        request->send(200, "application/json", "{\"status\":\"ok\"}");
      }
      else
      {
        request->send(400, "application/json", "{\"status\":\"error\",\"error\":\"missing_params\"}");
      } });

    _server.on("/api/clear", HTTP_POST, [this](AsyncWebServerRequest *request)
               {
      _strip.clear();
      request->send(200, "application/json", "{\"status\":\"ok\"}"); });

    _server.on("/api/clear", HTTP_GET, [this](AsyncWebServerRequest *request)
               {
      _strip.clear();
      request->send(200, "application/json", "{\"status\":\"ok\"}"); });

    _server.on("/api/setBrightness", HTTP_POST, [this](AsyncWebServerRequest *request)
               {
      if (request->hasParam("brightness", true))
      {
        int b = request->getParam("brightness", true)->value().toInt();
        _strip.setBrightness(b);
        request->send(200, "application/json", "{\"status\":\"ok\"}");
      }
      else
      {
        request->send(400, "application/json", "{\"status\":\"error\",\"error\":\"missing_param\"}");
      } });

    _server.on("/api/show", HTTP_POST, [this](AsyncWebServerRequest *request)
               {
      this->show();
      request->send(200, "application/json", "{\"status\":\"ok\"}"); });
    _server.on("/api/show", HTTP_GET, [this](AsyncWebServerRequest *request)
               {
      this->show();
      request->send(200, "application/json", "{\"status\":\"ok\"}"); });

    // HTTP evalJS endpoint - NEW!
    // GET version for easy browser testing
    _server.on("/api/evaljs", HTTP_GET, [this](AsyncWebServerRequest *request)
               {
      if (request->hasParam("code"))
      {
        String jsCode = request->getParam("code")->value();
        if (DEBUG_LOGGING)
          Serial.printf("Received evaljs GET request, code length: %u\n", jsCode.length());

        // bool success = this->evalJS(jsCode);
        // if (success)
        // {
        //   request->send(200, "application/json", "{\"status\":\"ok\"}");
        // }
        // else
        // {
        //   request->send(500, "application/json", "{\"status\":\"error\",\"error\":\"eval_failed\"}");
        // }
      }
      else
      {
        request->send(400, "application/json", "{\"status\":\"error\",\"error\":\"missing_code\"}");
      } });

    // POST version with body handler
    _server.on("/api/evaljs", HTTP_POST, [this](AsyncWebServerRequest *request)
               {
      // Check if code was sent as URL parameter first
      if (request->hasParam("code", true))
      {
        String jsCode = request->getParam("code", true)->value();
        if (DEBUG_LOGGING)
          Serial.printf("Received evaljs POST (param), code length: %u\n", jsCode.length());

        // bool success = this->evalJS(jsCode);
        // if (success)
        // {
        //   request->send(200, "application/json", "{\"status\":\"ok\"}");
        // }
        // else
        // {
        //   request->send(500, "application/json", "{\"status\":\"error\",\"error\":\"eval_failed\"}");
        // }
      }
      else
      {
        // Body will be handled by the body handler below
        request->send(400, "application/json", "{\"status\":\"error\",\"error\":\"no_code_received\"}");
      } }, NULL, [this](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total)
               {
      // Handle raw body data
      if (index + len == total)
      {
        String jsCode = "";
        for (size_t i = 0; i < len; i++)
        {
          jsCode += (char)data[i];
        }

        if (DEBUG_LOGGING){
          Serial.printf("Received evaljs POST (body), code length: %u\n", jsCode.length());
        }

        // bool success = this->evalJS(jsCode);
        // if (success)
        // {
        //   request->send(200, "application/json", "{\"status\":\"ok\"}");
        // }
        // else
        // {
        //   request->send(500, "application/json", "{\"status\":\"error\",\"error\":\"eval_failed\"}");
        // }
      } });

    _server.on("/api/savejs", HTTP_POST, [this](AsyncWebServerRequest *request)
               {
      // Check if code was sent as URL parameter first
      if (request->hasParam("code", true))
      {
        String jsCode = request->getParam("code", true)->value();
        if (DEBUG_LOGGING)
          Serial.printf("Received evaljs POST (param), code length: %u\n", jsCode.length());
        bool success = this->saveJsCodeToPreferences(jsCode);
        if (success)
        {
          request->send(200, "application/json", "{\"status\":\"ok\"}");
        }
        else
        {
          request->send(500, "application/json", "{\"status\":\"error\",\"error\":\"save_failed\"}");
        }
      }
      else
      {
        // Body will be handled by the body handler below
        request->send(400, "application/json", "{\"status\":\"error\",\"error\":\"no_code_received\"}");
      } }, NULL, [this](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total)
               {
      // Handle raw body data
      if (index + len == total)
      {
        String jsCode = "";
        for (size_t i = 0; i < len; i++)
        {
          jsCode += (char)data[i];
        }

        if (DEBUG_LOGGING){
          Serial.printf("Received evaljs POST (body), code length: %u\n", jsCode.length());
        }

        // bool success = this->evalJS(jsCode);
        // if (success)
        // {
        //   request->send(200, "application/json", "{\"status\":\"ok\"}");
        // }
        // else
        // {
        //   request->send(500, "application/json", "{\"status\":\"error\",\"error\":\"eval_failed\"}");
        // }
      } });

    _server.begin();
    return true;
  }
  bool stopWebServer()
  {
    _ws.closeAll();
    _ws.cleanupClients();
    _server.reset();
    _server.end();
  }

  static void js_clear(CScriptVar *var, void *userdata)
  {
    NeopixelCommander *self = static_cast<NeopixelCommander *>(userdata);
    if (!self)
      return;

    self->_strip.clear();
    // self->_strip.show();   // optional: update LEDs immediately
  }
  static void js_show(CScriptVar *var, void *userdata)
  {
    NeopixelCommander *self = static_cast<NeopixelCommander *>(userdata);
    if (!self)
      return;

    self->_strip.show();
    // self->_strip.show();   // optional: update LEDs immediately
  }
  static void js_delay(CScriptVar *var, void *userdata)
  {
    CScriptVar *value = var->getParameter("value");
    if (!value)
      return;
    delay(value->getInt());
  }
  static void js_setPixelColor(CScriptVar *var, void *userdata)
  {
    NeopixelCommander *self = static_cast<NeopixelCommander *>(userdata);
    if (!self)
    {
      return;
    }
    CScriptVar *index = var->getParameter("index");
    CScriptVar *r = var->getParameter("r");
    CScriptVar *g = var->getParameter("g");
    CScriptVar *b = var->getParameter("b");
    if (!index || !r || !g || !b)
      return;
    self->_strip.setPixelColor(index->getInt(), self->_strip.Color(r->getInt(), g->getInt(), b->getInt()));
  }
  static void js_setAllPixelColor(CScriptVar *var, void *userdata)
  {
    NeopixelCommander *self = static_cast<NeopixelCommander *>(userdata);
    if (!self)
    {
      return;
    }
    CScriptVar *r = var->getParameter("r");
    CScriptVar *g = var->getParameter("g");
    CScriptVar *b = var->getParameter("b");
    if (!r || !g || !b)
      return;
    Serial.printf("js_setAllPixelColor called: r=%d, g=%d, b=%d\n", r->getInt(), g->getInt(), b->getInt());
    for (int i = 0; i < self->_strip.numPixels(); i++)
    {
      self->_strip.setPixelColor(i, self->_strip.Color(r->getInt(), g->getInt(), b->getInt()));
    }
  }

  bool initJs()
  {
    registerFunctions(_js);
    _js->addNative("function clear()", &NeopixelCommander::js_clear, this);
    _js->addNative("function show()", &NeopixelCommander::js_show, this);
    _js->addNative("function setPixelColor(index, r, g, b)", &NeopixelCommander::js_setPixelColor, this);
    _js->addNative("function setAllPixelColor(r, g, b)", &NeopixelCommander::js_setAllPixelColor, this);
    _js->addNative("function delay(value)", &NeopixelCommander::js_delay, this);

    try
    {
      // auto result = _js.evaluate("var i = 0; i++;");
      // _js->execute("clear();");
      // _js->execute("show();");
      // _js->execute("delay(1000);");
      // _js->execute("var i = 0;");
      // _js->execute("for(var i = 0; i < 10; i++) { delay(i); }");
      // auto result = _js->evaluate("i=10;");
      // Serial.println(result.c_str());
      // if (_storedJsCode.length() > 0)
      // {
      //   _js->execute(_storedJsCode.c_str());
      // }
    }
    catch (CScriptException *e)
    {
      printf("ERROR: %s\n", e->text.c_str());
    }
    return true;
  }

  void setColor(uint8_t r, uint8_t g, uint8_t b)
  {
    for (uint16_t i = 0; i < _numPixels; ++i)
      _strip.setPixelColor(i, _strip.Color(r, g, b));
  }

  void setPixelColor(uint16_t n, uint8_t r, uint8_t g, uint8_t b)
  {
    if (n < _numPixels)
    {
      _strip.setPixelColor(n, _strip.Color(r, g, b));
    }
  }

  void clear()
  {
    _strip.clear();
  }

  void show()
  {
    _strip.show();
  }

  void setBrightness(uint8_t brightness)
  {
    _strip.setBrightness(brightness);
  }
  uint16_t getNumPixels()
  {
    return _numPixels;
  }

  void setExecuteStoredCodeAfterBootInactivity(int value)
  {
    _executeStoredCode = true;
    _executeStoredCodeAfterBootInactivityDuration = value;
  }
  void setUseFallbackCredentialsAfterBootInactivity(int value)
  {
    _useFallbackCredentials = true;
    _useFallbackCredentialsAfterBootInactivityDuration = value;
  }

private:
  enum WifiState
  {
    WIFI_STATE_CONNECTED,
    WIFI_STATE_AP_MODE
  };

  WifiState _wifiState = WIFI_STATE_AP_MODE;
  static constexpr uint32_t SERIAL_CLIENT_ID = 0xFFFFFFFF;

  Preferences _preferences;
  static const uint16_t QUEUE_SIZE = 512;

  String _ssid;
  String _password;
  String _fallbackSsid;
  String _fallbackPassword;

  uint8_t _pin;
  uint16_t _numPixels;
  uint8_t _brightness;

  AsyncWebServer _server;
  AsyncWebSocket _ws;
  Adafruit_NeoPixel _strip;
  CTinyJS *_js;

  uint32_t _connectTimeoutMs;

  String _storedJsCode;
  bool _executeStoredCode = false;
  int _executeStoredCodeAfterBootInactivityDuration = 60000; // ms
  bool _useFallbackCredentials = false;
  int _useFallbackCredentialsAfterBootInactivityDuration = 30000; // ms
  unsigned long _lastPing;

  Command commandQueue[QUEUE_SIZE];
  uint16_t queueStart = 0;
  uint16_t queueEnd = 0;

  bool saveJsCodeToPreferences(const String &code)
  {
    _preferences.begin("neopixel", false);
    _preferences.putString("js_code", code);
    _preferences.end();
    _storedJsCode = code;
    return true;
  }

  void loadStoredJsCodeFromPreferences()
  {
    _preferences.begin("neopixel", true);
    _storedJsCode = _preferences.getString("js_code", "");
    _preferences.end();

#if (DEBUG_LOGGING)
    if (_storedJsCode.length() > 0)
    {
      Serial.printf("Loaded stored JS code from preferences, length: %u\n", _storedJsCode.length());
      // Serial.println(_storedJsCode.c_str());
    }
    else
    {
      Serial.println("No stored JS code found in preferences.");
    }
#endif
  }

  void clearJsCodeFromPreferences()
  {
    _preferences.begin("neopixel", false);
    _preferences.remove("js_code");
    _preferences.end();
  }

  bool saveNetworkCredentialsToPreferences(const String &ssid, const String &password)
  {
    if (!_preferences.begin("neopixel", false))
    {
      Serial.println("Failed to open preferences!");
      return false;
    }

    size_t ssidWritten = _preferences.putString("wifi_ssid", ssid);
    size_t passWritten = _preferences.putString("wifi_password", password);
    size_t flagWritten = _preferences.putUChar("initialized", 42);

    _preferences.end();

    if (ssidWritten == 0 || passWritten == 0 || flagWritten == 0)
    {
      Serial.println("Failed to write credentials!");
      return false;
    }

    Serial.println("✓ Network credentials saved!");
    return true;
  }
  bool loadNetworkCredentialsFromPreferences()
  {
    _preferences.begin("neopixel", true);
    uint8_t initialized = _preferences.getUChar("initialized", 0);
    String ssid = _preferences.getString("wifi_ssid", "");
    String password = _preferences.getString("wifi_password", "");
    _preferences.end();

    if (initialized != 42 || ssid.length() == 0)
      return false;

    _ssid = ssid;
    _password = password;
    return true;
  }

  bool enqueueCommand(const Command &cmd)
  {
    uint16_t nextEnd = (queueEnd + 1) % QUEUE_SIZE;
    if (nextEnd == queueStart)
    {
      if (DEBUG_LOGGING)
        Serial.printf("WARNING: Command queue full! Dropping command ID %u\n", cmd.commandId);
      return false;
    }
    commandQueue[queueEnd] = cmd;
    queueEnd = nextEnd;
    return true;
  }

  bool evalJS(const String &code)
  {
    return true;
  }
  String getActiveIP() const
  {
    if (_wifiState == WIFI_STATE_AP_MODE)
      return WiFi.softAPIP().toString();
    else
      return WiFi.localIP().toString();
  }

  String getMacAddress() const
  {
    // ESP32 supports per-interface MACs; this returns the active STA MAC,
    // which is normally what clients expect.
    return WiFi.macAddress();
  }

  inline void _onWsEvent(AsyncWebSocket *server, AsyncWebSocketClient *client,
                         AwsEventType type, void *arg, uint8_t *data, size_t len)
  {
    if (type == WS_EVT_CONNECT)
    {
      if (DEBUG_LOGGING)
        Serial.printf("WebSocket client #%u connected\n", client->id());
    }
    else if (type == WS_EVT_DISCONNECT)
    {
      if (DEBUG_LOGGING)
        Serial.printf("WebSocket client #%u disconnected\n", client->id());
    }
    else if (type == WS_EVT_DATA)
    {
      AwsFrameInfo *info = (AwsFrameInfo *)arg;
      if (info->final && info->opcode == WS_TEXT)
      {
        data[len] = 0;

        // Check for simple "ping" message (not JSON)
        if (strcmp((char *)data, "ping") == 0)
        {
          if (DEBUG_LOGGING)
          {
            Serial.printf("Received WebSocket ping from client #%u\n", client->id());
          }
          _lastPing = millis();
          client->text("{\"status\":\"ok\",\"message\":\"pong\"}");
          return;
        }

        handleJsonCommand(
            (char *)data,
            client->id(),
            [client](const char *msg)
            {
              client->text(msg);
            });
      }
    }
  }
  void handleJsonCommand(
      const char *json,
      uint32_t clientId,
      std::function<void(const char *reply)> reply)
  {
    StaticJsonDocument<512> doc;
    if (deserializeJson(doc, json) != DeserializationError::Ok)
    {
      reply("{\"status\":\"error\",\"error\":\"bad_json\"}");
      return;
    }

    const char *cmd = doc["cmd"] | "";
    uint32_t id = doc["id"] | 0;

    // ping
    if (strcmp(cmd, "ping") == 0)
    {
      _lastPing = millis();
      reply("{\"status\":\"ok\",\"message\":\"pong\"}");
      return;
    }

    // setCredentials
    if (strcmp(cmd, "setCredentials") == 0)
    {
      String ssid = doc["ssid"] | "";
      String password = doc["password"] | "";

      if (ssid.length() == 0 || password.length() == 0)
      {
        reply("{\"status\":\"error\",\"error\":\"missing_credentials\"}");
        return;
      }

      _ssid = ssid;
      _password = password;

      saveNetworkCredentialsToPreferences(_ssid, _password);

      Serial.println("Updated WiFi credentials. Reconnecting...");
      initWifi();
      reply("{\"status\":\"ok\"}");
      return;
    }

    // getPixelCount
    if (strcmp(cmd, "getPixelCount") == 0)
    {
      char buf[64];
      snprintf(buf, sizeof(buf), "{\"status\":\"ok\",\"pixelCount\":%u}", _numPixels);
      reply(buf);
      return;
    }

    // saveJS
    if (strcmp(cmd, "saveJS") == 0)
    {
      const char *code = doc["code"] | "";
      if (!code[0])
      {
        reply("{\"status\":\"error\",\"error\":\"missing_code\"}");
        return;
      }
      bool ok = saveJsCodeToPreferences(code);
      if (ok)
      {
        char buf[64];
        snprintf(buf, sizeof(buf), "{\"status\":\"ok\",\"ack\":%u}", id);
        reply(buf);
      }
      else
      {
        reply("{\"status\":\"error\",\"error\":\"save_failed\"}");
      }
      return;
    }

    // All remaining commands require id
    if (id == 0)
    {
      reply("{\"status\":\"error\",\"error\":\"missing_id\"}");
      return;
    }

    Command command;
    command.clientId = clientId;
    command.commandId = id;
    bool valid = true;

    if (strcmp(cmd, "setColor") == 0)
    {
      command.type = SET_COLOR;
      command.r = doc["r"] | 0;
      command.g = doc["g"] | 0;
      command.b = doc["b"] | 0;
    }
    else if (strcmp(cmd, "clear") == 0)
    {
      command.type = CLEAR;
    }
    else if (strcmp(cmd, "show") == 0)
    {
      command.type = SHOW;
    }
    else if (strcmp(cmd, "setBrightness") == 0)
    {
      command.type = SET_BRIGHTNESS;
      command.brightness = doc["brightness"] | 255;
    }
    else if (strcmp(cmd, "setPixelColor") == 0)
    {
      command.type = SET_PIXEL_COLOR;
      command.index = doc["index"] | 0;
      command.r = doc["r"] | 0;
      command.g = doc["g"] | 0;
      command.b = doc["b"] | 0;

      if (command.index >= _numPixels)
      {
        char buf[96];
        snprintf(buf, sizeof(buf),
                 "{\"status\":\"error\",\"error\":\"index_out_of_bounds\",\"id\":%u}", id);
        reply(buf);
        return;
      }
    }
    // getCredentials
    else if (strcmp(cmd, "getCredentials") == 0)
    {
      StaticJsonDocument<256> resp;
      resp["status"] = "ok";

      if (_wifiState == WIFI_STATE_AP_MODE)
      {
        // In AP mode the device is advertising the fallback network
        resp["ssid"] = _fallbackSsid;
        resp["password"] = _fallbackPassword;
        resp["wifiMode"] = "ap";
        resp["ip"] = WiFi.softAPIP().toString();
      }
      else
      {
        // In STA mode we report the configured network
        resp["ssid"] = _ssid;
        resp["password"] = _password;
        resp["wifiMode"] = "sta";
        resp["ip"] = WiFi.localIP().toString();
      }
      resp["mac"] = getMacAddress();

      char buf[256];
      serializeJson(resp, buf, sizeof(buf));
      reply(buf);
      return;
    }
    else
    {
      char buf[80];
      snprintf(buf, sizeof(buf), "{\"status\":\"error\",\"error\":\"unknown_cmd\",\"id\":%u}", id);
      reply(buf);
      return;
    }

    if (!enqueueCommand(command))
    {
      char buf[80];
      snprintf(buf, sizeof(buf), "{\"status\":\"error\",\"error\":\"queue_full\",\"id\":%u}", id);
      reply(buf);
    }
  }

  void checkSerialCommands()
  {
    while (Serial.available())
    {
      String line = Serial.readStringUntil('\n');
      line.trim();
      if (!line.length())
        continue;

      handleJsonCommand(
          line.c_str(),
          SERIAL_CLIENT_ID,
          [](const char *msg)
          {
            Serial.println(msg);
          });
    }
  }
};