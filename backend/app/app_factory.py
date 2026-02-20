from __future__ import annotations

import threading
from pathlib import Path

from fastapi import FastAPI

from .constants import UDP_GET_EVA, UDP_GET_LTV
from .mission_state import MissionState
from .procedures import ProcedureEngine
from .routes import register_routes
from .udp_client import TssUdpClient


def create_app(
    *,
    tss_host: str,
    tss_port: int,
    poll_interval_s: float,
    udp_timeout_s: float,
    udp_retries: int,
    procedures_file: Path,
) -> FastAPI:
    app = FastAPI(title="TSS Unity Mission API", version="1.0.0")

    state = MissionState()
    udp = TssUdpClient(tss_host, tss_port, udp_timeout_s, udp_retries)
    procedures = ProcedureEngine(procedures_file)
    stop_event = threading.Event()

    def poll_loop() -> None:
        while not stop_event.is_set():
            eva = udp.request_json(UDP_GET_EVA)
            ltv = udp.request_json(UDP_GET_LTV)
            state.update(eva=eva, ltv=ltv)
            stop_event.wait(poll_interval_s)

    @app.on_event("startup")
    def _startup() -> None:
        thread = threading.Thread(target=poll_loop, name="tss-udp-poller", daemon=True)
        thread.start()
        app.state._poller_thread = thread

    @app.on_event("shutdown")
    def _shutdown() -> None:
        stop_event.set()
        udp.close()

    register_routes(app, state, procedures)
    return app
