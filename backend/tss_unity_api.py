#!/usr/bin/env python3

from __future__ import annotations

import argparse
from pathlib import Path

from app import create_app


def _parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description="Unity-facing TSS mission API")
    parser.add_argument("--tss-host", default="127.0.0.1", help="TSS UDP host")
    parser.add_argument("--tss-port", type=int, default=8080, help="TSS UDP port")
    parser.add_argument("--api-host", default="127.0.0.1", help="Mission API bind host")
    parser.add_argument("--api-port", type=int, default=8100, help="Mission API bind port")
    parser.add_argument("--poll-interval", type=float, default=0.25, help="UDP poll interval seconds")
    parser.add_argument("--udp-timeout", type=float, default=0.5, help="UDP receive timeout seconds")
    parser.add_argument("--udp-retries", type=int, default=2, help="Retries per UDP request")
    parser.add_argument(
        "--procedures-file",
        default=str(Path(__file__).parent / "procedures" / "ltv_procedures.json"),
        help="Path to LTV procedures JSON content",
    )
    return parser.parse_args()


if __name__ == "__main__":
    args = _parse_args()
    app = create_app(
        tss_host=args.tss_host,
        tss_port=args.tss_port,
        poll_interval_s=args.poll_interval,
        udp_timeout_s=args.udp_timeout,
        udp_retries=args.udp_retries,
        procedures_file=Path(args.procedures_file),
    )

    import uvicorn

    uvicorn.run(app, host=args.api_host, port=args.api_port)
