#!/usr/bin/env python3
"""
bridge.py — talk to an o1 Bridge ESP32 device from the command line.

Usage:
    bridge.py info <host>
    bridge.py logs <host>
    bridge.py <host> <command> [key=value ...]

Examples:
    bridge.py info o1bridge.local
    bridge.py 192.168.1.42 blink times=5 delay_ms=100
    bridge.py o1bridge.local read_pin pin=34 mode=analog

`host` can be an IP or `<hostname>.local` (via mDNS). Values are parsed as
int/float/bool/string in that order.
"""
from __future__ import annotations

import json
import sys
import urllib.request
import urllib.error


def _parse_value(v: str):
    low = v.lower()
    if low == "true":
        return True
    if low == "false":
        return False
    try:
        return int(v)
    except ValueError:
        pass
    try:
        return float(v)
    except ValueError:
        pass
    return v


def _normalise_host(host: str) -> str:
    if host.startswith("http://") or host.startswith("https://"):
        return host.rstrip("/")
    return f"http://{host}"


def _get(url: str) -> dict:
    with urllib.request.urlopen(url, timeout=10) as resp:
        return json.loads(resp.read().decode())


def _post(url: str, payload: dict) -> dict:
    req = urllib.request.Request(
        url,
        data=json.dumps(payload).encode(),
        headers={"Content-Type": "application/json"},
        method="POST",
    )
    with urllib.request.urlopen(req, timeout=30) as resp:
        return json.loads(resp.read().decode())


def cmd_info(host: str) -> int:
    base = _normalise_host(host)
    print(json.dumps(_get(f"{base}/info"), indent=2))
    return 0


def cmd_logs(host: str) -> int:
    base = _normalise_host(host)
    data = _get(f"{base}/logs")
    for line in data.get("lines", []):
        print(line)
    return 0


def cmd_send(host: str, name: str, args_kv: list[str]) -> int:
    base = _normalise_host(host)
    args = {}
    for kv in args_kv:
        if "=" not in kv:
            print(f"argument '{kv}' must be key=value", file=sys.stderr)
            return 2
        k, v = kv.split("=", 1)
        args[k] = _parse_value(v)
    payload = {"cmd": name, "args": args}
    try:
        resp = _post(f"{base}/cmd", payload)
    except urllib.error.HTTPError as e:
        body = e.read().decode()
        try:
            body = json.dumps(json.loads(body), indent=2)
        except Exception:
            pass
        print(f"HTTP {e.code}\n{body}", file=sys.stderr)
        return 1
    print(json.dumps(resp, indent=2))
    return 0


def main(argv: list[str]) -> int:
    if len(argv) < 2:
        print(__doc__)
        return 1
    if argv[1] in {"-h", "--help"}:
        print(__doc__)
        return 0
    if argv[1] == "info" and len(argv) >= 3:
        return cmd_info(argv[2])
    if argv[1] == "logs" and len(argv) >= 3:
        return cmd_logs(argv[2])
    if len(argv) >= 3:
        return cmd_send(argv[1], argv[2], argv[3:])
    print(__doc__)
    return 1


if __name__ == "__main__":
    sys.exit(main(sys.argv))
