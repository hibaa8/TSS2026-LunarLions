from __future__ import annotations

import json
import socket
import struct
import threading
import time
from typing import Any, Dict, Optional


class TssUdpClient:
    """Minimal UDP GET client matching TSS wire format."""

    def __init__(self, host: str, port: int, timeout_s: float, retries: int) -> None:
        self._retries = retries
        self._lock = threading.Lock()
        self._socket = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        self._socket.settimeout(timeout_s)
        self._socket.connect((host, port))

    def close(self) -> None:
        self._socket.close()

    def request_json(self, command: int) -> Optional[Dict[str, Any]]:
        packet = struct.pack(">II", int(time.time()), command)
        for _ in range(self._retries):
            try:
                with self._lock:
                    self._socket.send(packet)
                    raw = self._socket.recv(65535)
                return self._decode_response(raw)
            except (TimeoutError, socket.timeout):
                continue
            except OSError:
                break
        return None

    @staticmethod
    def _decode_response(raw: bytes) -> Optional[Dict[str, Any]]:
        if not raw:
            return None

        payload = raw[8:] if len(raw) > 8 else raw
        payload = payload.split(b"\x00", 1)[0].strip()
        if not payload:
            return None

        try:
            decoded = payload.decode("utf-8", errors="replace")
            parsed = json.loads(decoded)
            return parsed if isinstance(parsed, dict) else None
        except (UnicodeDecodeError, json.JSONDecodeError):
            return None
