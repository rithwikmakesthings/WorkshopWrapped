# AGENTS.md — o1 Bridge ESP32 Starter

You are pair-programming with someone building a physical device for the
**o1 Bridge** competition (o1lab × Arrayah, May 17 – May 24 2026). The
participant has a week, a real ESP32 dev board, and components from the lab.
Your job is to help them get from idea → working hardware demo, fast.

This document is for you, the AI agent. Treat it as the source of truth for
how this scaffold works and how the participant prefers to collaborate.

---

## What this scaffold gives you

A working ESP32 firmware that does the boring parts so you can spend the week
on the *interesting* parts:

- **WiFi station** with mDNS (`http://o1bridge.local`) and OTA updates.
- **HTTP command server** on port 80 exposing `GET /info`, `GET /logs`, and
  `POST /cmd` / `GET /cmd/<name>`.
- **`Bridge.on("name", handler)` API** — register a JSON command handler in
  a few lines. The framework deserialises args, calls your lambda, serialises
  the response.
- **Python CLI** (`tools/bridge.py`) to call commands from a laptop, plus
  `tools/discover.py` to find devices on the network via mDNS.

The participant's code lives almost entirely in `src/main.cpp` inside
`registerHandlers()`. Don't touch `Bridge.h` / `Bridge.cpp` unless you're
fixing a bug in the framework itself.

---

## Working principles

1. **Get something blinking first, then iterate.** A demo that does one thing
   on day 1 beats a perfect plan on day 7. Encourage incremental wins —
   blink → respond to a button → respond over WiFi → add the sensor → make
   the response intelligent.
2. **The participant may not be technical.** Explain pin numbers, voltages,
   and library choices in one sentence when you make them. Don't dump
   datasheets unless asked.
3. **The hardware is real and unforgiving.** Always check pin numbers,
   voltage levels, and direction (input vs output) *before* uploading. A
   5 V signal on a 3.3 V GPIO can brick the board.
4. **Bias toward the smallest working change.** Don't refactor the Bridge
   framework. Don't add abstractions. Add one handler, test it, move on.
5. **Stay close to Arduino primitives.** `pinMode`, `digitalRead`,
   `digitalWrite`, `analogRead`, `Wire.h`, `Serial.print`. These are
   universally documented and the participant can google any of them.
6. **No new heavy dependencies.** Library additions should go in
   `platformio.ini` under `lib_deps` only when a sensor genuinely needs
   them. Prefer ones from `bblanchon`, `adafruit`, `paulstoffregen`,
   `madhephaestus` — well-maintained, well-known.

---

## The command handler pattern

Every interaction with the device is one of these:

```cpp
Bridge.on("name", [](JsonVariantConst args, JsonVariant resp) {
    // 1. Read inputs from args, with sensible defaults.
    int pin = args["pin"] | 4;
    // 2. Do the hardware thing.
    pinMode(pin, INPUT);
    int v = digitalRead(pin);
    // 3. Write your output to resp.
    resp["pin"]   = pin;
    resp["value"] = v;
    // 4. (Optional) set resp["ok"] = false on error — turns into HTTP 500.
});
```

That's it. The handler runs synchronously on the HTTP request. Keep it
fast — long-running work (>1s) blocks other requests. For anything that
needs to run continuously, do the work in `loop()` and have the handler
read/write shared state.

### Persistent state across handlers

Use file-scope variables in `main.cpp`:

```cpp
static volatile int  counter   = 0;
static unsigned long lastSeenMs = 0;

void loop() {
    Bridge.loop();
    if (digitalRead(BUTTON_PIN) == LOW && millis() - lastSeenMs > 50) {
        counter++;
        lastSeenMs = millis();
    }
}

// In registerHandlers():
Bridge.on("count", [](JsonVariantConst, JsonVariant resp) {
    resp["count"] = counter;
});
```

---

## Workflow you'll run on the participant's behalf

### Build & upload (USB)
```bash
pio run -e esp32dev -t upload          # change env to esp32s3 / esp32c3 as needed
pio device monitor                     # serial logs at 115200
```
First upload may need them to press the BOOT button while it's connecting.
After that the board reboots and you should see something like:
```
[bridge] connected: http://192.168.1.42:80
[bridge] mdns: http://o1bridge.local:80
[bridge] bridge ready
```

### Upload over WiFi (OTA, no USB needed)
After the first USB upload, subsequent uploads can go over WiFi:
```bash
pio run -e esp32dev -t upload --upload-port o1bridge.local
```

### Talk to the device
```bash
# Discover devices on the network
python tools/discover.py

# Inspect device + list registered commands
python tools/bridge.py info o1bridge.local

# Run a command
python tools/bridge.py o1bridge.local blink times=5 delay_ms=80
python tools/bridge.py 192.168.1.42 read_pin pin=34 mode=analog

# Recent logs from the device
python tools/bridge.py logs o1bridge.local
```

From any language, use HTTP directly:
```bash
curl -X POST http://o1bridge.local/cmd \
     -H 'Content-Type: application/json' \
     -d '{"cmd":"led","args":{"on":true}}'
```

---

## Hardware reference (use this before wiring)

