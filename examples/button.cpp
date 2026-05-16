// Count button presses. Wire one leg of the button to GPIO 4 and the other
// to GND. We use INPUT_PULLUP so an open button reads HIGH and a press
// reads LOW.
//
// Paste into registerHandlers() in src/main.cpp. Also add the static state
// at file scope and the debounce check inside loop().

// ── file scope (above setup()) ─────────────────────────────────────────
static constexpr int BUTTON_PIN = 4;
static volatile uint32_t buttonCount = 0;
static unsigned long lastBounceMs = 0;

// ── inside setup() ─────────────────────────────────────────────────────
pinMode(BUTTON_PIN, INPUT_PULLUP);

// ── inside loop(), after Bridge.loop() ─────────────────────────────────
static int lastState = HIGH;
int state = digitalRead(BUTTON_PIN);
if (state == LOW && lastState == HIGH && millis() - lastBounceMs > 50) {
    buttonCount++;
    lastBounceMs = millis();
}
lastState = state;

// ── inside registerHandlers() ──────────────────────────────────────────
Bridge.on("count", [](JsonVariantConst, JsonVariant resp) {
    resp["count"] = buttonCount;
});

Bridge.on("reset_count", [](JsonVariantConst, JsonVariant resp) {
    buttonCount = 0;
    resp["count"] = 0;
});
