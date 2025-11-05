
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
  NeopixelCommander(const char *ssid, const char *password, uint8_t pin, uint16_t numPixels, uint8_t brightness)
      : _ssid(ssid), _password(password), _pin(pin), _numPixels(numPixels), _brightness(brightness),
        _server(80), _ws("/ws"), _strip(numPixels, pin, NEO_GRB + NEO_KHZ800),
        _js(new CTinyJS()),
        _connectTimeoutMs(15000), _lastPing(0)
  {
  }

  void setConnectTimeout(uint32_t ms) { _connectTimeoutMs = ms; }

  void begin()
  {
    Serial.begin(115200);
    delay(1000);
    if (DEBUG_LOGGING)
    {
      Serial.println("Starting NeopixelCommander");
    }

    initNeopixels();
    initWifi();
    loadStoredJsCodeFromPreferences();
    initWebServer();
    initJs();

    // IPAddress ip = (WiFi.getMode() & WIFI_AP) ? WiFi.softAPIP() : WiFi.localIP();
    // if (DEBUG_LOGGING)
    // {
    //   Serial.printf("WebSocket endpoint: ws://%s/ws\n", ip.toString().c_str());
    //   Serial.printf("HTTP ping endpoint: http://%s/ping\n", ip.toString().c_str());
    // }

    // if (setupAnimation)
    // {
    //   setupAnimation();
    // }
  }

  void loop()
  {
    _ws.cleanupClients();
    // if (!_duktapeInitialized && _lastPing == 0 && millis() > 60000)
    // {
    //   Serial.println("no ping in 60s ...");
    //   stopWebServer();
    //   initDuktape();
    //   _duktapeInitialized = true;
    //   evalJS(_storedJsCode);
    // }

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

  bool initWifi()
  {
    Serial.printf("Trying STA connect to '%s'\n", _ssid);
    WiFi.mode(WIFI_STA);
    WiFi.begin(_ssid, _password);

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
      if (DEBUG_LOGGING)
      {
        Serial.printf("\nConnected as STA. IP: %s\n", WiFi.localIP().toString().c_str());
      }
    }
    else
    {
      if (DEBUG_LOGGING)
      {
        Serial.printf("\nSTA connect failed after %u ms. Starting SoftAP with SSID '%s'\n",
                      (unsigned)_connectTimeoutMs, _ssid);
      }

      if (_password != nullptr && strlen(_password) >= 8)
      {
        WiFi.mode(WIFI_AP_STA);
        bool ok = WiFi.softAP(_ssid, _password);
        if (!ok)
        {
          if (DEBUG_LOGGING)
          {
            Serial.println("softAP() returned false. Attempting open AP (no password).");
          }
          WiFi.softAP(_ssid);
        }
      }
      else
      {
        if (DEBUG_LOGGING)
        {
          Serial.println("Password too short for WPA2; starting open AP.");
        }
        WiFi.mode(WIFI_AP);
        WiFi.softAP(_ssid);
      }

      delay(500);
      if (DEBUG_LOGGING)
      {
        Serial.printf("SoftAP active. AP IP: %s\n", WiFi.softAPIP().toString().c_str());
      }
    }
    return true;
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
    Serial.println("js_clear called");

    self->_strip.clear();
    // self->_strip.show();   // optional: update LEDs immediately
  }
  static void js_show(CScriptVar *var, void *userdata)
  {
    NeopixelCommander *self = static_cast<NeopixelCommander *>(userdata);
    if (!self)
      return;
    Serial.println("js_show called");

    self->_strip.show();
    // self->_strip.show();   // optional: update LEDs immediately
  }
  static void js_delay(CScriptVar *var, void *userdata)
  {
    CScriptVar *value = var->getParameter("value");
    if (!value)
      return;
    Serial.println("js_delay called");
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
    Serial.printf("js_setPixelColor called: index=%d, r=%d, g=%d, b=%d\n", index->getInt(), r->getInt(), g->getInt(), b->getInt());
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
    Serial.printf("Free heap: %u\n", ESP.getFreeHeap());

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
      if (_storedJsCode.length() > 0)
      {
        _js->execute(_storedJsCode.c_str());
      }
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

  void setSetupAnimation(std::function<void()> animation)
  {
    setupAnimation = animation;
  }

private:
  Preferences _preferences;
  static const uint16_t QUEUE_SIZE = 512;

  const char *_ssid;
  const char *_password;
  uint8_t _pin;
  uint16_t _numPixels;
  uint8_t _brightness;

  AsyncWebServer _server;
  AsyncWebSocket _ws;
  Adafruit_NeoPixel _strip;
  CTinyJS *_js;
  std::function<void()> setupAnimation;

  uint32_t _connectTimeoutMs;

  String _storedJsCode;
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
      Serial.println(_storedJsCode.c_str());
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

        StaticJsonDocument<512> doc;
        if (deserializeJson(doc, (char *)data) == DeserializationError::Ok)
        {
          const char *cmd = doc["cmd"] | "";
          uint32_t id = doc["id"] | 0; // Get command ID from client

          // Handle JSON ping command
          if (strcmp(cmd, "ping") == 0)
          {
            if (DEBUG_LOGGING)
              Serial.printf("Received WebSocket ping (JSON) from client #%u\n", client->id());
            client->text("{\"status\":\"ok\",\"message\":\"pong\"}");
            return;
          }

          // Handle getPixelCount command
          if (strcmp(cmd, "getPixelCount") == 0)
          {
            if (DEBUG_LOGGING)
              Serial.printf("Received getPixelCount from client #%u\n", client->id());
            char response[64];
            snprintf(response, sizeof(response), "{\"status\":\"ok\",\"pixelCount\":%u}", _numPixels);
            client->text(response);
            return;
          }

          // Handle evalJS command - NEW!
          if (strcmp(cmd, "evalJS") == 0)
          {
            const char *jsCode = doc["code"] | "";
            if (strlen(jsCode) == 0)
            {
              client->text("{\"status\":\"error\",\"error\":\"missing_code\"}");
              return;
            }

            if (DEBUG_LOGGING)
              Serial.printf("Received evalJS from client #%u, code length: %u\n", client->id(), strlen(jsCode));

            // bool success = evalJS(String(jsCode));
            // if (success)
            // {
            //   client->text("{\"status\":\"ok\"}");
            // }
            // else
            // {
            //   client->text("{\"status\":\"error\",\"error\":\"eval_failed\"}");
            // }
            return;
          }

          if (strcmp(cmd, "saveJS") == 0)
          {
            const char *jsCode = doc["code"] | "";
            if (strlen(jsCode) == 0)
            {
              client->text("{\"status\":\"error\",\"error\":\"missing_code\"}");
              return;
            }

            if (DEBUG_LOGGING)
            {
              Serial.printf("Received saveJS from client #%u, code length: %u\n", client->id(), strlen(jsCode));
            }

            bool success = this->saveJsCodeToPreferences(jsCode);
            if (success)
            {
              char ackMsg[64];
              snprintf(ackMsg, sizeof(ackMsg), "{\"status\":\"ok\",\"ack\":%u}", id);
              Serial.printf("ackMsg: %s\n", ackMsg);
              client->text(ackMsg);
            }
            else
            {
              client->text("{\"status\":\"error\",\"error\":\"save_failed\"}");
            }
            return;
          }

          if (id == 0)
          {
            // Command ID is required for non-ping commands
            client->text("{\"status\":\"error\",\"error\":\"missing_id\"}");
            return;
          }

          Command command;
          command.clientId = client->id();
          command.commandId = id;
          bool validCmd = true;

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
          else if (strcmp(cmd, "setPixelColor") == 0)
          {
            command.type = SET_PIXEL_COLOR;
            command.index = doc["index"] | 0;
            command.r = doc["r"] | 0;
            command.g = doc["g"] | 0;
            command.b = doc["b"] | 0;

            // Validate pixel index bounds
            if (command.index >= _numPixels)
            {
              char errMsg[96];
              snprintf(errMsg, sizeof(errMsg), "{\"status\":\"error\",\"error\":\"index_out_of_bounds\",\"id\":%u,\"max\":%u}", id, _numPixels - 1);
              client->text(errMsg);
              validCmd = false;
            }
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
          else
          {
            validCmd = false;
            char errMsg[80];
            snprintf(errMsg, sizeof(errMsg), "{\"status\":\"error\",\"error\":\"unknown_cmd\",\"id\":%u}", id);
            client->text(errMsg);
          }

          if (validCmd)
          {
            if (!enqueueCommand(command))
            {
              char errMsg[80];
              snprintf(errMsg, sizeof(errMsg), "{\"status\":\"error\",\"error\":\"queue_full\",\"id\":%u}", id);
              client->text(errMsg);
            }
            // ACK with ID will be sent after processing in loop()
          }
        }
        else
        {
          client->text("{\"status\":\"error\",\"error\":\"bad_json\"}");
        }
      }
    }
  }
};