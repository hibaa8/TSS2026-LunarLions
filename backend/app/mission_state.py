from __future__ import annotations

import copy
import threading
import time
from typing import Any, Dict, Optional


class MissionState:
    """Thread-safe in-memory state for Unity queries."""

    def __init__(self) -> None:
        self._lock = threading.Lock()
        self._state: Dict[str, Any] = {
            "eva": {},
            "ltv": {},
            "last_updated_unix": 0.0,
            "source_online": False,
        }

    def update(self, *, eva: Optional[Dict[str, Any]], ltv: Optional[Dict[str, Any]]) -> None:
        with self._lock:
            if eva is not None:
                self._state["eva"] = eva
            if ltv is not None:
                self._state["ltv"] = ltv

            self._state["source_online"] = eva is not None and ltv is not None
            if self._state["source_online"]:
                self._state["last_updated_unix"] = time.time()

    def snapshot(self) -> Dict[str, Any]:
        with self._lock:
            return copy.deepcopy(self._state)
