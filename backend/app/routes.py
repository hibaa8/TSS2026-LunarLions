from __future__ import annotations

import time
from typing import Any, Dict

from fastapi import FastAPI, HTTPException

from .procedures import ProcedureEngine
from .telemetry_provider import TelemetryProvider
from .utils import get_path, normalize_eva_id, to_bool


def register_routes(app: FastAPI, telemetry: TelemetryProvider, procedures: ProcedureEngine) -> None:
    @app.get("/api/v1/health")
    def health() -> Dict[str, Any]:
        eva = telemetry.get_eva()
        ltv = telemetry.get_ltv()
        online = bool(eva) and bool(ltv)
        return {
            "ok": True,
            "source_online": online,
            "last_updated_unix": time.time() if online else 0.0,
        }

    @app.get("/api/v1/eva")
    def get_eva() -> Dict[str, Any]:
        eva = telemetry.get_eva()
        return {
            "status": eva.get("status", {}),
            "telemetry": eva.get("telemetry", {}),
            "dcu": eva.get("dcu", {}),
            "error": eva.get("error", {}),
            "imu": eva.get("imu", {}),
            "uia": eva.get("uia", {}),
        }

    @app.get("/api/v1/eva/{eva_id}")
    def get_eva_by_id(eva_id: str) -> Dict[str, Any]:
        normalized_eva_id = normalize_eva_id(eva_id)
        eva = telemetry.get_eva()
        return {
            "telemetry": eva.get("telemetry", {}).get(normalized_eva_id, {}),
            "dcu": eva.get("dcu", {}).get(normalized_eva_id, {}),
            "imu": eva.get("imu", {}).get(normalized_eva_id, {}),
        }

    @app.get("/api/v1/eva/errors")
    def get_eva_errors() -> Dict[str, Any]:
        return telemetry.get_eva().get("error", {})

    @app.get("/api/v1/eva/{eva_id}/uia")
    def get_eva_uia_by_id(eva_id: str) -> Dict[str, Any]:
        normalized_eva_id = normalize_eva_id(eva_id)
        uia = telemetry.get_eva().get("uia", {})
        prefix = f"{normalized_eva_id}_"
        return {
            "evaId": normalized_eva_id,
            "uia": {
                "power": to_bool(uia.get(f"{prefix}power")),
                "oxy": to_bool(uia.get(f"{prefix}oxy")),
                "water_supply": to_bool(uia.get(f"{prefix}water_supply")),
                "water_waste": to_bool(uia.get(f"{prefix}water_waste")),
            },
            "shared": {
                "oxy_vent": to_bool(uia.get("oxy_vent")),
                "depress": to_bool(uia.get("depress")),
            },
        }

    @app.get("/api/v1/eva/{eva_id}/dcu")
    def get_eva_dcu_by_id(eva_id: str) -> Dict[str, Any]:
        normalized_eva_id = normalize_eva_id(eva_id)
        return telemetry.get_eva().get("dcu", {}).get(normalized_eva_id, {})

    @app.get("/api/v1/dcu/eva1")
    def get_dcu_eva1() -> Dict[str, Any]:
        return telemetry.get_eva().get("dcu", {}).get("eva1", {})

    @app.get("/api/v1/imu/eva1")
    def get_imu_eva1() -> Dict[str, Any]:
        return telemetry.get_eva().get("imu", {}).get("eva1", {})

    @app.get("/api/v1/status")
    @app.get("/api/v1/status/")
    def get_status() -> Dict[str, Any]:
        return telemetry.get_eva().get("status", {})

    @app.get("/api/v1/ltv")
    def get_ltv() -> Dict[str, Any]:
        ltv = telemetry.get_ltv()
        return {
            "location": ltv.get("location", {}),
            "signal": ltv.get("signal", {}),
            "errors": ltv.get("errors", {}),
        }

    @app.get("/api/v1/ltv/errors")
    def get_ltv_errors() -> Dict[str, Any]:
        return telemetry.get_ltv().get("errors", {})

    @app.get("/api/v1/ltv/signal")
    def get_ltv_signal() -> Dict[str, Any]:
        return telemetry.get_ltv().get("signal", {})

    @app.get("/api/v1/ltv/location")
    def get_ltv_location() -> Dict[str, Any]:
        return telemetry.get_ltv().get("location", {})

    @app.get("/api/v1/eva/{eva_id}/egress-readiness")
    def get_egress_readiness(eva_id: str) -> Dict[str, Any]:
        normalized_eva_id = normalize_eva_id(eva_id)
        eva = telemetry.get_eva()
        checks = [
            {
                "name": "mission_started",
                "pass": to_bool(get_path(eva, "status.started", False)),
                "path": "status.started",
            },
            {
                "name": "uia_depress",
                "pass": to_bool(get_path(eva, "uia.depress", False)),
                "path": "uia.depress",
            },
            {
                "name": "uia_oxy_vent",
                "pass": to_bool(get_path(eva, "uia.oxy_vent", False)),
                "path": "uia.oxy_vent",
            },
        ]
        return {"evaId": normalized_eva_id, "checks": checks}

    @app.get("/api/v1/procedures/ltv")
    def get_ltv_procedures() -> Dict[str, Any]:
        return {"procedures": procedures.list_ltv()}

    @app.get("/api/v1/procedures/ltv/{procedure_id}")
    def get_ltv_procedure(procedure_id: str) -> Dict[str, Any]:
        try:
            return procedures.get_ltv(procedure_id)
        except KeyError:
            raise HTTPException(status_code=404, detail="Unknown procedureId")

    @app.get("/api/v1/procedures/ltv/{procedure_id}/status")
    def get_ltv_procedure_status(procedure_id: str) -> Dict[str, Any]:
        merged_state = {"eva": telemetry.get_eva(), "ltv": telemetry.get_ltv()}
        try:
            return procedures.get_ltv_status(procedure_id, merged_state)
        except KeyError:
            raise HTTPException(status_code=404, detail="Unknown procedureId")
