from __future__ import annotations

import json
from pathlib import Path
from typing import Any, Dict, List

from .utils import evaluate_criterion


def _read_json(path: Path) -> Any:
    with path.open("r", encoding="utf-8") as f:
        return json.load(f)


class ProcedureEngine:
    def __init__(self, procedures_file: Path) -> None:
        self._procedures = _read_json(procedures_file)
        if not isinstance(self._procedures, dict):
            raise ValueError("Procedure file must be a JSON object.")

    def list_ltv(self) -> List[Dict[str, str]]:
        result = []
        for proc_id, proc in self._procedures.get("ltv", {}).items():
            result.append({"id": proc_id, "title": proc.get("title", proc_id)})
        return result

    def get_ltv(self, procedure_id: str) -> Dict[str, Any]:
        proc = self._procedures.get("ltv", {}).get(procedure_id)
        if proc is None:
            raise KeyError(procedure_id)
        return proc

    def get_ltv_status(self, procedure_id: str, state: Dict[str, Any]) -> Dict[str, Any]:
        proc = self.get_ltv(procedure_id)
        steps_out: List[Dict[str, Any]] = []
        completed = 0

        for step in proc.get("steps", []):
            criteria = step.get("completion_criteria", [])
            checks = []
            all_passed = True

            for criterion in criteria:
                passed = evaluate_criterion(state, criterion)
                checks.append(
                    {
                        "path": criterion["path"],
                        "op": criterion.get("op", "eq"),
                        "value": criterion.get("value"),
                        "pass": passed,
                    }
                )
                all_passed = all_passed and passed

            if all_passed:
                completed += 1

            steps_out.append(
                {
                    "id": step.get("id"),
                    "instruction": step.get("instruction"),
                    "criticality": step.get("criticality", "optional"),
                    "hint": step.get("hint", ""),
                    "voice_short": step.get("voice_short", ""),
                    "complete": all_passed,
                    "checks": checks,
                }
            )

        next_step = next((s for s in steps_out if not s["complete"]), None)
        return {
            "procedure_id": procedure_id,
            "completed_steps": completed,
            "total_steps": len(steps_out),
            "complete": completed == len(steps_out),
            "next_step": next_step,
            "steps": steps_out,
        }
