// o1 Bridge — ESP32 Starter
//
// This is the only file most builds need. Keep `Bridge.begin()` and
// `Bridge.loop()` in place; add your own `Bridge.on(...)` handlers in
// `registerHandlers()` below, then build with `pio run -t upload`.
//
// Need ideas? Check examples/ for ready-to-paste handler patterns.

#include "Bridge.h"

#ifndef BOARD_LED_PIN
#define BOARD_LED_PIN 2
#endif

static void registerHandlers() {
    pinMode(BOARD_LED_PIN, OUTPUT);

    // Toggle or set the onboard LED.
    //   curl -X POST http://<ip>/cmd -d '{"cmd":"led","args":{"on":true}}'
    Bridge.on("led", [](JsonVariantConst args, JsonVariant resp) {
        bool on = args["on"] | false;
        digitalWrite(BOARD_LED_PIN, on ? HIGH : LOW);
        resp["led"] = on;
    });

    // Blink the onboard LED a few times.
    //   curl http://<ip>/cmd/blink?times=5
    Bridge.on("blink", [](JsonVariantConst args, JsonVariant resp) {
        int times    = args["times"]    | 3;
        int delayMs  = args["delay_ms"] | 200;
        for (int i = 0; i < times; i++) {
            digitalWrite(BOARD_LED_PIN, HIGH); delay(delayMs);
            digitalWrite(BOARD_LED_PIN, LOW);  delay(delayMs);
        }
        resp["blinked"] = times;
    });

    // Generic GPIO reader — handy for poking at sensors without a custom handler.
    //   curl 'http://<ip>/cmd/read_pin?pin=34&mode=analog'
    Bridge.on("read_pin", [](JsonVariantConst args, JsonVariant resp) {
        int pin = args["pin"] | -1;
        if (pin < 0) {
            resp["ok"]    = false;
            resp["error"] = "missing 'pin'";
            return;
        }
        String mode = args["mode"] | "digital";
        pinMode(pin, INPUT);
        if (mode == "analog") {
            resp["value"] = analogRead(pin);
        } else {
            resp["value"] = digitalRead(pin);
        }
        resp["pin"]  = pin;
        resp["mode"] = mode;
    });

    // ─── add your own handlers below ───
    //
    // Bridge.on("my_thing", [](JsonVariantConst args, JsonVariant resp) {
    //     // read args, do something with hardware, fill resp
    // });
}

void setup() {
    if (!Bridge.begin()) {
        // No WiFi? Keep looping so OTA / serial can still recover the board.
        Serial.println("[main] Bridge.begin() returned false; check src/secrets.h");
    }
    registerHandlers();
}

void loop() {
    Bridge.loop();
}
