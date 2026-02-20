# TSS Unity API

Prerequisite: before running this backend, start the main TSS server first by following the setup instructions here: [SUITS-Techteam/TSS2026](https://github.com/SUITS-Techteam/TSS2026/tree/main).

Python service that pulls live telemetry from TSS over UDP (`command=1` EVA, `command=2` LTV) on each REST request and exposes endpoints under `/api/v1/*`.

Base URL (local default): `http://127.0.0.1:8100`

Path params:
- `eva_id`: `eva1` or `eva2`
- `procedure_id`: one of `GET /api/v1/procedures/ltv`

## Run

```bash
cd backend
python3 -m venv .venv
source .venv/bin/activate
pip install -r requirements.txt
python tss_unity_api.py --tss-host 10.207.124.137 --tss-port 14141 --api-host 0.0.0.0 --api-port 8100
```

## API

### `GET /api/v1/health`
- Response:
```json
{ "ok": true, "source_online": true, "last_updated_unix": 1730000000.12 }
```

### `GET /api/v1/eva`
- Response:
```json
{
  "status": { "started": true },
  "telemetry": { "eva1": {}, "eva2": {} },
  "dcu": { "eva1": {}, "eva2": {} },
  "error": { "fan_error": false, "oxy_error": false, "power_error": false, "scrubber_error": false },
  "imu": { "eva1": {}, "eva2": {} },
  "uia": {}
}
```

### `GET /api/v1/eva/{eva_id}`
- Parameters: `eva_id` (`eva1` or `eva2`)
- Response:
```json
{ "telemetry": {}, "dcu": {}, "imu": {} }
```

### `GET /api/v1/eva/errors`
- Response:
```json
{ "fan_error": false, "oxy_error": false, "power_error": false, "scrubber_error": false }
```

### `GET /api/v1/eva/{eva_id}/uia`
- Parameters: `eva_id` (`eva1` or `eva2`)
- Response:
```json
{
  "evaId": "eva1",
  "uia": { "power": false, "oxy": false, "water_supply": false, "water_waste": false },
  "shared": { "oxy_vent": false, "depress": false }
}
```

### `GET /api/v1/eva/{eva_id}/dcu`
- Parameters: `eva_id` (`eva1` or `eva2`)
- Response:
```json
{ "oxy": true, "fan": true, "pump": false, "co2": false, "batt": { "lu": true, "ps": true } }
```

### `GET /api/v1/eva/{eva_id}/egress-readiness`
- Parameters: `eva_id` (`eva1` or `eva2`)
- Response:
```json
{
  "evaId": "eva1",
  "checks": [
    { "name": "mission_started", "pass": true, "path": "status.started" },
    { "name": "uia_depress", "pass": false, "path": "uia.depress" },
    { "name": "uia_oxy_vent", "pass": false, "path": "uia.oxy_vent" }
  ]
}
```

### `GET /api/v1/dcu/eva1`
- Response:
```json
{ "oxy": true, "fan": true, "pump": false, "co2": false, "batt": { "lu": true, "ps": true } }
```

### `GET /api/v1/imu/eva1`
- Response:
```json
{ "posx": 0, "posy": 0, "heading": 0 }
```

### `GET /api/v1/status` (also `/api/v1/status/`)
- Response:
```json
{ "started": true }
```

### `GET /api/v1/ltv`
- Response:
```json
{
  "location": { "last_known_x": 0, "last_known_y": 0 },
  "signal": { "strength": 0, "pings_left": 0, "ping_requested": false },
  "errors": {
    "recovery_mode": false,
    "dust_sensor": false,
    "power_distribution": false,
    "nav_system": false,
    "electronic_heater": false,
    "comms": false,
    "fuse": false
  }
}
```

### `GET /api/v1/ltv/errors`
- Response:
```json
{
  "recovery_mode": false,
  "dust_sensor": false,
  "power_distribution": false,
  "nav_system": false,
  "electronic_heater": false,
  "comms": false,
  "fuse": false
}
```

### `GET /api/v1/ltv/signal`
- Response:
```json
{ "strength": 0, "pings_left": 0, "ping_requested": false }
```

### `GET /api/v1/ltv/location`
- Response:
```json
{ "last_known_x": 0, "last_known_y": 0 }
```

### `GET /api/v1/procedures/ltv`
- Response:
```json
{ "procedures": [{ "id": "erm", "title": "Emergency Recovery Mode" }] }
```

### `GET /api/v1/procedures/ltv/{procedure_id}`
- Parameters: `procedure_id`
- Response:
```json
{ "title": "Emergency Recovery Mode", "description": "...", "steps": [] }
```

### `GET /api/v1/procedures/ltv/{procedure_id}/status`
- Parameters: `procedure_id`
- Response:
```json
{
  "procedure_id": "erm",
  "completed_steps": 1,
  "total_steps": 2,
  "complete": false,
  "next_step": {},
  "steps": []
}
```

## Procedure Content

- `backend/procedures/ltv_procedures.json`
