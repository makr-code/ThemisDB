"""
CHIMERA REST API Server
=======================

Stellt Benchmark-Ergebnisse über eine REST-API und ein interaktives
HTML-Dashboard bereit.

Endpunkte:
    GET /                        → Weiterleitung zu /dashboard
    GET /dashboard               → HTML-Dashboard (BenchmarkDashboard)
    GET /api/v1/status           → Server-Status (JSON)
    GET /api/v1/results          → Alle Ergebniszeilen (JSON-Array)
    GET /api/v1/results/latest   → Aktuellster Checkpoint (JSON-Array)
    GET /api/v1/runs             → Liste der gefundenen Checkpoint-Dateien
    GET /api/v1/dashboard        → Aggregiertes Dashboard-JSON

HTTPS:
    Zertifikat und Schlüssel über --tls-cert / --tls-key oder via
    Umgebungsvariablen CHIMERA_SERVER_TLS_CERT / CHIMERA_SERVER_TLS_KEY
    konfigurieren.

Verwendung:
    python chimera_server.py --results-dir ./endurance_results --port 8443 \\
        --tls-cert certs/server.crt --tls-key certs/server.key

    # Oder via CLI:
    python chimera_cli.py serve --results-dir ./endurance_results --port 8443 --tls-cert ...
"""

import json
import os
import ssl
import time
from dataclasses import dataclass, field
from datetime import datetime, timezone
from pathlib import Path
from typing import Any, Dict, List, Optional

try:
    from flask import Flask, jsonify, redirect, Response, url_for
except ImportError as _e:
    raise ImportError(
        "Flask ist nicht installiert. Bitte 'pip install flask' ausführen."
    ) from _e

# ---------------------------------------------------------------------------
# Interner Import mit Fallback für Standalone-Start
# ---------------------------------------------------------------------------

try:
    from benchmark_harness import WorkloadResult  # type: ignore
    _HAS_HARNESS = True
except ImportError:
    _HAS_HARNESS = False

_CHIMERA_VERSION = "1.0.0"

# ---------------------------------------------------------------------------
# Konfiguration
# ---------------------------------------------------------------------------

@dataclass
class ServerConfig:
    """Konfiguration für den CHIMERA REST-API-Server."""

    results_dir: str = "./endurance_results"
    """Verzeichnis mit Checkpoint-JSON-Dateien."""

    host: str = "0.0.0.0"
    """Lausch-Adresse des Servers."""

    port: int = 8080
    """TCP-Port des Servers (Standard 8080 ohne TLS, 8443 mit TLS)."""

    tls_cert: str = ""
    """Pfad zum TLS-Zertifikat (PEM). Leer = HTTP."""

    tls_key: str = ""
    """Pfad zum privaten TLS-Schlüssel (PEM)."""

    results_cache_ttl: int = 30
    """Sekunden bis zum nächsten Reload der Ergebnisdateien."""

    title: str = "CHIMERA Benchmark Dashboard"
    """Titel des HTML-Dashboards."""

    debug: bool = False
    """Flask Debug-Modus (niemals in Produktion aktivieren)."""


# ---------------------------------------------------------------------------
# Ergebnis-Lade-Helfer
# ---------------------------------------------------------------------------

def _load_result_files(results_dir: Path) -> List[Dict[str, Any]]:
    """Lädt alle Checkpoint- und Final-JSON-Dateien aus *results_dir*.

    Returns:
        Flache Liste aller Ergebniszeilen (dicts).
    """
    rows: List[Dict[str, Any]] = []
    patterns = ["checkpoint_*.json", "endurance_final_*.json", "run_*.json"]
    for pattern in patterns:
        for jf in sorted(results_dir.glob(pattern)):
            try:
                data = json.loads(jf.read_text(encoding="utf-8"))
                if isinstance(data, list):
                    rows.extend(data)
                elif isinstance(data, dict) and data:
                    rows.append(data)
            except (json.JSONDecodeError, OSError):
                pass
    return rows


