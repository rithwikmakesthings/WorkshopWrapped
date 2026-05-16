#pragma once
//
// o1 Bridge — minimal WiFi command server for ESP32.
//
// Workflow:
//   Bridge.begin();                       // connects WiFi, starts HTTP server
//   Bridge.on("name", [](args, resp){});  // register a JSON command
//   Bridge.loop();                        // call from loop()
//
// All commands are reached via:
//   POST http://<ip>/cmd   body: {"cmd": "name", "args": {...}}
// or GET http://<ip>/info to list registered commands.
//
#include <Arduino.h>
#include <ArduinoJson.h>
#include <functional>
#include <map>

#ifndef BRIDGE_PORT
#define BRIDGE_PORT 80
#endif

#ifndef BRIDGE_HOSTNAME
#define BRIDGE_HOSTNAME "o1bridge"
#endif

using BridgeHandler = std::function<void(JsonVariantConst args, JsonVariant response)>;

class BridgeClass {
public:
    // Connect to WiFi and start the HTTP server. If ssid/password are null,
    // values from secrets.h (WIFI_SSID / WIFI_PASSWORD) are used.
    bool begin(const char* ssid = nullptr, const char* password = nullptr);

    // Service the HTTP server and OTA. Call from loop().
    void loop();

    // Register a command. Use args["key"] | default to read arguments.
    void on(const String& name, BridgeHandler handler);

    // Connection helpers.
    bool isConnected() const;
    String ip() const;
    String hostname() const;

    // Built-in logger that mirrors to Serial and the optional /log buffer.
    void log(const String& line);

    // Override the default 20s WiFi join timeout (ms). Call before begin().
    void setWifiTimeout(unsigned long ms) { _wifiTimeoutMs = ms; }

private:
    unsigned long _wifiTimeoutMs = 20000;
};

extern BridgeClass Bridge;
