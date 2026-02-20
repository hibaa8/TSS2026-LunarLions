from __future__ import annotations

from typing import Any, Dict

from fastapi import FastAPI, HTTPException

from .mission_state import MissionState
from .procedures import ProcedureEngine
from .utils import get_path, normalize_eva_id, to_bool


def register_routes(app: FastAPI, state: MissionState, procedures: ProcedureEngine) -> None:
    @app.get("/api/v1/health")
    def health() -> Dict[str, Any]:
        snap = state.snapshot()
        return {
            "ok": True,
            "source_online": snap["source_online"],
            "last_updated_unix": snap["last_updated_unix"],
        }

    @app.get("/api/v1/eva")
    def get_eva() -> Dict[str, Any]:
        eva = state.snapshot().get("eva", {})
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
        eva = state.snapshot().get("eva", {})
        return {
            "telemetry": eva.get("telemetry", {}).get(normalized_eva_id, {}),
            "dcu": eva.get("dcu", {}).get(normalized_eva_id, {}),
            "imu": eva.get("imu", {}).get(normalized_eva_id, {}),
        }

    @app.get("/api/v1/eva/errors")
    def get_eva_errors() -> Dict[str, Any]:
        return state.snapshot().get("eva", {}).get("error", {})

    @app.get("/api/v1/eva/{eva_id}/uia")
    def get_eva_uia_by_id(eva_id: str) -> Dict[str, Any]:
        normalized_eva_id = normalize_eva_id(eva_id)
        uia = state.snapshot().get("eva", {}).get("uia", {})
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
        return state.snapshot().get("eva", {}).get("dcu", {}).get(normalized_eva_id, {})

    @app.get("/api/v1/dcu/eva1")
    def get_dcu_eva1() -> Dict[str, Any]:
        return state.snapshot().get("eva", {}).get("dcu", {}).get("eva1", {})

    @app.get("/api/v1/imu/eva1")
    def get_imu_eva1() -> Dict[str, Any]:
        return state.snapshot().get("eva", {}).get("imu", {}).get("eva1", {})

    @app.get("/api/v1/status")
    @app.get("/api/v1/status/")
    def get_status() -> Dict[str, Any]:
        return state.snapshot().get("eva", {}).get("status", {})

    @app.get("/api/v1/ltv")
    def get_ltv() -> Dict[str, Any]:
        ltv = state.snapshot().get("ltv", {})
        return {
            "location": ltv.get("location", {}),
            "signal": ltv.get("signal", {}),
            "errors": ltv.get("errors", {}),
        }

    @app.get("/api/v1/ltv/errors")
    def get_ltv_errors() -> Dict[str, Any]:
        return state.snapshot().get("ltv", {}).get("errors", {})

    @app.get("/api/v1/ltv/signal")
    def get_ltv_signal() -> Dict[str, Any]:
        return state.snapshot().get("ltv", {}).get("signal", {})

    @app.get("/api/v1/ltv/location")
    def get_ltv_location() -> Dict[str, Any]:
        return state.snapshot().get("ltv", {}).get("location", {})

    @app.get("/api/v1/ltv/triage")
    def get_ltv_triage() -> Dict[str, Any]:
        errors = state.snapshot().get("ltv", {}).get("errors", {})
        order = [
            "recovery_mode",
            "power_distribution",
            "electronic_heater",
            "nav_system",
            "fuse",
            "comms",
            "dust_sensor",
        ]
        active_errors = [{"key": key, "active": to_bool(errors.get(key))} for key in order]
        return {"active_errors": active_errors}

    @app.get("/api/v1/eva/{eva_id}/egress-readiness")
    def get_egress_readiness(eva_id: str) -> Dict[str, Any]:
        normalized_eva_id = normalize_eva_id(eva_id)
        eva = state.snapshot().get("eva", {})
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
        snap = state.snapshot()
        merged_state = {"eva": snap.get("eva", {}), "ltv": snap.get("ltv", {})}
        try:
            return procedures.get_ltv_status(procedure_id, merged_state)
        except KeyError:
            raise HTTPException(status_code=404, detail="Unknown procedureId")
