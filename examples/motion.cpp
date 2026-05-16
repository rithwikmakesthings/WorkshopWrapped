// PIR motion sensor with optional outbound webhook on detection.
//
// Wiring: most PIR modules (HC-SR501) take 5 V power; OUT pin is 3.3 V
// logic level (or close enough) — connect to GPIO 14.
//
// Optional: provide a webhook URL via the "watch" command and the device
// will POST {"event":"motion"} when the PIR fires. Lets you wire it up to
// a phone notification (e.g. ntfy.sh) without writing any laptop code.

#include <HTTPClient.h>

// ── file scope ─────────────────────────────────────────────────────────
static constexpr int PIR_PIN = 14;
static String     webhookUrl   = "";
static uint32_t   motionEvents = 0;
static unsigned long lastEventMs = 0;
static int        lastPirState   = LOW;

// ── inside setup() ─────────────────────────────────────────────────────
pinMode(PIR_PIN, INPUT);

// ── inside loop(), after Bridge.loop() ─────────────────────────────────
{
    int s = digitalRead(PIR_PIN);
    if (s == HIGH && lastPirState == LOW && millis() - lastEventMs > 1000) {
        motionEvents++;
        lastEventMs = millis();
        Bridge.log("motion detected");
        if (webhookUrl.length() > 0) {
            HTTPClient http;
            http.begin(webhookUrl);
            http.addHeader("Content-Type", "application/json");
            http.POST("{\"event\":\"motion\"}");
            http.end();
        }
    }
    lastPirState = s;
}

// ── inside registerHandlers() ──────────────────────────────────────────
Bridge.on("motion_status", [](JsonVariantConst, JsonVariant resp) {
    resp["events"]     = motionEvents;
    resp["last_ms"]    = lastEventMs ? (millis() - lastEventMs) : -1;
    resp["state_high"] = digitalRead(PIR_PIN) == HIGH;
});

Bridge.on("watch", [](JsonVariantConst args, JsonVariant resp) {
    webhookUrl = String((const char*)(args["url"] | ""));
    resp["watching"] = webhookUrl;
});
