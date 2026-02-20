from __future__ import annotations

from typing import Any, Dict

from .constants import UDP_GET_EVA, UDP_GET_LTV
from .udp_client import TssUdpClient


class TelemetryProvider:
    """Fetches live telemetry from TSS on-demand via UDP."""

    def __init__(self, udp_client: TssUdpClient) -> None:
        self._udp = udp_client

    def get_eva(self) -> Dict[str, Any]:
        return self._udp.request_json(UDP_GET_EVA) or {}

    def get_ltv(self) -> Dict[str, Any]:
        return self._udp.request_json(UDP_GET_LTV) or {}