def _list_run_files(results_dir: Path) -> List[Dict[str, Any]]:
    """Gibt Metadaten aller Ergebnisdateien zurück."""
    entries: List[Dict[str, Any]] = []
    for jf in sorted(results_dir.glob("*.json")):
        if jf.name.startswith("."):
            continue
        stat = jf.stat()
        entries.append({
            "filename": jf.name,
            "size_bytes": stat.st_size,
            "modified": datetime.fromtimestamp(stat.st_mtime, tz=timezone.utc).isoformat(),
        })
    return entries


def _latest_checkpoint_rows(results_dir: Path) -> List[Dict[str, Any]]:
    """Gibt die Zeilen des aktuellsten Checkpoints zurück."""
    checkpoints = sorted(results_dir.glob("checkpoint_*.json"))
    if not checkpoints:
        # Fallback: aktuellste run_*.json
        runs = sorted(results_dir.glob("run_*.json"))
        if not runs:
            return []
        checkpoints = runs
    try:
        data = json.loads(checkpoints[-1].read_text(encoding="utf-8"))
        return data if isinstance(data, list) else ([data] if data else [])
    except (json.JSONDecodeError, OSError):
        return []


def _load_test_topology(results_dir: Path) -> Optional[Dict[str, Any]]:
    path = results_dir / "test_topology.json"
    if not path.exists():
        return None
    try:
        data = json.loads(path.read_text(encoding="utf-8"))
    except (json.JSONDecodeError, OSError):
        return None
    return data if isinstance(data, dict) else None


def _load_latest_system_metrics(results_dir: Path) -> Optional[Dict[str, Any]]:
    path = results_dir / "system_metrics.jsonl"
    if not path.exists():
        return None
    try:
        lines = [line.strip() for line in path.read_text(encoding="utf-8").splitlines() if line.strip()]
    except OSError:
        return None
    if not lines:
        return None
    try:
        data = json.loads(lines[-1])
    except json.JSONDecodeError:
        return None
    return data if isinstance(data, dict) else None


# ---------------------------------------------------------------------------
# Dashboard-Aggregation aus flachen Zeilen
# ---------------------------------------------------------------------------

def _aggregate_from_rows(rows: List[Dict[str, Any]], title: str) -> Optional[Dict[str, Any]]:
    """Aggregiert flache Ergebniszeilen zu einem Dashboard-JSON.

    Wenn das BenchmarkDashboard-Modul verfügbar ist, werden WorkloadResult-
    Objekte synthetisch aufgebaut und die volle Scoring-Pipeline genutzt.
    Anderenfalls wird ein kompaktes Summary-Dict zurückgegeben.

    Returns:
        Dashboard-JSON-Dict oder None bei leeren Daten.
    """
    if not rows:
        return None

    if _HAS_HARNESS:
        dashboard_result = _aggregate_via_dashboard(rows, title)
        if dashboard_result is not None:
            return dashboard_result

    # Lightweight-Fallback ohne benchmark_harness
    by_workload: Dict[str, Dict[str, Any]] = {}
    for row in rows:
        wid = str(row.get("workload_id", "unknown"))
        if wid not in by_workload:
            by_workload[wid] = {
                "throughput_ops_per_sec": [],
                "p99_latency_ms": [],
                "error_count": [],
            }
        by_workload[wid]["throughput_ops_per_sec"].append(
            float(row.get("throughput_ops_per_sec", 0.0))
        )
        by_workload[wid]["p99_latency_ms"].append(float(row.get("p99_latency_ms", 0.0)))
        by_workload[wid]["error_count"].append(int(row.get("error_count", 0)))

    summary: Dict[str, Any] = {}
    for wid, buckets in by_workload.items():
        n = len(buckets["throughput_ops_per_sec"])
        summary[wid] = {
            "samples": n,
            "throughput_mean": round(sum(buckets["throughput_ops_per_sec"]) / n, 2),
            "p99_mean": round(sum(buckets["p99_latency_ms"]) / n, 3),
            "total_errors": sum(buckets["error_count"]),
        }

    return {
        "title": title,
        "generated_at": datetime.now(timezone.utc).isoformat(),
        "workload_summary": summary,
    }


