// DHT22 (or DHT11) temperature and humidity.
//
//   platformio.ini → lib_deps:
//     adafruit/DHT sensor library @ ^1.4.6
//     adafruit/Adafruit Unified Sensor @ ^1.1.14
//
// Wiring: VCC→3V3 (or 5V on DHT22), GND→GND, DATA→GPIO 4 with a 10 kΩ
// pull-up between DATA and VCC.

#include <DHT.h>

// ── file scope ─────────────────────────────────────────────────────────
static constexpr int DHT_PIN  = 4;
static constexpr int DHT_TYPE = DHT22;   // or DHT11
static DHT dht(DHT_PIN, DHT_TYPE);

// ── inside setup() ─────────────────────────────────────────────────────
dht.begin();

// ── inside registerHandlers() ──────────────────────────────────────────
Bridge.on("env", [](JsonVariantConst, JsonVariant resp) {
    float t = dht.readTemperature();
    float h = dht.readHumidity();
    if (isnan(t) || isnan(h)) {
        resp["ok"] = false;
        resp["error"] = "sensor read failed";
        return;
    }
    resp["temp_c"]    = t;
    resp["humidity"]  = h;
    resp["heat_idx"]  = dht.computeHeatIndex(t, h, false);
});
