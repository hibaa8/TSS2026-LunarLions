# Unity Mission API Backend

Python service that:

- Pulls live telemetry from TSS over UDP (`command=1` for EVA, `command=2` for LTV)
- Maintains an in-memory mission state
- Exposes Unity-friendly REST endpoints under `/api/v1/*`

## Run

```bash
cd backend
python3 -m venv .venv
source .venv/bin/activate
pip install -r requirements.txt
python tss_unity_api.py --tss-host 127.0.0.1 --tss-port 8080 --api-host 0.0.0.0 --api-port 8100
```

> Adjust `--tss-host` and `--tss-port` to your running TSS server.

## Endpoints

- `GET /api/v1/health`
- `GET /api/v1/eva`
- `GET /api/v1/eva/{evaId}` where `evaId` is `eva1` or `eva2`
- `GET /api/v1/eva/errors`
- `GET /api/v1/eva/uia`
- `GET /api/v1/ltv`
- `GET /api/v1/ltv/errors`
- `GET /api/v1/ltv/signal`
- `GET /api/v1/mission/warnings`
- `GET /api/v1/ltv/triage`
- `GET /api/v1/eva/{evaId}/egress-readiness`
- `GET /api/v1/procedures/ltv`
- `GET /api/v1/procedures/ltv/{procedureId}`
- `GET /api/v1/procedures/ltv/{procedureId}/status`

## Procedure Content

Procedure definitions are stored in:

- `backend/procedures/ltv_procedures.json`

This keeps procedure text/content separate from telemetry transport logic.