def _aggregate_via_dashboard(
    rows: List[Dict[str, Any]], title: str
) -> Optional[Dict[str, Any]]:
    """Vollständige Aggregation über BenchmarkDashboard + BenchmarkScorer."""
    try:
        from dashboard import BenchmarkDashboard  # type: ignore
        from benchmark_harness import WorkloadResult  # type: ignore
        from dataclasses import fields as dc_fields
    except ImportError:
        return None

    dash = BenchmarkDashboard(title=title)

    for row in rows:
        wid = str(row.get("workload_id", "unknown"))
        system = str(row.get("system_name", "ThemisDB"))

        # Synthetisches WorkloadResult aus flacher Zeile
        result = WorkloadResult(
            workload_id=wid,
            system_name=system,
            latencies_ms=[float(row.get("mean_latency_ms", 0.0))],
            throughput_ops_per_sec=float(row.get("throughput_ops_per_sec", 0.0)),
            error_count=int(row.get("error_count", 0)),
            warmup_iterations=0,
            run_iterations=1,
            elapsed_seconds=float(row.get("elapsed_seconds", 0.0)),
            mean_latency_ms=float(row.get("mean_latency_ms", 0.0)),
            p50_latency_ms=float(row.get("p50_latency_ms", 0.0)),
            p95_latency_ms=float(row.get("p95_latency_ms", 0.0)),
            p99_latency_ms=float(row.get("p99_latency_ms", 0.0)),
        )
        dash.add_workload_result(result)

    try:
        return dash.aggregate()
    except ValueError:
        return None


# ---------------------------------------------------------------------------
# Flask App Factory
# ---------------------------------------------------------------------------

