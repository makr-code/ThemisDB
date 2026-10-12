#!/usr/bin/env python3
"""Final runtime API smoke gate for ThemisDB."""

from __future__ import annotations

import argparse
import json
import sys
import urllib.error
import urllib.request
from dataclasses import dataclass, asdict
from pathlib import Path


@dataclass
class CheckResult:
    name: str
    ok: bool
    status: int
    detail: str


def _request(method: str, url: str, *, data: bytes | None = None, headers: dict[str, str] | None = None, timeout: int = 10) -> tuple[int, str]:
    req = urllib.request.Request(url=url, method=method, data=data, headers=headers or {})
    try:
        with urllib.request.urlopen(req, timeout=timeout) as resp:
            body = resp.read().decode("utf-8", errors="replace")
            return int(resp.status), body
    except urllib.error.HTTPError as exc:
        body = exc.read().decode("utf-8", errors="replace")
        return int(exc.code), body
    except Exception as exc:  # pragma: no cover - network failures
        return 0, str(exc)


def _run(endpoint: str, timeout: int) -> list[CheckResult]:
    checks: list[CheckResult] = []

    status, body = _request("GET", f"{endpoint}/api/health", timeout=timeout)
    health_text = body.lower()
    checks.append(
        CheckResult(
            name="health.api_health",
            ok=status == 200 and ("ok" in health_text or "healthy" in health_text),
            status=status,
            detail="expected 200 + healthy marker",
        )
    )

    status, _ = _request("GET", f"{endpoint}/api/v1/entities/users/_all", timeout=timeout)
    checks.append(
        CheckResult(
            name="auth.protected_endpoint_rejected_without_token",
            ok=status in (401, 403),
            status=status,
            detail="expected 401/403 for unauthenticated protected endpoint",
        )
    )

    status, _ = _request(
        "POST",
        f"{endpoint}/api/v1/query",
        data=b"{",
        headers={"Content-Type": "application/json"},
        timeout=timeout,
    )
    checks.append(
        CheckResult(
            name="validation.invalid_json_rejected",
            ok=status in (400, 415, 422),
            status=status,
            detail="expected 400/415/422 for invalid JSON body",
        )
    )

    status, _ = _request("GET", f"{endpoint}/api/v1/__nonexistent__", timeout=timeout)
    checks.append(
        CheckResult(
            name="routing.unknown_endpoint_not_found",
            ok=status == 404,
            status=status,
            detail="expected 404 for unknown endpoint",
        )
    )

    return checks


def main() -> int:
    parser = argparse.ArgumentParser(description="Run ThemisDB final runtime API smoke gate")
    parser.add_argument("--endpoint", default="http://localhost:18080", help="ThemisDB endpoint (default: http://localhost:18080)")
    parser.add_argument("--timeout", type=int, default=10, help="HTTP timeout in seconds")
    parser.add_argument("--output", default="/tmp/final-runtime-api-gate.json", help="Path to JSON report")
    args = parser.parse_args()

    checks = _run(args.endpoint.rstrip("/"), args.timeout)
    passed = sum(1 for c in checks if c.ok)

    report = {
        "endpoint": args.endpoint,
        "checks_total": len(checks),
        "checks_passed": passed,
        "checks_failed": len(checks) - passed,
        "checks": [asdict(c) for c in checks],
    }

    output = Path(args.output)
    output.parent.mkdir(parents=True, exist_ok=True)
    output.write_text(json.dumps(report, indent=2), encoding="utf-8")

    print("Final Runtime API Gate")
    for c in checks:
        marker = "PASS" if c.ok else "FAIL"
        print(f"[{marker}] {c.name} (status={c.status}) — {c.detail}")
    print(f"Report: {output}")

    if passed != len(checks):
        return 1
    return 0


if __name__ == "__main__":
    sys.exit(main())
