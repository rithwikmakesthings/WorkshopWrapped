#include "Bridge.h"

#include <WiFi.h>
#include <WebServer.h>
#include <uri/UriBraces.h>
#include <ESPmDNS.h>
#include <ArduinoOTA.h>

#ifdef BRIDGE_USE_SECRETS
#include "secrets.h"
#endif

BridgeClass Bridge;

namespace {

WebServer server(BRIDGE_PORT);
std::map<String, BridgeHandler> handlers;
unsigned long startMs = 0;

constexpr size_t kLogLines = 32;
String logBuf[kLogLines];
size_t logHead = 0;
size_t logCount = 0;

String chipId() {
    uint64_t mac = ESP.getEfuseMac();
    char buf[13];
    snprintf(buf, sizeof(buf), "%012llx", mac);
    return String(buf);
}

void sendJson(int code, JsonDocument& doc) {
    String out;
    serializeJsonPretty(doc, out);
    server.sendHeader("Access-Control-Allow-Origin", "*");
    server.send(code, "application/json", out);
}

void handleInfo() {
    JsonDocument doc;
    doc["device"]    = BRIDGE_HOSTNAME;
    doc["chip_id"]   = chipId();
    doc["ip"]        = WiFi.localIP().toString();
    doc["rssi"]      = WiFi.RSSI();
    doc["uptime_ms"] = millis() - startMs;
    doc["free_heap"] = ESP.getFreeHeap();
    JsonArray cmds = doc["commands"].to<JsonArray>();
    for (auto& kv : handlers) cmds.add(kv.first);
    sendJson(200, doc);
}

void handleLogs() {
    JsonDocument doc;
    JsonArray arr = doc["lines"].to<JsonArray>();
    size_t start = (logCount < kLogLines) ? 0 : logHead;
    size_t n = (logCount < kLogLines) ? logCount : kLogLines;
    for (size_t i = 0; i < n; i++) {
        arr.add(logBuf[(start + i) % kLogLines]);
    }
    sendJson(200, doc);
}

void handleCmdPost() {
    JsonDocument req;
    DeserializationError err = deserializeJson(req, server.arg("plain"));
    if (err) {
        JsonDocument resp;
        resp["ok"] = false;
        resp["error"] = String("invalid json: ") + err.c_str();
        sendJson(400, resp);
        return;
    }
    String name = req["cmd"] | "";
    if (name.length() == 0) {
        JsonDocument resp;
        resp["ok"] = false;
        resp["error"] = "missing 'cmd' field";
        sendJson(400, resp);
        return;
    }
    auto it = handlers.find(name);
    if (it == handlers.end()) {
        JsonDocument resp;
        resp["ok"] = false;
        resp["error"] = "unknown command";
        resp["cmd"]   = name;
        JsonArray av  = resp["available"].to<JsonArray>();
        for (auto& kv : handlers) av.add(kv.first);
        sendJson(404, resp);
        return;
    }
    JsonDocument resp;
    JsonVariant respVar = resp.to<JsonVariant>();
    respVar["ok"] = true;
    JsonVariantConst args = req["args"];
    unsigned long t0 = millis();
    it->second(args, respVar);
    respVar["took_ms"] = millis() - t0;
    int code = (respVar["ok"] | true) ? 200 : 500;
    sendJson(code, resp);
}

// GET /cmd/<name>?key=value  — handy for browser/curl one-liners.
void handleCmdGet() {
    String path = server.uri();
    int slash = path.indexOf('/', 1);
    String name = (slash >= 0) ? path.substring(slash + 1) : "";
    auto it = handlers.find(name);
    if (it == handlers.end()) {
        JsonDocument resp;
        resp["ok"] = false;
        resp["error"] = "unknown command";
        resp["cmd"] = name;
        sendJson(404, resp);
        return;
    }
    JsonDocument argsDoc;
    argsDoc.to<JsonObject>();
    for (int i = 0; i < server.args(); i++) {
        const String& k = server.argName(i);
        const String& v = server.arg(i);
        // try to interpret numbers/booleans
        if (v == "true") argsDoc[k] = true;
        else if (v == "false") argsDoc[k] = false;
        else {
            char* end = nullptr;
            long iv = strtol(v.c_str(), &end, 10);
            if (end && *end == '\0') argsDoc[k] = iv;
            else {
                float fv = strtof(v.c_str(), &end);
                if (end && *end == '\0') argsDoc[k] = fv;
                else argsDoc[k] = v;
            }
        }
    }
    JsonDocument resp;
    JsonVariant respVar = resp.to<JsonVariant>();
    respVar["ok"] = true;
    unsigned long t0 = millis();
    it->second(argsDoc.as<JsonVariantConst>(), respVar);
    respVar["took_ms"] = millis() - t0;
    int code = (respVar["ok"] | true) ? 200 : 500;
    sendJson(code, resp);
}

void handleNotFound() {
    JsonDocument doc;
    doc["ok"] = false;
    doc["error"] = "not found";
    doc["path"]  = server.uri();
    sendJson(404, doc);
}

void registerBuiltins() {
    handlers["ping"] = [](JsonVariantConst, JsonVariant resp) {
        resp["pong"] = millis();
    };
    handlers["restart"] = [](JsonVariantConst, JsonVariant resp) {
        resp["restarting"] = true;
        delay(100);
        ESP.restart();
    };
}

} // namespace

