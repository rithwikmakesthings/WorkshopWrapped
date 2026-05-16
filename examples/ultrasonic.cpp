// HC-SR04 ultrasonic distance sensor — no library needed.
//
// Wiring: VCC→5V, GND→GND, TRIG→GPIO 5, ECHO→GPIO 18 via a voltage
// divider (1 kΩ + 2 kΩ) — the Echo pin is 5 V and ESP32 GPIOs are 3.3 V.

// ── file scope ─────────────────────────────────────────────────────────
static constexpr int TRIG_PIN = 5;
static constexpr int ECHO_PIN = 18;

static float readDistanceCm() {
    digitalWrite(TRIG_PIN, LOW);  delayMicroseconds(2);
    digitalWrite(TRIG_PIN, HIGH); delayMicroseconds(10);
    digitalWrite(TRIG_PIN, LOW);
    long us = pulseIn(ECHO_PIN, HIGH, 30000);   // 30 ms ≈ 5 m timeout
    if (us == 0) return -1.0f;                  // out of range
    return us / 58.0f;                          // µs → cm
}

// ── inside setup() ─────────────────────────────────────────────────────
pinMode(TRIG_PIN, OUTPUT);
pinMode(ECHO_PIN, INPUT);

// ── inside registerHandlers() ──────────────────────────────────────────
Bridge.on("distance", [](JsonVariantConst args, JsonVariant resp) {
    int samples = args["samples"] | 5;
    float sum = 0;
    int valid = 0;
    for (int i = 0; i < samples; i++) {
        float d = readDistanceCm();
        if (d > 0) { sum += d; valid++; }
        delay(30);
    }
    if (valid == 0) { resp["ok"] = false; resp["error"] = "no echo"; return; }
    resp["cm"]    = sum / valid;
    resp["valid"] = valid;
});