def _build_app(cfg: ServerConfig) -> Flask:
    """Erzeugt die Flask-App mit allen Routen."""

    app = Flask(__name__)
    app.config["JSON_SORT_KEYS"] = False

    results_dir = Path(cfg.results_dir)
    _cache: Dict[str, Any] = {"rows": [], "ts": 0.0, "runs_ts": 0.0, "runs": []}
    _start_time = time.time()

    def _get_rows() -> List[Dict[str, Any]]:
        """Rows mit TTL-Cache."""
        now = time.time()
        if now - _cache["ts"] > cfg.results_cache_ttl:
            _cache["rows"] = _load_result_files(results_dir)
            _cache["ts"] = now
        return _cache["rows"]

    def _get_runs() -> List[Dict[str, Any]]:
        now = time.time()
        if now - _cache["runs_ts"] > cfg.results_cache_ttl:
            _cache["runs"] = _list_run_files(results_dir)
            _cache["runs_ts"] = now
        return _cache["runs"]

    # ---------------------------------------------------------------- /

    @app.get("/")
    def root() -> Response:
        return redirect("/dashboard", code=302)

    # --------------------------------------------------------- /dashboard

    @app.get("/dashboard")
    def dashboard_html() -> Response:
        rows = _get_rows()

        if not rows:
            html = (
                "<!DOCTYPE html><html><head><meta charset='UTF-8'>"
                f"<title>{cfg.title}</title></head><body>"
                f"<h1>{cfg.title}</h1>"
                "<p>Noch keine Ergebnisse in "
                f"<code>{cfg.results_dir}</code> gefunden.</p>"
                "</body></html>"
            )
            return Response(html, mimetype="text/html")

        # Versuche vollständiges BenchmarkDashboard HTML
        try:
            from dashboard import BenchmarkDashboard  # type: ignore
            from benchmark_harness import WorkloadResult  # type: ignore

            dash = BenchmarkDashboard(title=cfg.title)
            for row in rows:
                wid = str(row.get("workload_id", "unknown"))
                system = str(row.get("system_name", "ThemisDB"))
                result = WorkloadResult(
                    workload_id=wid,
                    system_name=system,
                    latencies_ms=[float(row.get("mean_latency_ms", 0.0))],
                    throughput_ops_per_sec=float(row.get("throughput_ops_per_sec", 0.0)),
                    error_count=int(row.get("error_count", 0)),
                    warmup_iterations=0,
                    run_iterations=1,
                    elapsed_seconds=float(row.get("elapsed_seconds", 0.0)),
                    mean_latency_ms=float(row.get("mean_latency_ms", 0.0)),
                    p50_latency_ms=float(row.get("p50_latency_ms", 0.0)),
                    p95_latency_ms=float(row.get("p95_latency_ms", 0.0)),
                    p99_latency_ms=float(row.get("p99_latency_ms", 0.0)),
                )
                dash.add_workload_result(result)

            from io import StringIO
            import tempfile, os as _os

            with tempfile.NamedTemporaryFile(
                mode="w", suffix=".html", delete=False, encoding="utf-8"
            ) as tmp:
                tmp_path = tmp.name
            try:
                dash.generate_html_dashboard(tmp_path)
                html = Path(tmp_path).read_text(encoding="utf-8")
            finally:
                _os.unlink(tmp_path)

            return Response(html, mimetype="text/html")

        except Exception:
            # Minimales HTML-Fallback
            agg = _aggregate_from_rows(rows, cfg.title)
            html = _render_minimal_html(cfg.title, agg, rows)
            return Response(html, mimetype="text/html")

    # ------------------------------------------------------ /api/v1/status

    @app.get("/api/v1/status")
    def api_status() -> Response:
        rows = _get_rows()
        topology = _load_test_topology(results_dir) or {}
        latest_system_metrics = _load_latest_system_metrics(results_dir)
        hb_path = results_dir / ".heartbeat"
        heartbeat_age_sec: Optional[float] = None
        if hb_path.exists():
            try:
                hb_ts = datetime.fromisoformat(
                    hb_path.read_text(encoding="utf-8").strip()
                )
                heartbeat_age_sec = round(
                    (datetime.now(timezone.utc) - hb_ts).total_seconds(), 1
                )
            except ValueError:
                pass

        scheme = "https" if cfg.tls_cert else "http"
        return jsonify({
            "status": "ok",
            "chimera_version": _CHIMERA_VERSION,
            "server": f"{scheme}://{cfg.host}:{cfg.port}",
            "results_dir": str(results_dir.resolve()),
            "rows_loaded": len(rows),
            "uptime_seconds": round(time.time() - _start_time, 1),
            "cache_ttl_seconds": cfg.results_cache_ttl,
            "heartbeat_age_seconds": heartbeat_age_sec,
            "database_info": topology.get("database_info"),
            "host_baseline": topology.get("host_baseline"),
            "latest_system_metrics": latest_system_metrics,
            "generated_at": datetime.now(timezone.utc).isoformat(),
        })

    # ----------------------------------------------------- /api/v1/results

    @app.get("/api/v1/results")
    def api_results() -> Response:
        return jsonify(_get_rows())

    # ----------------------------------------------- /api/v1/results/latest

    @app.get("/api/v1/results/latest")
    def api_results_latest() -> Response:
        # Immer frisch lesen (kein Cache), da "latest" zeitkritisch ist
        rows = _latest_checkpoint_rows(results_dir)
        return jsonify(rows)

    # -------------------------------------------------------- /api/v1/runs

    @app.get("/api/v1/runs")
    def api_runs() -> Response:
        return jsonify(_get_runs())

    # ------------------------------------------------- /api/v1/dashboard

    @app.get("/api/v1/dashboard")
    def api_dashboard_json() -> Response:
        rows = _get_rows()
        agg = _aggregate_from_rows(rows, cfg.title)
        if agg is None:
            return jsonify({"error": "Keine Ergebnisse verfügbar"}), 404
        return jsonify(agg)

    @app.get("/api/v1/system-metrics/latest")
    def api_system_metrics_latest() -> Response:
        latest = _load_latest_system_metrics(results_dir)
        if latest is None:
            return jsonify({"error": "Keine Systemmetriken verfügbar"}), 404
        return jsonify(latest)

    return app


