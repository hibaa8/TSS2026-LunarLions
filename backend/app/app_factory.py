from __future__ import annotations

from pathlib import Path

from fastapi import FastAPI

from .procedures import ProcedureEngine
from .routes import register_routes
from .telemetry_provider import TelemetryProvider
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

    udp = TssUdpClient(tss_host, tss_port, udp_timeout_s, udp_retries)
    telemetry = TelemetryProvider(udp)
    procedures = ProcedureEngine(procedures_file)

    @app.on_event("shutdown")
    def _shutdown() -> None:
        udp.close()

    register_routes(app, telemetry, procedures)
    return app
