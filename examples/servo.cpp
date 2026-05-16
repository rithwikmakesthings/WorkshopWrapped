// Control a servo via the ESP32Servo library.
//
//   platformio.ini → lib_deps:
//     madhephaestus/ESP32Servo @ ^3.0.5
//
// IMPORTANT: a servo can draw >500 mA. Power it from an external 5 V supply
// and share GND with the ESP32. Don't run it off the 3V3 rail.

#include <ESP32Servo.h>

// ── file scope ─────────────────────────────────────────────────────────
static Servo servo;
static constexpr int SERVO_PIN = 13;

// ── inside setup() ─────────────────────────────────────────────────────
servo.setPeriodHertz(50);
servo.attach(SERVO_PIN, 500, 2400);   // pulse widths in µs — tune per servo

// ── inside registerHandlers() ──────────────────────────────────────────
Bridge.on("servo", [](JsonVariantConst args, JsonVariant resp) {
    int angle = args["angle"] | 90;
    angle = constrain(angle, 0, 180);
    servo.write(angle);
    resp["angle"] = angle;
});

Bridge.on("sweep", [](JsonVariantConst args, JsonVariant resp) {
    int from   = args["from"]   | 0;
    int to     = args["to"]     | 180;
    int stepMs = args["step_ms"] | 10;
    int step   = (to >= from) ? 1 : -1;
    for (int a = from; a != to; a += step) {
        servo.write(a);
        delay(stepMs);
    }
    servo.write(to);
    resp["from"] = from;
    resp["to"]   = to;
});
