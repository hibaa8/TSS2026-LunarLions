# TSS Unity API (Minimal)

Base URL (local default): `http://127.0.0.1:8100`

Path parameter notes:

- `eva_id`: accepts `eva1` or `eva2`
- `procedure_id`: one of the IDs returned by `GET /api/v1/procedures/ltv` (for example `erm`)

## Health

- `GET /api/v1/health`
- Params: none
- Returns:

```json
{ "ok": true, "source_online": true, "last_updated_unix": 1730000000.12 }
```

## EVA

- `GET /api/v1/eva`
- Params: none
- Returns:

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

- `GET /api/v1/eva/{eva_id}`
- Params: `eva_id`
- Returns:

```json
{ "telemetry": {}, "dcu": {}, "imu": {} }
```

- `GET /api/v1/eva/errors`
- Params: none
- Returns:

```json
{ "fan_error": false, "oxy_error": false, "power_error": false, "scrubber_error": false }
```

- `GET /api/v1/eva/{eva_id}/uia`
- Params: `eva_id`
- Returns:

```json
{
  "evaId": "eva1",
  "uia": { "power": false, "oxy": false, "water_supply": false, "water_waste": false },
  "shared": { "oxy_vent": false, "depress": false }
}
```

- `GET /api/v1/eva/{eva_id}/dcu`
- Params: `eva_id`
- Returns:

```json
{ "oxy": true, "fan": true, "pump": false, "co2": false, "batt": { "lu": true, "ps": true } }
```

- `GET /api/v1/eva/{eva_id}/egress-readiness`
- Params: `eva_id`
- Returns:

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

## Convenience EVA slices

- `GET /api/v1/dcu/eva1`
- Params: none
- Returns EVA1 DCU object:

```json
{ "oxy": true, "fan": true, "pump": false, "co2": false, "batt": { "lu": true, "ps": true } }
```

- `GET /api/v1/imu/eva1`
- Params: none
- Returns EVA1 IMU object:

```json
{ "posx": 0, "posy": 0, "heading": 0 }
```

- `GET /api/v1/status` (also `/api/v1/status/`)
- Params: none
- Returns EVA status object:

```json
{ "started": true }
```

## LTV

- `GET /api/v1/ltv`
- Params: none
- Returns:

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

- `GET /api/v1/ltv/errors`
- Params: none
- Returns LTV errors object.
- `GET /api/v1/ltv/signal`
- Params: none
- Returns LTV signal object.
- `GET /api/v1/ltv/location`
- Params: none
- Returns LTV location object.
- `GET /api/v1/ltv/triage`
- Params: none
- Returns:

```json
{
  "active_errors": [{ "key": "recovery_mode", "active": false }]
}
```

## Procedures (content API)

- `GET /api/v1/procedures/ltv`
- Params: none
- Returns list of supported procedure IDs/titles:

```json
{ "procedures": [{ "id": "erm", "title": "Emergency Recovery Mode" }] }
```

- `GET /api/v1/procedures/ltv/{procedure_id}`
- Params: `procedure_id`
- Returns full stored procedure content:

```json
{ "title": "Emergency Recovery Mode", "description": "...", "steps": [] }
```

- `GET /api/v1/procedures/ltv/{procedure_id}/status`
- Params: `procedure_id`
- Returns evaluated completion against current telemetry:

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

