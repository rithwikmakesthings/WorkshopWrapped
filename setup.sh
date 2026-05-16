#!/usr/bin/env bash
# o1 Bridge ESP32 Starter — first-time setup
set -euo pipefail

cd "$(dirname "$0")"

echo "── checking python ──"
if ! command -v python3 >/dev/null; then
    echo "python3 is required. Install it first."
    exit 1
fi

echo "── installing platformio core if needed ──"
if ! command -v pio >/dev/null; then
    python3 -m pip install --user -U platformio
    echo "PlatformIO installed. You may need to add ~/.local/bin to your PATH."
fi

echo "── installing tools/ python deps ──"
python3 -m pip install --user -r tools/requirements.txt

if [ ! -f src/secrets.h ]; then
    echo "── creating src/secrets.h ──"
    cp src/secrets.h.example src/secrets.h
    echo
    echo "Edit src/secrets.h with your WiFi credentials, then run:"
    echo "    pio run -t upload"
else
    echo "── src/secrets.h already present ──"
fi

echo
echo "setup complete."
echo "next: pio run -t upload   (plug your ESP32 in over USB first)"
