// WS2812 / NeoPixel strip control.
//
//   platformio.ini → lib_deps:
//     adafruit/Adafruit NeoPixel @ ^1.12.3
//
// Wiring: DIN → GPIO 27 (any GPIO works). Power: most strips want 5 V
// directly from your supply, NOT through the ESP32. The strip's logic is
// 5 V — a 74AHCT125 level shifter on the data line gives reliable results.
// Common ground between strip supply and ESP32.

#include <Adafruit_NeoPixel.h>

// ── file scope ─────────────────────────────────────────────────────────
static constexpr int NEOPIXEL_PIN = 27;
static constexpr int NUM_PIXELS   = 16;
static Adafruit_NeoPixel pixels(NUM_PIXELS, NEOPIXEL_PIN, NEO_GRB + NEO_KHZ800);

// ── inside setup() ─────────────────────────────────────────────────────
pixels.begin();
pixels.setBrightness(64);   // 0-255; keep modest if powering through ESP32 USB
pixels.show();

// ── inside registerHandlers() ──────────────────────────────────────────
Bridge.on("color", [](JsonVariantConst args, JsonVariant resp) {
    uint8_t r = args["r"] | 0;
    uint8_t g = args["g"] | 0;
    uint8_t b = args["b"] | 0;
    int idx   = args["i"] | -1;   // -1 = all pixels
    if (idx < 0) {
        for (int i = 0; i < NUM_PIXELS; i++) pixels.setPixelColor(i, r, g, b);
    } else if (idx < NUM_PIXELS) {
        pixels.setPixelColor(idx, r, g, b);
    } else {
        resp["ok"] = false;
        resp["error"] = "index out of range";
        return;
    }
    pixels.show();
    resp["r"] = r; resp["g"] = g; resp["b"] = b;
});

Bridge.on("clear", [](JsonVariantConst, JsonVariant resp) {
    pixels.clear();
    pixels.show();
    resp["cleared"] = NUM_PIXELS;
});
