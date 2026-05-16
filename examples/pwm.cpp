// PWM output — LED dimming, motor speed via a MOSFET / driver, etc.
//
// Uses the Arduino-ESP32 v3 LEDC wrapper: analogWrite-like behaviour with
// 8-bit duty cycle (0–255). For finer control or different frequencies,
// use the ledc* API directly.

// ── file scope ─────────────────────────────────────────────────────────
static constexpr int PWM_PIN  = 25;
static constexpr int PWM_FREQ = 5000;     // Hz; raise for motors, lower for LEDs flicker-free
static constexpr int PWM_RES  = 8;        // 8-bit → 0..255

// ── inside setup() ─────────────────────────────────────────────────────
ledcAttach(PWM_PIN, PWM_FREQ, PWM_RES);

// ── inside registerHandlers() ──────────────────────────────────────────
Bridge.on("pwm", [](JsonVariantConst args, JsonVariant resp) {
    int duty = args["duty"] | 0;
    duty = constrain(duty, 0, 255);
    ledcWrite(PWM_PIN, duty);
    resp["duty"] = duty;
});

Bridge.on("fade", [](JsonVariantConst args, JsonVariant resp) {
    int target = constrain((int)(args["to"] | 255), 0, 255);
    int ms     = args["ms"] | 1000;
    int start = 0;
    unsigned long t0 = millis();
    while (millis() - t0 < (unsigned long)ms) {
        float k = (float)(millis() - t0) / ms;
        ledcWrite(PWM_PIN, (int)(start + (target - start) * k));
        delay(10);
    }
    ledcWrite(PWM_PIN, target);
    resp["duty"] = target;
});