# ---------------------------------------------------------------------------
# Minimales HTML-Fallback (ohne BenchmarkDashboard)
# ---------------------------------------------------------------------------

def _render_minimal_html(
    title: str,
    agg: Optional[Dict[str, Any]],
    rows: List[Dict[str, Any]],
) -> str:
    """Rendert ein einfaches HTML-Dashboard ohne externe Abhängigkeiten."""
    ts = datetime.now(timezone.utc).isoformat()
    rows_html = ""
    for r in rows[-50:]:  # letzte 50 Einträge
        rows_html += (
            f"<tr>"
            f"<td>{r.get('timestamp','')}</td>"
            f"<td>{r.get('workload_id','')}</td>"
            f"<td>{r.get('throughput_ops_per_sec',0):.1f}</td>"
            f"<td>{r.get('p50_latency_ms',0):.2f}</td>"
            f"<td>{r.get('p99_latency_ms',0):.2f}</td>"
            f"<td>{r.get('error_count',0)}</td>"
            f"</tr>\n"
        )
    return f"""<!DOCTYPE html>
<html lang="de">
<head><meta charset="UTF-8"><title>{title}</title>
<style>
body{{font-family:sans-serif;max-width:1200px;margin:0 auto;padding:20px}}
h1{{border-bottom:2px solid #336;padding-bottom:8px}}
table{{width:100%;border-collapse:collapse;margin:16px 0}}
th{{background:#336;color:#fff;padding:8px 10px;text-align:left}}
td{{padding:6px 10px;border-bottom:1px solid #ddd}}
tr:hover{{background:#f5f5f5}}
.meta{{color:#666;font-size:.9em}}
</style>
</head>
<body>
<h1>🔬 {title}</h1>
<p class="meta">CHIMERA v{_CHIMERA_VERSION} &nbsp;|&nbsp; Generiert: {ts}</p>
<h2>Ergebnisse (letzte 50)</h2>
<table>
<thead><tr>
<th>Zeitstempel</th><th>Workload</th>
<th>Throughput (ops/s)</th><th>p50 (ms)</th><th>p99 (ms)</th><th>Fehler</th>
</tr></thead>
<tbody>{rows_html}</tbody>
</table>
</body></html>"""


# ---------------------------------------------------------------------------
# TLS-Kontext
# ---------------------------------------------------------------------------

def _build_ssl_context(cert: str, key: str) -> ssl.SSLContext:
    """Erzeugt einen SSLContext für den Flask-Server.

    Args:
        cert: Pfad zum PEM-Zertifikat.
        key: Pfad zum privaten PEM-Schlüssel.

    Returns:
        Konfigurierter SSLContext.

    Raises:
        FileNotFoundError: Wenn Zertifikat oder Schlüssel nicht gefunden.
        ssl.SSLError: Wenn die Dateien ungültig sind.
    """
    cert_path = Path(cert)
    key_path = Path(key)

    if not cert_path.exists():
        raise FileNotFoundError(f"TLS-Zertifikat nicht gefunden: {cert_path}")
    if not key_path.exists():
        raise FileNotFoundError(f"TLS-Schlüssel nicht gefunden: {key_path}")

    ctx = ssl.SSLContext(ssl.PROTOCOL_TLS_SERVER)
    ctx.minimum_version = ssl.TLSVersion.TLSv1_2
    ctx.load_cert_chain(certfile=str(cert_path), keyfile=str(key_path))
    return ctx


# ---------------------------------------------------------------------------
# Öffentliche Einstiegsfunktion
# ---------------------------------------------------------------------------