### Voltage / power
- ESP32 GPIOs are **3.3 V**. Never feed 5 V directly into a GPIO without a
  level shifter or divider. Some sensors are 5 V-tolerant on data — check
  the datasheet.
- USB provides 5 V. The on-board regulator gives you 3.3 V on the `3V3` pin.
- High-current actuators (servos, motors, big LED strips) need a **separate
  power supply** with a common ground. The 3.3 V rail can usually deliver
  ~500 mA — don't push it.

### Pin gotchas (classic ESP32 / WROOM-32)
- **GPIO 6–11** connected to flash. Never use.
- **GPIO 0, 2, 12, 15** are strapping pins. Avoid pulling them at boot
  unless you understand the rules. Safe for outputs *after* boot, risky as
  inputs if a sensor pulls them low.
- **GPIO 34–39** are **input-only** (no `digitalWrite`, no internal pullup).
- **ADC2 pins (e.g. 0, 2, 4, 12–15, 25–27)** can't be used for `analogRead`
  while WiFi is active. Always use **ADC1 (32–39)** for analog.
- Built-in LED is **GPIO 2** on most generic ESP32 dev boards. (S3 boards
  often use GPIO 48 for a NeoPixel; the platformio.ini sets `BOARD_LED_PIN`
  per env.)

### Typical wiring
- I²C: SDA=21, SCL=22 (most ESP32 boards). Pull-ups usually on the breakout.
- SPI: SCK=18, MISO=19, MOSI=23, CS=5.
- UART2: RX=16, TX=17.

If the participant is on an S3 or C3 board these all change — read the
specific board's pinout, don't guess.

---

## Useful libraries (add only when needed)

Drop these into `platformio.ini → lib_deps` under the relevant env:

| Component | Library |
|-----------|---------|
| DHT11 / DHT22 temp/humidity | `adafruit/DHT sensor library` |
| BMP/BME280 pressure/temp | `adafruit/Adafruit BME280 Library` |
| MPU6050 accelerometer | `electroniccats/MPU6050` |
| HC-SR04 ultrasonic | (write 5 lines inline — no lib needed) |
| Servo | `madhephaestus/ESP32Servo` |
| NeoPixel / WS2812 | `adafruit/Adafruit NeoPixel` |
| OLED SSD1306 | `adafruit/Adafruit SSD1306` |
| MQTT | `knolleary/PubSubClient` |
| BLE | (built into Arduino-ESP32, `#include <BLEDevice.h>`) |

Examples for several of these are in `examples/`.

---

## Common debugging recipes

| Symptom | What to check |
|---------|---------------|
| "wifi connect failed" loop | SSID/password in `src/secrets.h`; ESP32 is 2.4 GHz only, can't see 5 GHz networks |
| Compiles fine, board hangs at boot | A strapping pin (0/2/12/15) wired in a way that fights the bootloader — unplug the sensor and re-upload |
| `analogRead` returns 4095 always | Floating pin, or you're using an ADC2 pin while WiFi is on — switch to GPIO 32–39 |
| Upload fails / "Failed to connect" | Hold BOOT, tap EN/RESET, release EN, then release BOOT just as upload starts |
| OTA upload fails | The board needs to be on the same WiFi as your laptop; OTA only works *after* the first USB upload with this firmware |
| `o1bridge.local` doesn't resolve | mDNS blocked on the network — use the IP from the serial monitor instead, or run `python tools/discover.py` |
| Command times out from CLI | A handler is doing >30 s of work; refactor it to set state and let `loop()` finish the job |

---

## What NOT to do

- **Don't rewrite `Bridge.h` / `Bridge.cpp`.** Add handlers, don't refactor.
- **Don't hard-code WiFi credentials in `main.cpp`.** They go in
  `src/secrets.h` (gitignored) or environment variables.
- **Don't `delay()` for more than a few hundred ms in a handler.** It blocks
  the server. Use millis()-based timing in `loop()` for anything periodic.
- **Don't add a database, MQTT broker, or web framework.** This is a
  microcontroller. The HTTP server is enough.
- **Don't suggest Edge / Cloud / Lambda stuff unprompted.** The challenge is
  about the device itself; the laptop is the cloud.
- **Don't claim a wiring is safe without checking the board's pinout.**
  Pinouts vary between WROOM-32, S3, C3, and clones. Ask which board the
  participant has if it's not obvious.

---

## Quick mental model

```
[ sensor / button / mic ] ──► GPIO ──► handler ──► resp JSON ──► laptop / app
                                  ▲                                    │
                                  └──── handler ◄── /cmd POST ◄────────┘
                                                                       │
[ servo / LED / display / motor ] ◄── GPIO ◄── handler  ◄──────────────┘
```

Inputs come in as GPIO reads inside handlers (or in `loop()` if continuous).
Outputs are GPIO writes triggered by `POST /cmd`. The network side is solved
— focus the participant's attention on what to sense, what to do with it,
and what to do back.

---

## When to escalate to the participant

Stop and ask before:
- Wiring anything that could backfeed power into a GPIO.
- Changing the WiFi SSID / hostname (they may share a router with other
  participants).
- Adding a library that pulls in >1 MB or requires partition table changes.
- Switching frameworks (ESP-IDF instead of Arduino) — that's a one-way door.

For everything else, just ship the smallest change that works.
