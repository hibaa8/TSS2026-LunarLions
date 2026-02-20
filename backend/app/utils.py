from __future__ import annotations

from typing import Any, Dict

from fastapi import HTTPException

from .constants import EVA_IDS


def to_bool(value: Any) -> bool:
    return bool(value) if value is not None else False


def get_path(source: Dict[str, Any], path: str, default: Any = None) -> Any:
    current: Any = source
    for part in path.split("."):
        if not isinstance(current, dict) or part not in current:
            return default
        current = current[part]
    return current


def normalize_eva_id(eva_id: str) -> str:
    value = eva_id.strip().lower()
    if value in EVA_IDS:
        return value
    raise HTTPException(status_code=404, detail="evaId must be 'eva1' or 'eva2'")


def evaluate_criterion(state: Dict[str, Any], criterion: Dict[str, Any]) -> bool:
    value = get_path(state, criterion["path"])
    op = criterion.get("op", "eq")
    target = criterion.get("value")

    if op == "eq":
        return value == target
    if op == "ne":
        return value != target
    if op == "gt":
        return value is not None and value > target
    if op == "gte":
        return value is not None and value >= target
    if op == "lt":
        return value is not None and value < target
    if op == "lte":
        return value is not None and value <= target
    return False