bool BridgeClass::begin(const char* ssid, const char* password) {
    if (!Serial) {
        Serial.begin(115200);
    }
    delay(50);
    startMs = millis();

    const char* useSsid = ssid;
    const char* usePw   = password;
#ifdef WIFI_SSID
    if (!useSsid) useSsid = WIFI_SSID;
#endif
#ifdef WIFI_PASSWORD
    if (!usePw) usePw = WIFI_PASSWORD;
#endif

    if (!useSsid || !*useSsid) {
        Serial.println("[bridge] no WiFi SSID configured. Copy src/secrets.h.example to src/secrets.h.");
        return false;
    }

    log(String("connecting to wifi: ") + useSsid);
    WiFi.mode(WIFI_STA);
    WiFi.setHostname(BRIDGE_HOSTNAME);
    WiFi.begin(useSsid, usePw);

    unsigned long t0 = millis();
    while (WiFi.status() != WL_CONNECTED && millis() - t0 < _wifiTimeoutMs) {
        delay(250);
        Serial.print(".");
    }
    Serial.println();

    if (WiFi.status() != WL_CONNECTED) {
        log("wifi connect failed");
        return false;
    }

    log(String("connected: http://") + WiFi.localIP().toString() + ":" + BRIDGE_PORT);

    if (MDNS.begin(BRIDGE_HOSTNAME)) {
        MDNS.addService("http", "tcp", BRIDGE_PORT);
        MDNS.addServiceTxt("http", "tcp", "device", BRIDGE_HOSTNAME);
        MDNS.addServiceTxt("http", "tcp", "chip", chipId().c_str());
        log(String("mdns: http://") + BRIDGE_HOSTNAME + ".local:" + BRIDGE_PORT);
    }

    registerBuiltins();

    server.on("/",     HTTP_GET,  handleInfo);
    server.on("/info", HTTP_GET,  handleInfo);
    server.on("/logs", HTTP_GET,  handleLogs);
    server.on("/cmd",  HTTP_POST, handleCmdPost);
    server.on(UriBraces("/cmd/{}"), HTTP_GET, handleCmdGet);
    server.onNotFound(handleNotFound);
    server.enableCORS(true);
    server.begin();

    ArduinoOTA.setHostname(BRIDGE_HOSTNAME);
    ArduinoOTA.onStart([]() { Bridge.log("OTA start"); });
    ArduinoOTA.onEnd  ([]() { Bridge.log("OTA done");  });
    ArduinoOTA.begin();

    log("bridge ready");
    return true;
}

void BridgeClass::loop() {
    server.handleClient();
    ArduinoOTA.handle();
}

void BridgeClass::on(const String& name, BridgeHandler handler) {
    handlers[name] = handler;
}

bool BridgeClass::isConnected() const { return WiFi.status() == WL_CONNECTED; }
String BridgeClass::ip() const        { return WiFi.localIP().toString(); }
String BridgeClass::hostname() const  { return String(BRIDGE_HOSTNAME) + ".local"; }

void BridgeClass::log(const String& line) {
    Serial.print("[bridge] ");
    Serial.println(line);
    logBuf[logHead] = line;
    logHead = (logHead + 1) % kLogLines;
    if (logCount < kLogLines) logCount++;
}
