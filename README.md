# o1 Bridge — ESP32 Starter

A from-zero scaffold for the **o1 Bridge** competition (o1lab × Arrayah,
May 17 – May 24 2026). Flash it onto an ESP32, give it your WiFi password,
and you have a device that:

- Joins your WiFi and announces itself as `o1bridge.local`.
- Exposes a JSON HTTP API on port 80 — `POST /cmd` to run things, `GET /info`
  to see what it can do.
- Accepts over-the-air firmware updates so you stop reaching for the USB
  cable after the first flash.
- Comes pre-wired with a small command framework — register a handler
  (`Bridge.on("blink", ...)`) and call it from your laptop, browser, phone,
  or whatever you want.

You spend the week building the *interesting* parts. The plumbing is done.

> 🤖 **Working with an AI agent (Claude Code, Cursor, etc.)?** Point it at
> [`AGENTS.md`](./AGENTS.md). It has everything an agent needs to help you
> build on this scaffold.

---

## Quickstart

### 1. Install PlatformIO Core (one time)
```bash
# macOS / Linux (recommended)
pip install -U platformio
# or via Homebrew
brew install platformio
# or via VS Code: install the "PlatformIO IDE" extension
```

### 2. Set your WiFi credentials
```bash
cp src/secrets.h.example src/secrets.h
# edit src/secrets.h — must be a 2.4 GHz network
```

### 3. Plug in your ESP32 over USB and flash it
```bash
pio run -t upload
pio device monitor      # watch it boot
```
You should see something like:
```
[bridge] connecting to wifi: my-network
[bridge] connected: http://192.168.1.42:80
[bridge] mdns: http://o1bridge.local:80
[bridge] bridge ready
```

### 4. Talk to it
```bash
# What can it do?
python tools/bridge.py info o1bridge.local

# Make the onboard LED blink five times
python tools/bridge.py o1bridge.local blink times=5

# Or from any HTTP client:
curl http://o1bridge.local/cmd/blink?times=5
```

---

## Where you actually write code

Everything you'll edit lives in **`src/main.cpp`**, inside `registerHandlers()`:

```cpp
Bridge.on("hello", [](JsonVariantConst args, JsonVariant resp) {
    String who = args["name"] | "world";
    resp["greeting"] = "hello, " + who;
});
```

Build, upload, call it:
```bash
pio run -t upload
python tools/bridge.py o1bridge.local hello name=Sydney
# → { "ok": true, "greeting": "hello, Sydney", ... }
```

That's the whole loop: **edit handler → upload → call from CLI**.

See [`examples/`](./examples/) for paste-ready handlers covering buttons,
servos, ultrasonic distance, accelerometers, NeoPixels, DHT temp sensors,
and more.

---

## Which board do you have?

The `platformio.ini` ships with three environments. Pick the one matching
the board you grabbed from the lab:

| Board | Command |
|-------|---------|
| ESP32 / WROOM-32 (classic, silver shield) | `pio run -e esp32dev -t upload` |
| ESP32-S3 (USB-C, often dual-USB) | `pio run -e esp32s3 -t upload` |
| ESP32-C3 (tiny, RISC-V) | `pio run -e esp32c3 -t upload` |

If you're not sure, `esp32dev` is the safe default and works on most boards.

---

## Project layout

```
esp32-starter/
├── AGENTS.md          ← agent playbook (point Claude / Cursor at this)
├── README.md          ← you are here
├── platformio.ini     ← build config, board selection, libraries
├── src/
│   ├── main.cpp       ← your handlers go here
│   ├── Bridge.h       ← framework header — don't edit unless fixing a bug
│   ├── Bridge.cpp     ← framework impl
│   └── secrets.h      ← WiFi creds (gitignored — copy from .example)
├── tools/
│   ├── bridge.py      ← CLI: bridge.py <host> <cmd> [k=v ...]
│   ├── discover.py    ← finds o1bridge devices on the LAN via mDNS
│   └── requirements.txt
└── examples/          ← copy-paste handlers for common sensors/actuators
```

---

## Useful one-liners

| Goal | Command |
|------|---------|
| Build | `pio run` |
| Upload over USB | `pio run -t upload` |
| Upload over WiFi (OTA, after first flash) | `pio run -t upload --upload-port o1bridge.local` |
| Watch serial output | `pio device monitor` |
| Clean build | `pio run -t clean` |
| List devices on the LAN | `python tools/discover.py` |
| Call a command | `python tools/bridge.py o1bridge.local <cmd> k=v ...` |
| Latest logs from device | `python tools/bridge.py logs o1bridge.local` |
| Erase flash (panic button) | `pio run -t erase` |

---

## Troubleshooting

**"wifi connect failed"** — Make sure you're on a **2.4 GHz** network (ESP32
can't see 5 GHz). Double-check `src/secrets.h`. Phone hotspots usually
work — try one if the lab WiFi is iffy.

**"Failed to connect to ESP32"** on upload — Hold the `BOOT` button on the
board, tap `EN` (reset), keep holding `BOOT` until upload starts.

**`o1bridge.local` won't resolve** — Some networks (incl. corporate / guest
WiFi) block mDNS. Use the IP printed in the serial monitor instead, or run
`python tools/discover.py`.

**`analogRead()` always returns 4095** — You're on an ADC2 pin (most pins
< 32) which conflicts with WiFi. Use GPIO 32–39 for analog input.

**Random reboots when a sensor is plugged in** — Probably feeding 5 V into
a 3.3 V GPIO, or drawing too much current from the 3V3 rail. Use a level
shifter / external supply.

More gotchas (boot strapping pins, OTA recovery, library suggestions) are
documented for the AI agent in [`AGENTS.md`](./AGENTS.md) — humans can read
it too.

---

## What is o1 Bridge?

A week-long hardware competition by **o1lab** in collaboration with
**Arrayah**. May 17 – May 24, 2026. Two mandatory in-person events:

- **Kickoff & brief**: May 17, 11:00am — challenge revealed, components on
  the table, builds begin.
- **Demos & mixer**: May 24, 3:00pm — show what you built.

Bring an idea. Or don't — show up curious. There's a Bambu Lab P1S 3D
printer for the winner, plus hardware prizes across other categories.
Components, tools, and people to build alongside are all in the lab during
the week. The full kit list lives at [o1lab.xyz](https://o1lab.xyz).

Now go make something real.