def run_server(cfg: ServerConfig) -> None:
    """Startet den CHIMERA REST-API-Server.

    Lädt TLS-Kontext wenn ``cfg.tls_cert`` gesetzt ist, sonst HTTP.

    Args:
        cfg: Server-Konfiguration.
    """
    results_dir = Path(cfg.results_dir)
    results_dir.mkdir(parents=True, exist_ok=True)

    app = _build_app(cfg)

    ssl_ctx: Optional[ssl.SSLContext] = None
    scheme = "http"
    if cfg.tls_cert:
        if not cfg.tls_key:
            raise ValueError("--tls-cert angegeben, aber --tls-key fehlt.")
        ssl_ctx = _build_ssl_context(cfg.tls_cert, cfg.tls_key)
        scheme = "https"

    print(f"  CHIMERA Server  v{_CHIMERA_VERSION}")
    print(f"  Adresse:        {scheme}://{cfg.host}:{cfg.port}")
    print(f"  Dashboard:      {scheme}://{cfg.host}:{cfg.port}/dashboard")
    print(f"  API-Base:       {scheme}://{cfg.host}:{cfg.port}/api/v1/")
    print(f"  Ergebnisse:     {results_dir.resolve()}")
    print(f"  TLS:            {'an (' + cfg.tls_cert + ')' if ssl_ctx else 'aus (HTTP)'}")
    print(f"  Cache-TTL:      {cfg.results_cache_ttl} s")
    print()

    app.run(
        host=cfg.host,
        port=cfg.port,
        debug=cfg.debug,
        ssl_context=ssl_ctx,
        use_reloader=False,
    )


# ---------------------------------------------------------------------------
# Standalone-Start
# ---------------------------------------------------------------------------

def _parse_args():
    import argparse

    p = argparse.ArgumentParser(
        prog="chimera_server",
        description="CHIMERA REST API Server – Benchmark-Ergebnisse per HTTP(S) bereitstellen",
    )
    p.add_argument(
        "--results-dir",
        default=os.environ.get("RESULTS_DIR", "./endurance_results"),
        metavar="DIR",
        help="Verzeichnis mit Ergebnisdateien (Standard: ./endurance_results)",
    )
    p.add_argument(
        "--host",
        default=os.environ.get("CHIMERA_SERVER_HOST", "0.0.0.0"),
        metavar="ADDR",
        help="Lausch-Adresse (Standard: 0.0.0.0)",
    )
    p.add_argument(
        "--port",
        type=int,
        default=int(os.environ.get("CHIMERA_SERVER_PORT", "8080")),
        metavar="PORT",
        help="TCP-Port (Standard: 8080 / $CHIMERA_SERVER_PORT)",
    )
    p.add_argument(
        "--tls-cert",
        default=os.environ.get("CHIMERA_SERVER_TLS_CERT", ""),
        metavar="FILE",
        help="PEM-Zertifikat für HTTPS ($CHIMERA_SERVER_TLS_CERT)",
    )
    p.add_argument(
        "--tls-key",
        default=os.environ.get("CHIMERA_SERVER_TLS_KEY", ""),
        metavar="FILE",
        help="Privater PEM-Schlüssel für HTTPS ($CHIMERA_SERVER_TLS_KEY)",
    )
    p.add_argument(
        "--cache-ttl",
        type=int,
        default=int(os.environ.get("CHIMERA_SERVER_CACHE_TTL", "30")),
        metavar="SEC",
        help="Cache-TTL für Ergebnisdateien in Sekunden (Standard: 30)",
    )
    p.add_argument(
        "--title",
        default=os.environ.get("CHIMERA_SERVER_TITLE", "CHIMERA Benchmark Dashboard"),
        metavar="STR",
        help="Dashboard-Titel",
    )
    p.add_argument(
        "--debug",
        action="store_true",
        help="Flask Debug-Modus (nur Entwicklung!)",
    )
    return p.parse_args()


if __name__ == "__main__":
    args = _parse_args()
    cfg = ServerConfig(
        results_dir=args.results_dir,
        host=args.host,
        port=args.port,
        tls_cert=args.tls_cert,
        tls_key=args.tls_key,
        results_cache_ttl=args.cache_ttl,
        title=args.title,
        debug=args.debug,
    )
    run_server(cfg)
