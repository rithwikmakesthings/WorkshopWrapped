#!/usr/bin/env python3
"""
discover.py — find o1 Bridge devices on the local network via mDNS.

    pip install zeroconf
    python tools/discover.py

Prints each device's hostname, IP, port, and chip id, then exits after a few
seconds. If your network blocks mDNS, watch the device's serial monitor for
the IP instead: `pio device monitor`.
"""
from __future__ import annotations

import socket
import sys
import time

try:
    from zeroconf import ServiceBrowser, ServiceListener, Zeroconf
except ImportError:
    print("zeroconf not installed. Run: pip install zeroconf", file=sys.stderr)
    sys.exit(1)


class Listener(ServiceListener):
    def __init__(self) -> None:
        self.found: list[dict] = []

    def add_service(self, zc: Zeroconf, type_: str, name: str) -> None:  # noqa: N802
        info = zc.get_service_info(type_, name)
        if not info:
            return
        device = (info.properties.get(b"device") or b"").decode(errors="replace")
        chip = (info.properties.get(b"chip") or b"").decode(errors="replace")
        # Filter to o1bridge devices when the TXT record is present, otherwise
        # show every HTTP service so users can still spot their board.
        if device and device != "o1bridge":
            return
        addrs = [socket.inet_ntoa(a) for a in info.addresses]
        entry = {
            "name": name.replace("._http._tcp.local.", ""),
            "ip": addrs[0] if addrs else "?",
            "port": info.port,
            "chip": chip,
        }
        self.found.append(entry)
        print(f"  {entry['name']:<24} http://{entry['ip']}:{entry['port']}  chip={entry['chip']}")

    def remove_service(self, *_args, **_kwargs) -> None: ...
    def update_service(self, *_args, **_kwargs) -> None: ...


def main() -> int:
    print("scanning for o1 Bridge devices (5s)...")
    zc = Zeroconf()
    listener = Listener()
    ServiceBrowser(zc, "_http._tcp.local.", listener)
    try:
        time.sleep(5)
    finally:
        zc.close()
    if not listener.found:
        print("no devices found. Is the board on the same WiFi?")
        return 1
    return 0


if __name__ == "__main__":
    sys.exit(main())
