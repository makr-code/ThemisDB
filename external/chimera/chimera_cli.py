"""
CHIMERA Suite – Kommandozeilen-Controller
==========================================
Zentrales CLI-Werkzeug für alle CHIMERA-Operationen.

Verwendung:
    python chimera_cli.py <Befehl> [Optionen]

Befehle:
    run         Benchmark ausführen (einmalig oder fortlaufend)
    endurance   24h-Endurance-Test starten
    export      Ergebnisse/Berichte exportieren
    status      Laufenden Test überwachen
    validate    Konfigurationsdatei prüfen
    connect     Verbindung zu ThemisDB testen
    list        Verfügbare Workloads und Adapter anzeigen

Beispiele:
    # Schnell-Benchmark gegen lokale ThemisDB
    python chimera_cli.py run --host localhost --workloads ycsb_a,ycsb_c

    # 24h-Endurance-Test via HTTPS auf QNAP
    python chimera_cli.py endurance --host 192.168.1.100 --hours 24 --tls

    # Alle Checkpoints im Verzeichnis zu HTML/CSV exportieren
    python chimera_cli.py export --input ./endurance_results --format html,csv

    # Status eines laufenden Tests prüfen
    python chimera_cli.py status --results-dir ./endurance_results

    # Konfiguration validieren
    python chimera_cli.py validate --config endurance_config.toml

    # Verbindung testen
    python chimera_cli.py connect --host 192.168.1.100 --port 18765 --tls
"""

import argparse
import json
import os
import pathlib
import sys
import time
from datetime import datetime, timezone
from typing import List, Optional

from benchmark_standards import (
    CURRENT_CHIMERA_WORKLOADS,
    get_profile_catalog,
    get_standard_catalog,
    get_standard_compliance_requirements,
    get_standard_coverage_snapshot,
    resolve_profile_workloads,
)

# ---------------------------------------------------------------------------
# Ausgabe-Helfer
# ---------------------------------------------------------------------------

_BOLD = "\033[1m"
_GREEN = "\033[92m"
_YELLOW = "\033[93m"
_RED = "\033[91m"
_CYAN = "\033[96m"
_RESET = "\033[0m"


def _ok(msg: str) -> None:
    print(f"{_GREEN}✓{_RESET}  {msg}")


def _warn(msg: str) -> None:
    print(f"{_YELLOW}⚠{_RESET}  {msg}", file=sys.stderr)


def _err(msg: str) -> None:
    print(f"{_RED}✗{_RESET}  {msg}", file=sys.stderr)


def _info(msg: str) -> None:
    print(f"{_CYAN}»{_RESET}  {msg}")


def _header(title: str) -> None:
    width = max(60, len(title) + 4)
    print(f"\n{_BOLD}{'─' * width}")
    print(f"  {title}")
    print(f"{'─' * width}{_RESET}\n")


# ---------------------------------------------------------------------------
# Argument-Parser
# ---------------------------------------------------------------------------

def _build_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(
        prog="chimera_cli",
        description=(
            "CHIMERA Suite – Vendor-neutrales Datenbank-Benchmark-Framework\n"
            "Umgebungsvariablen THEMISDB_PASSWORD und THEMISDB_USER werden\n"
            "automatisch übernommen, wenn --password / --user nicht gesetzt sind."
        ),
        formatter_class=argparse.RawDescriptionHelpFormatter,
    )
    parser.add_argument(
        "--version", action="version", version="CHIMERA CLI 1.0.0"
    )

    subs = parser.add_subparsers(dest="command", metavar="<Befehl>")
    subs.required = True

    # ------------------------------------------------------------------ run
    p_run = subs.add_parser(
        "run",
        help="Benchmark einmalig ausführen",
        description="Führt einen oder mehrere Workloads gegen ThemisDB aus.",
    )
    _add_connection_args(p_run)
    _add_workload_args(p_run)
    p_run.add_argument(
        "--warmup", type=int, default=100,
        metavar="N", help="Anzahl Warm-up-Iterationen (Standard: 100)",
    )
    p_run.add_argument(
        "--iterations", type=int, default=1000,
        metavar="N", help="Anzahl Mess-Iterationen (Standard: 1000)",
    )
    p_run.add_argument(
        "--workers", type=int, default=1,
        metavar="N", help="Parallele Benchmark-Worker (Standard: 1)",
    )
    p_run.add_argument(
        "--output", default="./chimera_results",
        metavar="DIR", help="Ausgabeverzeichnis (Standard: ./chimera_results)",
    )
    p_run.add_argument(
        "--format", default="json,csv,html",
        metavar="FMT", help="Ausgabeformate: json,csv,html (Standard: alle)",
    )
    p_run.add_argument(
        "--sort", choices=["alphabetical", "metric"], default="alphabetical",
        help="Sortiermethode im Bericht (Standard: alphabetical)",
    )

    # -------------------------------------------------------------- endurance
    p_end = subs.add_parser(
        "endurance",
        help="24h-Endurance-Test starten",
        description=(
            "Startet den kontinuierlichen Endurance-Test via run_endurance.py.\n"
            "Schreibt Checkpoint-Reports alle INTERVAL Minuten."
        ),
    )
    _add_connection_args(p_end)
    _add_workload_args(p_end)
    p_end.add_argument(
        "--hours", type=float, default=24.0,
        metavar="H", help="Gesamte Testdauer in Stunden (Standard: 24)",
    )
    p_end.add_argument(
        "--interval", type=float, default=30.0,
        metavar="MIN", help="Checkpoint-Intervall in Minuten (Standard: 30)",
    )
    p_end.add_argument(
        "--workers", type=int, default=4,
        metavar="N", help="Parallele Benchmark-Worker (Standard: 4)",
    )
    p_end.add_argument(
        "--warmup", type=int, default=200,
        metavar="N", help="Warm-up-Iterationen (Standard: 200)",
    )
    p_end.add_argument(
        "--output", default="./endurance_results",
        metavar="DIR", help="Ausgabeverzeichnis (Standard: ./endurance_results)",
    )
    p_end.add_argument(
        "--config", default=None,
        metavar="FILE", help="TOML-Konfigurationsdatei (überschreibt CLI-Argumente)",
    )
    p_end.add_argument(
        "--docker", action="store_true",
        help="Über docker compose starten statt direkt",
    )

    # --------------------------------------------------------------- export
    p_exp = subs.add_parser(
        "export",
        help="Ergebnisse exportieren",
        description=(
            "Liest Checkpoint-JSON-Dateien aus einem Verzeichnis und\n"
            "generiert zusammengefasste Berichte in den gewünschten Formaten."
        ),
    )
    p_exp.add_argument(
        "--input", required=True,
        metavar="DIR|FILE", help="Eingabe: Verzeichnis mit JSON-Checkpoints oder einzelne JSON-Datei",
    )
    p_exp.add_argument(
        "--output", default=None,
        metavar="DIR", help="Ausgabeverzeichnis (Standard: gleich wie --input)",
    )
    p_exp.add_argument(
        "--format", default="json,csv,html",
        metavar="FMT",
        help="Komma-getrennte Formate: json, csv, html, md, xlsx (Standard: json,csv,html)",
    )
    p_exp.add_argument(
        "--name", default=None,
        metavar="NAME", help="Basisname für Ausgabedateien (Standard: export_<timestamp>)",
    )
    p_exp.add_argument(
        "--sort", choices=["alphabetical", "metric", "time"], default="time",
        help="Sortierung der Ergebnisse im Bericht (Standard: time)",
    )
    p_exp.add_argument(
        "--aggregate", action="store_true",
        help="Alle Checkpoints zu einem Gesamtbericht zusammenfassen",
    )
    p_exp.add_argument(
        "--pattern", default="*.json",
        metavar="GLOB", help="Glob-Muster für Eingabedateien (Standard: *.json)",
    )

    # --------------------------------------------------------------- status
    p_sta = subs.add_parser(
        "status",
        help="Laufenden Test überwachen",
        description="Zeigt Echtzeit-Status eines laufenden Endurance-Tests.",
    )
    p_sta.add_argument(
        "--results-dir", default="./endurance_results",
        metavar="DIR", help="Ergebnisverzeichnis (Standard: ./endurance_results)",
    )
    p_sta.add_argument(
        "--watch", action="store_true",
        help="Kontinuierlich aktualisieren (alle 60 s)",
    )
    p_sta.add_argument(
        "--interval", type=int, default=60,
        metavar="SEC", help="Aktualisierungsintervall in Sekunden bei --watch (Standard: 60)",
    )

    # ------------------------------------------------------------- validate
    p_val = subs.add_parser(
        "validate",
        help="Konfigurationsdatei prüfen",
        description="Prüft eine TOML-Konfigurationsdatei auf Vollständigkeit und Korrektheit.",
    )
    p_val.add_argument(
        "--config", required=True,
        metavar="FILE", help="Pfad zur TOML-Konfigurationsdatei",
    )

    # --------------------------------------------------------------- connect
    p_con = subs.add_parser(
        "connect",
        help="Verbindung zu ThemisDB testen",
        description="Prüft die Erreichbarkeit und TLS-Konfiguration von ThemisDB.",
    )
    _add_connection_args(p_con)
    p_con.add_argument(
        "--verbose", action="store_true",
        help="Zertifikats- und Verbindungsdetails ausgeben",
    )

    # --------------------------------------------------------------- list
    p_lst = subs.add_parser(
        "list",
        help="Verfügbare Workloads und Adapter anzeigen",
    )
    p_lst.add_argument(
        "--adapters", action="store_true",
        help="Registrierte Adapter-Konfigurationen anzeigen",
    )
    p_lst.add_argument(
        "--standards", action="store_true",
        help="Etablierte Benchmark-Standards anzeigen (YCSB, TPC, TSBS, LDBC, ANN, BEIR, MLPerf)",
    )
    p_lst.add_argument(
        "--profiles", action="store_true",
        help="CHIMERA Profile fuer Spezialdomaenen anzeigen",
    )
    p_lst.add_argument(
        "--profile", default="",
        metavar="ID",
        help="Details fuer ein Profil anzeigen (z. B. timeseries, geo, process, llm, ml, lora, rag, full)",
    )
    p_lst.add_argument(
        "--emit-workloads", action="store_true",
        help="Bei --profile: workload-Liste als CSV-Zeile ausgeben",
    )
    p_lst.add_argument(
        "--search-standards", default="",
        metavar="KEYWORD",
        help="Standards nach Keyword durchsuchen (z. B. tpc, vector, rag)",
    )
    p_lst.add_argument(
        "--standard-details", default="",
        metavar="ID",
        help="Detaillierte Info zu Standard anzeigen (z. B. MLPerf, BEIR, YCSB)",
    )
    p_lst.add_argument(
        "--standard-coverage",
        action="store_true",
        help="Implementierungs-Coverage pro Standard anzeigen",
    )
    p_lst.add_argument(
        "--workloads-for-standard", default="",
        metavar="ID",
        help="Alle Workloads fuer einen Standard anzeigen",
    )
    p_lst.add_argument(
        "--compliance", default="",
        metavar="ID",
        help="Compliance-Requirements fuer Standard anzeigen",
    )

    # ---------------------------------------------------------------- serve
    p_srv = subs.add_parser(
        "serve",
        help="REST-API-Server mit Dashboard starten",
        description=(
            "Startet den CHIMERA REST-API-Server.\n"
            "Liest Ergebnisdateien aus --results-dir und stellt sie über\n"
            "HTTP(S) bereit. Dashboard unter /dashboard, API unter /api/v1/.\n\n"
            "HTTPS:  --tls-cert und --tls-key angeben oder per Umgebungsvariablen\n"
            "        CHIMERA_SERVER_TLS_CERT / CHIMERA_SERVER_TLS_KEY setzen."
        ),
    )
    p_srv.add_argument(
        "--results-dir",
        default=os.environ.get("RESULTS_DIR", "./endurance_results"),
        metavar="DIR",
        help="Ergebnisverzeichnis (Standard: ./endurance_results / $RESULTS_DIR)",
    )
    p_srv.add_argument(
        "--server-host",
        default=os.environ.get("CHIMERA_SERVER_HOST", "0.0.0.0"),
        metavar="ADDR",
        help="Lausch-Adresse (Standard: 0.0.0.0 / $CHIMERA_SERVER_HOST)",
    )
    p_srv.add_argument(
        "--server-port",
        type=int,
        default=int(os.environ.get("CHIMERA_SERVER_PORT", "8080")),
        metavar="PORT",
        help="TCP-Port (Standard: 8080 / $CHIMERA_SERVER_PORT)",
    )
    p_srv.add_argument(
        "--tls-cert",
        default=os.environ.get("CHIMERA_SERVER_TLS_CERT", ""),
        metavar="FILE",
        help="PEM-Zertifikat für HTTPS ($CHIMERA_SERVER_TLS_CERT)",
    )
    p_srv.add_argument(
        "--tls-key",
        default=os.environ.get("CHIMERA_SERVER_TLS_KEY", ""),
        metavar="FILE",
        help="Privater PEM-Schlüssel für HTTPS ($CHIMERA_SERVER_TLS_KEY)",
    )
    p_srv.add_argument(
        "--cache-ttl",
        type=int,
        default=int(os.environ.get("CHIMERA_SERVER_CACHE_TTL", "30")),
        metavar="SEC",
        help="Cache-TTL für Ergebnisdateien in Sekunden (Standard: 30)",
    )
    p_srv.add_argument(
        "--title",
        default=os.environ.get("CHIMERA_SERVER_TITLE", "CHIMERA Benchmark Dashboard"),
        metavar="STR",
        help="Dashboard-Titel",
    )
    p_srv.add_argument(
        "--debug",
        action="store_true",
        help="Flask Debug-Modus (nur Entwicklung!)",
    )

    return parser


def _add_connection_args(p: argparse.ArgumentParser) -> None:
    grp = p.add_argument_group("ThemisDB-Verbindung")
    grp.add_argument(
        "--host", default=os.environ.get("THEMISDB_HOST", "localhost"),
        metavar="HOST", help="ThemisDB-Host (Standard: localhost / $THEMISDB_HOST)",
    )
    grp.add_argument(
        "--port", type=int, default=int(os.environ.get("THEMISDB_PORT", "18765")),
        metavar="PORT", help="ThemisDB-Port (Standard: 18765 / $THEMISDB_PORT)",
    )
    grp.add_argument(
        "--db", default=os.environ.get("THEMISDB_DB", "benchmark_db"),
        metavar="DB", help="Datenbankname (Standard: benchmark_db / $THEMISDB_DB)",
    )
    grp.add_argument(
        "--user", default=os.environ.get("THEMISDB_USER", "bench_user"),
        metavar="USER", help="Benutzername ($THEMISDB_USER)",
    )
    grp.add_argument(
        "--password", default=os.environ.get("THEMISDB_PASSWORD", ""),
        metavar="PASS", help="Passwort ($THEMISDB_PASSWORD)",
    )
    grp.add_argument(
        "--tls", action="store_true",
        default=os.environ.get("THEMISDB_USE_TLS", "false").lower() in ("1", "true", "yes"),
        help="HTTPS/TLS aktivieren",
    )
    grp.add_argument(
        "--ca-cert", default=os.environ.get("THEMISDB_CA_CERT", ""),
        metavar="FILE", help="Pfad zum CA-Zertifikat ($THEMISDB_CA_CERT)",
    )
    grp.add_argument(
        "--no-verify-tls", action="store_true",
        help="TLS-Zertifikat NICHT prüfen (nur isolierte Netze!)",
    )


def _add_workload_args(p: argparse.ArgumentParser) -> None:
    grp = p.add_argument_group("Workloads")
    grp.add_argument(
        "--workloads", default="",
        metavar="LIST",
        help="Komma-getrennte Workload-IDs (optional; überschreibt --profile)",
    )
    grp.add_argument(
        "--profile",
        default="",
        metavar="ID",
        help="Benchmark-Profil-ID (z. B. core,timeseries,geo,process,llm,ml,lora,rag,full)",
    )
    grp.add_argument(
        "--profile-strict",
        action="store_true",
        help="Bei Profilnutzung strikt abbrechen, wenn required Workloads nicht verfuegbar sind",
    )
    grp.add_argument(
        "--capabilities",
        default="",
        metavar="LIST",
        help="Komma-getrennte Runtime-Capabilities (Override, z. B. kv_read,kv_write,vector_search)",
    )
    grp.add_argument(
        "--capabilities-endpoint",
        default="/api/v1/capabilities",
        metavar="PATH",
        help="Capability-Discovery Endpoint (Standard: /api/v1/capabilities)",
    )


def _resolve_selected_workloads(args: argparse.Namespace) -> List[str]:
    if getattr(args, "workloads", ""):
        return [w.strip() for w in args.workloads.split(",") if w.strip()]

    if getattr(args, "profile", ""):
        profile_id = args.profile.strip().lower()
        required, _, _ = resolve_profile_workloads(profile_id)
        return required

    return [
        "ycsb_a",
        "ycsb_b",
        "ycsb_c",
        "ycsb_f",
        "vector_search",
        "graph_traversal",
    ]


def _derive_domain_metrics_from_row(row: dict) -> dict:
    wid = str(row.get("workload_id", ""))
    p95 = float(row.get("p95_latency_ms", 0.0))
    p99 = float(row.get("p99_latency_ms", 0.0))
    errors = int(row.get("error_count", 0))
    throughput = float(row.get("throughput_ops_per_sec", 0.0))

    domain = "core"
    if wid.startswith("ts_"):
        domain = "timeseries"
    elif wid.startswith("geo_"):
        domain = "geo"
    elif wid.startswith("process_"):
        domain = "process"
    elif wid.startswith("llm_"):
        domain = "llm"
    elif wid.startswith("ml_") or wid.startswith("ethics_"):
        domain = "ml"
    elif wid.startswith("lora_"):
        domain = "lora"
    elif wid.startswith("rag_"):
        domain = "rag"
    elif wid.startswith("tpc_"):
        domain = "oltp"

    quality_score = max(0.0, min(1.0, 1.0 - (p99 / 2000.0) - (errors * 0.01)))
    stability_score = max(0.0, min(1.0, 1.0 - (p95 / 1000.0) - (errors * 0.01)))

    derived = {
        "benchmark_domain": domain,
        "quality_score": round(quality_score, 4),
        "stability_score": round(stability_score, 4),
    }

    if domain == "timeseries":
        derived["ingest_rate_points_per_sec"] = round(throughput, 2)
    elif domain == "geo":
        derived["geo_query_latency_p95_ms"] = round(p95, 3)
    elif domain == "process":
        derived["conformance_rate"] = round(max(0.0, min(1.0, 1.0 - errors * 0.02)), 4)
    elif domain in {"llm", "ml", "lora"}:
        derived["tokens_or_inferences_per_sec"] = round(throughput, 2)
    elif domain == "rag":
        derived["retrieval_quality_score"] = round(quality_score, 4)

    return derived


# ---------------------------------------------------------------------------
# Befehlsimplementierungen
# ---------------------------------------------------------------------------

# ------------------------------------------------------------------ run ----

def _cmd_run(args: argparse.Namespace) -> int:
    _header("CHIMERA Benchmark – Einzellauf")

    try:
        from run_endurance import (
            ThemisDBClient,
            EnduranceConfig,
            _build_workloads,
            _validate_workload_capabilities,
        )
        from benchmark_harness import BenchmarkHarness, HarnessConfig
        from exporter import ChimeraExporter
    except ImportError as exc:
        _err(f"Import fehlgeschlagen: {exc}")
        return 1

    cfg = _conn_cfg_from_args(args)
    cfg.warmup_iterations = args.warmup
    cfg.run_iterations = args.iterations
    cfg.parallel_workers = args.workers
    cfg.capabilities_endpoint = args.capabilities_endpoint
    cfg.capabilities_override = args.capabilities

    client = ThemisDBClient(cfg)
    _info(f"Verbinde mit {'https' if cfg.use_tls else 'http'}://{cfg.host}:{cfg.port} …")
    if not client.ping():
        _err("ThemisDB nicht erreichbar.")
        return 1
    _ok("Verbindung hergestellt.")

    try:
        workload_ids = _resolve_selected_workloads(args)
    except ValueError as exc:
        _err(str(exc))
        return 1

    if args.profile and not args.workloads:
        _info(f"Profil aktiv: {args.profile}")

    if args.profile_strict:
        if args.capabilities:
            runtime_caps = [c.strip().lower() for c in args.capabilities.split(",") if c.strip()]
            cap_source = "cli_override"
        else:
            runtime_caps = client.discover_capabilities(args.capabilities_endpoint)
            cap_source = f"endpoint:{args.capabilities_endpoint}"

        if not runtime_caps:
            _err(
                "Profile-Strict aktiv, aber keine Runtime-Capabilities verfuegbar. "
                "Nutze --capabilities oder pruefe --capabilities-endpoint."
            )
            return 2

        coverage = _validate_workload_capabilities(workload_ids, runtime_caps)
        if not coverage["ok"]:
            _err(f"Capability-Abdeckung unvollstaendig ({cap_source}).")
            for wid, missing_caps in coverage["missing_by_workload"].items():
                _err(f"  {wid}: fehlt {', '.join(missing_caps)}")
            return 2
        _ok(f"Capability-Abdeckung ok ({cap_source}).")

    all_wl = _build_workloads(client)
    results = []

    harness_cfg = HarnessConfig(
        warmup_iterations=args.warmup,
        run_iterations=args.iterations,
    )

    _info(f"Warm-up ({args.warmup} Iterationen) …")
    for wid in workload_ids:
        if wid not in all_wl:
            _warn(f"Unbekannter Workload '{wid}' – übersprungen.")
            continue
        h = BenchmarkHarness("ThemisDB", harness_cfg)
        h.add_workload(all_wl[wid])
        h.warm_up(wid)

    _info("Messung läuft …")
    for wid in workload_ids:
        if wid not in all_wl:
            continue
        h = BenchmarkHarness("ThemisDB", HarnessConfig(warmup_iterations=0, run_iterations=args.iterations))
        h.add_workload(all_wl[wid])
        r = h.run(wid)
        row = {
            "workload_id": wid,
            "timestamp": datetime.now(timezone.utc).isoformat(),
            "throughput_ops_per_sec": round(r.throughput_ops_per_sec, 2),
            "mean_latency_ms": round(r.mean_latency_ms, 3),
            "p50_latency_ms": round(r.p50_latency_ms, 3),
            "p95_latency_ms": round(r.p95_latency_ms, 3),
            "p99_latency_ms": round(r.p99_latency_ms, 3),
            "error_count": r.error_count,
            "elapsed_seconds": round(r.elapsed_seconds, 2),
        }
        row.update(_derive_domain_metrics_from_row(row))
        results.append(row)
        _ok(
            f"{wid:25s}  {r.throughput_ops_per_sec:8.1f} ops/s  "
            f"p50={r.p50_latency_ms:.1f}ms  p99={r.p99_latency_ms:.1f}ms  "
            f"Fehler={r.error_count}"
        )

    # Export
    out_dir = pathlib.Path(args.output)
    out_dir.mkdir(parents=True, exist_ok=True)
    ts = datetime.now(timezone.utc).strftime("%Y%m%dT%H%M%SZ")
    formats = [f.strip() for f in args.format.split(",")]

    exporter = ChimeraExporter(results, out_dir)
    written = exporter.export(formats, base_name=f"run_{ts}", sort_by=args.sort)

    print()
    _header("Ergebnisse")
    for path in written:
        _ok(str(path))
    return 0


# -------------------------------------------------------------- endurance --

def _cmd_endurance(args: argparse.Namespace) -> int:
    _header("CHIMERA Endurance-Test")

    if args.docker:
        return _start_endurance_docker(args)

    # Umgebungsvariablen für run_endurance.py setzen
    os.environ["THEMISDB_HOST"] = args.host
    os.environ["THEMISDB_PORT"] = str(args.port)
    os.environ["THEMISDB_USER"] = args.user
    os.environ["THEMISDB_PASSWORD"] = args.password
    os.environ["THEMISDB_DB"] = args.db
    os.environ["THEMISDB_USE_TLS"] = "true" if args.tls else "false"
    os.environ["THEMISDB_CA_CERT"] = args.ca_cert
    os.environ["THEMISDB_VERIFY_TLS"] = "false" if args.no_verify_tls else "true"
    os.environ["ENDURANCE_HOURS"] = str(args.hours)
    os.environ["PARALLEL_WORKERS"] = str(args.workers)
    os.environ["RESULTS_DIR"] = args.output
    os.environ["REPORT_INTERVAL_MIN"] = str(args.interval)
    if args.profile:
        os.environ["TEST_PROFILE"] = args.profile.strip().lower()
    os.environ["TEST_PROFILE_STRICT"] = "true" if args.profile_strict else "false"
    if args.capabilities:
        os.environ["TEST_DB_CAPABILITIES"] = args.capabilities
    os.environ["TEST_DB_CAPABILITIES_ENDPOINT"] = args.capabilities_endpoint

    try:
        selected_workloads = _resolve_selected_workloads(args)
    except ValueError as exc:
        _err(str(exc))
        return 1

    if selected_workloads:
        os.environ["TEST_WORKLOADS"] = ",".join(selected_workloads)

    if args.config:
        # Config-Datei als Override in /app/endurance_config.toml simulieren
        import shutil
        shutil.copy(args.config, pathlib.Path(__file__).parent / "endurance_config.toml")
        _ok(f"Konfiguration aus {args.config} geladen.")

    _info(f"Ziel:     {'https' if args.tls else 'http'}://{args.host}:{args.port}")
    _info(f"Dauer:    {args.hours} Stunden")
    _info(f"Ausgabe:  {args.output}")
    _info(f"Workloads: {', '.join(selected_workloads)}")

    # run_endurance.main() direkt aufrufen
    try:
        import run_endurance
        run_endurance.main()
    except ImportError as exc:
        _err(f"run_endurance.py nicht gefunden: {exc}")
        return 1
    return 0


def _start_endurance_docker(args: argparse.Namespace) -> int:
    import subprocess
    _info("Starte via docker compose …")

    env_vars = [
        f"THEMISDB_HOST={args.host}",
        f"THEMISDB_PORT={args.port}",
        f"THEMISDB_USER={args.user}",
        f"THEMISDB_PASSWORD={args.password}",
        f"THEMISDB_DB={args.db}",
        f"THEMISDB_USE_TLS={'true' if args.tls else 'false'}",
        f"THEMISDB_VERIFY_TLS={'false' if args.no_verify_tls else 'true'}",
        f"ENDURANCE_HOURS={args.hours}",
        f"PARALLEL_WORKERS={args.workers}",
        f"REPORT_INTERVAL_MIN={args.interval}",
        f"TEST_PROFILE_STRICT={'true' if args.profile_strict else 'false'}",
        f"TEST_DB_CAPABILITIES_ENDPOINT={args.capabilities_endpoint}",
    ]
    if args.profile:
        env_vars.append(f"TEST_PROFILE={args.profile.strip().lower()}")

    try:
        selected_workloads = _resolve_selected_workloads(args)
    except ValueError as exc:
        _err(str(exc))
        return 1

    if selected_workloads:
        env_vars.append(f"TEST_WORKLOADS={','.join(selected_workloads)}")
    if args.capabilities:
        env_vars.append(f"TEST_DB_CAPABILITIES={args.capabilities}")
    env_file = pathlib.Path(".env")
    env_file.write_text("\n".join(env_vars) + "\n", encoding="utf-8")
    _ok(f".env geschrieben ({env_file.resolve()})")

    cmd = ["docker", "compose", "up", "--build", "-d"]
    result = subprocess.run(cmd, cwd=str(pathlib.Path(__file__).parent))
    if result.returncode != 0:
        _err("docker compose fehlgeschlagen.")
        return result.returncode

    _ok("Container gestartet. Logs: docker compose logs -f")
    return 0


# --------------------------------------------------------------- export ----

def _cmd_export(args: argparse.Namespace) -> int:
    _header("CHIMERA Exporter")

    try:
        from exporter import ChimeraExporter
    except ImportError as exc:
        _err(f"exporter.py nicht gefunden: {exc}")
        return 1

    input_path = pathlib.Path(args.input)
    if not input_path.exists():
        _err(f"Eingabepfad existiert nicht: {input_path}")
        return 1

    # JSON-Daten einlesen
    if input_path.is_file():
        json_files = [input_path]
    else:
        json_files = sorted(input_path.glob(args.pattern))
        # Heartbeat-Datei und finale Zusammenfassung herausfiltern
        json_files = [f for f in json_files if not f.name.startswith(".")]

    if not json_files:
        _err(f"Keine Dateien gefunden: {input_path / args.pattern}")
        return 1

    _info(f"{len(json_files)} Datei(en) gefunden.")

    all_rows = []
    for jf in json_files:
        try:
            data = json.loads(jf.read_text(encoding="utf-8"))
            if isinstance(data, list):
                all_rows.extend(data)
            elif isinstance(data, dict):
                all_rows.append(data)
        except (json.JSONDecodeError, OSError) as exc:
            _warn(f"  Übersprungen ({jf.name}): {exc}")

    if not all_rows:
        _err("Keine auswertbaren Daten gefunden.")
        return 1

    _info(f"{len(all_rows)} Datensätze geladen.")

    out_dir = pathlib.Path(args.output) if args.output else input_path if input_path.is_dir() else input_path.parent
    out_dir.mkdir(parents=True, exist_ok=True)

    ts = datetime.now(timezone.utc).strftime("%Y%m%dT%H%M%SZ")
    base_name = args.name or f"export_{ts}"
    formats = [f.strip() for f in args.format.split(",")]

    exporter = ChimeraExporter(all_rows, out_dir)
    written = exporter.export(
        formats,
        base_name=base_name,
        sort_by=args.sort,
        aggregate=args.aggregate,
    )

    print()
    for path in written:
        _ok(str(path))
    return 0


# --------------------------------------------------------------- status ----

def _cmd_status(args: argparse.Namespace) -> int:
    _header("CHIMERA Endurance-Status")

    def _show_status(results_dir: pathlib.Path) -> None:
        if not results_dir.exists():
            _err(f"Verzeichnis nicht gefunden: {results_dir}")
            return

        # Heartbeat prüfen
        hb = results_dir / ".heartbeat"
        if hb.exists():
            hb_ts = hb.read_text(encoding="utf-8").strip()
            try:
                hb_dt = datetime.fromisoformat(hb_ts)
                age_sec = (datetime.now(timezone.utc) - hb_dt).total_seconds()
                if age_sec < 120:
                    _ok(f"Test läuft  (letzter Heartbeat vor {age_sec:.0f} s)")
                else:
                    _warn(f"Kein Heartbeat seit {age_sec:.0f} s – Test evtl. gestoppt.")
            except ValueError:
                _warn("Heartbeat-Datei konnte nicht gelesen werden.")
        else:
            _warn("Keine Heartbeat-Datei – kein laufender Test erkannt.")

        # Checkpoints zählen
        checkpoints = sorted(results_dir.glob("checkpoint_*.json"))
        _info(f"Checkpoints:  {len(checkpoints)}")

        if checkpoints:
            last = checkpoints[-1]
            try:
                rows = json.loads(last.read_text(encoding="utf-8"))
                if isinstance(rows, list) and rows:
                    _info(f"Letzter Checkpoint:  {last.name}")
                    _info(f"  Zeitstempel:  {rows[-1].get('timestamp', 'unbekannt')}")
                    print()
                    # Kurzübersicht letztes Fenster
                    _print_results_table(rows)
            except (json.JSONDecodeError, OSError):
                pass

        # Finale Berichte
        finals = list(results_dir.glob("endurance_final_*.json"))
        if finals:
            _ok(f"Abschlussbericht vorhanden: {finals[-1].name}")

    def _print_results_table(rows: list) -> None:
        print(f"  {'Workload':<25} {'Throughput':>12} {'p50 ms':>8} {'p99 ms':>8} {'Fehler':>7}")
        print(f"  {'─'*25} {'─'*12} {'─'*8} {'─'*8} {'─'*7}")
        for r in rows:
            print(
                f"  {r.get('workload_id','?'):<25} "
                f"{r.get('throughput_ops_per_sec', 0):>11.1f} "
                f"{r.get('p50_latency_ms', 0):>8.2f} "
                f"{r.get('p99_latency_ms', 0):>8.2f} "
                f"{r.get('error_count', 0):>7}"
            )

    results_dir = pathlib.Path(args.results_dir)
    _show_status(results_dir)

    if args.watch:
        _info(f"\nAktualisierung alle {args.interval} s  (STRG+C zum Beenden)\n")
        try:
            while True:
                time.sleep(args.interval)
                print("\033[2J\033[H", end="")  # Terminal leeren
                _header("CHIMERA Endurance-Status")
                _show_status(results_dir)
        except KeyboardInterrupt:
            print()
    return 0


# ------------------------------------------------------------- validate ----

def _cmd_validate(args: argparse.Namespace) -> int:
    _header("Konfiguration validieren")

    cfg_path = pathlib.Path(args.config)
    if not cfg_path.exists():
        _err(f"Datei nicht gefunden: {cfg_path}")
        return 1

    try:
        import toml
        raw = toml.loads(cfg_path.read_text(encoding="utf-8"))
    except Exception as exc:
        _err(f"TOML-Parse-Fehler: {exc}")
        return 1

    errors = 0
    warnings = 0

    def _check(section: str, key: str, typ: type, required: bool = True) -> None:
        nonlocal errors, warnings
        val = raw.get(section, {}).get(key)
        if val is None:
            if required:
                _err(f"[{section}].{key} fehlt (Pflichtfeld)")
                errors += 1
            else:
                _warn(f"[{section}].{key} nicht gesetzt (optional)")
                warnings += 1
        elif not isinstance(val, typ):
            _err(f"[{section}].{key} erwartet {typ.__name__}, erhalten: {type(val).__name__}")
            errors += 1
        else:
            _ok(f"[{section}].{key} = {val!r}")

    # ThemisDB
    _check("themisdb", "host", str)
    _check("themisdb", "port", int)
    _check("themisdb", "database", str)
    _check("themisdb", "username", str)
    _check("themisdb", "use_tls", bool, required=False)

    # Endurance
    _check("endurance", "duration_hours", float, required=False)
    _check("endurance", "parallel_workers", int, required=False)
    _check("endurance", "warmup_iterations", int, required=False)
    _check("endurance", "run_iterations", int, required=False)

    # Sicherheitshinweis
    pwd = raw.get("themisdb", {}).get("password", "")
    if pwd and pwd not in ("changeme", "secure_password", ""):
        _warn("Passwort in TOML-Datei gespeichert – besser $THEMISDB_PASSWORD verwenden.")
        warnings += 1

    print()
    if errors == 0:
        _ok(f"Konfiguration gültig  ({warnings} Warnung(en))")
        return 0
    else:
        _err(f"{errors} Fehler, {warnings} Warnungen gefunden.")
        return 1


# --------------------------------------------------------------- connect --

def _cmd_connect(args: argparse.Namespace) -> int:
    _header("ThemisDB Verbindungstest")

    try:
        from run_endurance import ThemisDBClient, EnduranceConfig
    except ImportError as exc:
        _err(f"run_endurance.py nicht gefunden: {exc}")
        return 1

    cfg = _conn_cfg_from_args(args)
    scheme = "https" if cfg.use_tls else "http"
    _info(f"Verbinde mit {scheme}://{cfg.host}:{cfg.port}/{cfg.database} …")
    if cfg.use_tls:
        _info(f"TLS:        an  |  CA-Cert: {cfg.ca_cert or '(System-CAs)'}")
        _info(f"Verifizierung: {'aus (unsicher!)' if not cfg.verify_tls else 'an'}")

    client = ThemisDBClient(cfg)

    try:
        t0 = time.perf_counter()
        ok = client.ping()
        rtt = (time.perf_counter() - t0) * 1000
    except Exception as exc:
        _err(f"Verbindungsfehler: {exc}")
        return 1

    if ok:
        _ok(f"ThemisDB erreichbar  (RTT {rtt:.1f} ms)")
        if getattr(args, "verbose", False):
            _info(f"  Host:      {cfg.host}:{cfg.port}")
            _info(f"  Datenbank: {cfg.database}")
            _info(f"  Benutzer:  {cfg.username}")
            _info(f"  Protokoll: {'HTTPS' if cfg.use_tls else 'HTTP'}")
        return 0
    else:
        _err("ThemisDB antwortete nicht korrekt (Health-Endpoint gab nicht 200 zurück).")
        return 1


# --------------------------------------------------------------- list ----

def _cmd_list(args: argparse.Namespace) -> int:
    _header("Verfügbare Workloads & Adapter")

    workloads = {
        "ycsb_a":        ("YCSB Workload A", "50% Read / 50% Update"),
        "ycsb_b":        ("YCSB Workload B", "95% Read /  5% Update"),
        "ycsb_c":        ("YCSB Workload C", "100% Read"),
        "ycsb_d":        ("YCSB Workload D", "95% Read /  5% Insert (latest)"),
        "ycsb_e":        ("YCSB Workload E", "95% Scan  /  5% Insert"),
        "ycsb_f":        ("YCSB Workload F", "50% Read  / 50% Read-Modify-Write"),
        "vector_search": ("Vector Search",   "128-dim k-NN (Top-10)"),
        "graph_traversal":("Graph Traversal","BFS bis Tiefe 3"),
        "ts_ingest_raw": ("Timeseries",      "Raw Ingest pro Punkt"),
        "ts_ingest_batch":("Timeseries",     "Batch Ingest (50 Punkte)"),
        "ts_range_query": ("Timeseries",      "Zeitfenster-Abfrage"),
        "geo_intersects": ("Geospatial",      "Intersects Bounding Box"),
        "geo_contains":   ("Geospatial",      "Contains Point"),
        "geo_range_query":("Geospatial",      "Radius/Range Query"),
        "process_discovery_alpha": ("Process", "Discovery (Alpha)"),
        "process_discovery_heuristic": ("Process", "Discovery (Heuristic)"),
        "process_conformance": ("Process", "Conformance Check"),
        "llm_serving_latency": ("LLM", "Serving Latency"),
        "llm_serving_throughput": ("LLM", "Serving Throughput"),
        "ml_policy_eval": ("ML", "Policy Evaluation"),
        "ethics_eval_pipeline": ("ML", "Ethics Batch Pipeline"),
        "lora_load_apply": ("LoRA", "Load + Apply"),
        "lora_switch_overhead": ("LoRA", "Adapter Switch Overhead"),
        "rag_retrieval_quality": ("RAG", "Retrieval Quality"),
        "rag_qa_e2e": ("RAG", "End-to-End QA"),
        "tpc_c_mix": ("OLTP", "TPC-C Mix (Lite)"),
    }

    print(f"  {'ID':<20} {'Familie':<20} {'Beschreibung'}")
    print(f"  {'─'*20} {'─'*20} {'─'*40}")
    for wid, (family, desc) in workloads.items():
        print(f"  {wid:<20} {family:<20} {desc}")

    if args.standards:
        print()
        _header("Etablierte Benchmark-Standards")
        standards = get_standard_catalog()
        print(f"  {'ID':<16} {'Domäne':<20} {'Metriken'}")
        print(f"  {'─'*16} {'─'*20} {'─'*60}")
        for std in standards:
            metrics = ", ".join(std.primary_metrics)
            print(f"  {std.standard_id:<16} {std.domain:<20} {metrics}")
        print()
        _info("Hinweis: CHIMERA uebernimmt Standards als workload-semantische Profile, nicht als vendor-spezifische Adapter.")

    if args.profiles:
        print()
        _header("CHIMERA Profilkatalog")
        profiles = get_profile_catalog()
        print(f"  {'Profil':<14} {'Standards':<44} {'Beschreibung'}")
        print(f"  {'─'*14} {'─'*44} {'─'*40}")
        for profile_id, profile in profiles.items():
            standards = ",".join(profile.standards)
            print(f"  {profile_id:<14} {standards:<44} {profile.description}")

    if args.profile:
        profile_id = args.profile.strip().lower()
        profiles = get_profile_catalog()
        profile = profiles.get(profile_id)
        print()
        if profile is None:
            _err(f"Unbekanntes Profil: {profile_id}")
            _info(f"Verfuegbare Profile: {', '.join(sorted(profiles.keys()))}")
            return 1

        required, available_now, missing_now = resolve_profile_workloads(profile_id)
        _header(f"Profil-Details: {profile_id}")
        _info(f"Beschreibung: {profile.description}")
        _info(f"Standards:   {', '.join(profile.standards)}")

        print()
        print(f"  {'Workload':<28} {'Status'}")
        print(f"  {'─'*28} {'─'*20}")
        for wid in required:
            status = "verfuegbar" if wid in available_now else "geplant"
            print(f"  {wid:<28} {status}")

        print()
        _info(f"Aktuell in CHIMERA vorhanden: {len(CURRENT_CHIMERA_WORKLOADS)}")
        _info(f"Profil-Required verfuegbar:   {len(available_now)} / {len(required)}")
        if missing_now:
            _warn("Noch zu implementieren: " + ", ".join(missing_now))

        if args.emit_workloads:
            print()
            print(",".join(required))

    if args.adapters:
        print()
        _header("Adapter-Konfigurationen")
        cfg_file = pathlib.Path(__file__).parent / "adapters" / "chimera" / "adapter_config_example.toml"
        if cfg_file.exists():
            try:
                import toml
                raw = toml.loads(cfg_file.read_text(encoding="utf-8"))
                for section in raw:
                    _ok(f"[{section}]  –  host={raw[section].get('host','?')}:{raw[section].get('port','?')}")
            except Exception:
                _info(f"Konfigurationsbeispiel: {cfg_file}")
        else:
            _warn("adapter_config_example.toml nicht gefunden.")

    # ====== CLI für Standard-Suites erweitern ======
    if args.search_standards:
        keyword = args.search_standards.strip().lower()
        standards = get_standard_catalog()
        matches = [
            s for s in standards
            if keyword in s.standard_id.lower()
            or keyword in s.domain.lower()
            or any(keyword in m.lower() for m in s.primary_metrics)
        ]
        print()
        _header(f"Suchresultate für '{keyword}'")
        if not matches:
            _warn(f"Kein Standard gefunden, der '{keyword}' enthält")
        else:
            print(f"  {'ID':<16} {'Domäne':<20} {'Metriken'}")
            print(f"  {'─'*16} {'─'*20} {'─'*60}")
            for std in matches:
                metrics = ", ".join(std.primary_metrics)
                print(f"  {std.standard_id:<16} {std.domain:<20} {metrics}")

    if args.standard_details:
        std_id = args.standard_details.strip()
        standards = get_standard_catalog()
        std = next((s for s in standards if s.standard_id == std_id), None)
        print()
        if std is None:
            _err(f"Standard nicht gefunden: {std_id}")
            _info(f"Verfügbar: {', '.join(s.standard_id for s in standards)}")
            return 1

        _header(f"Standard-Details: {std_id}")
        _info(f"Domäne:      {std.domain}")
        _info(f"Metriken:    {', '.join(std.primary_metrics)}")
        _info(f"Beschreibung: {std.description}")

    if args.standard_coverage:
        print()
        _header("Implementierungs-Coverage pro Standard")
        coverage = get_standard_coverage_snapshot()
        print(f"  {'Standard':<16} {'Status':<12} {'Details'}")
        print(f"  {'─'*16} {'─'*12} {'─'*60}")
        for std_id, info in coverage.items():
            status = info.get("status", "unknown")
            status_icon = "✓" if status == "implemented" else "◐" if status == "partial" else "○"
            details = info.get("notes", "")
            print(f"  {std_id:<16} {status_icon:<12} {details}")

    if args.workloads_for_standard:
        std_id = args.workloads_for_standard.strip()
        standards = get_standard_catalog()
        std = next((s for s in standards if s.standard_id == std_id), None)
        print()
        if std is None:
            _err(f"Standard nicht gefunden: {std_id}")
            return 1

        _header(f"Workloads für Standard: {std_id}")
        # Filtern von workload mapping (sollte in benchmark_standards.py vorhandene Zuordnung sein)
        matching_workloads = [
            (wid, winfo)
            for wid, winfo in [
                ("ycsb_a", ("YCSB Workload A", "50% Read / 50% Update")),
                ("ycsb_b", ("YCSB Workload B", "95% Read /  5% Update")),
                ("ycsb_c", ("YCSB Workload C", "100% Read")),
                ("ycsb_d", ("YCSB Workload D", "95% Read /  5% Insert (latest)")),
                ("ycsb_e", ("YCSB Workload E", "95% Scan  /  5% Insert")),
                ("ycsb_f", ("YCSB Workload F", "50% Read  / 50% Read-Modify-Write")),
                ("vector_search", ("Vector Search", "128-dim k-NN (Top-10)")),
                ("rag_retrieval_quality", ("RAG", "Retrieval Quality")),
                ("rag_qa_e2e", ("RAG", "End-to-End QA")),
                ("llm_serving_latency", ("LLM", "Serving Latency")),
                ("llm_serving_throughput", ("LLM", "Serving Throughput")),
                ("tpc_c_mix", ("OLTP", "TPC-C Mix (Lite)")),
            ]
            if std_id in winfo[0] or (std_id == "YCSB" and "YCSB" in winfo[0]) or (std_id == "BEIR" and "RAG" in winfo[0]) or (std_id == "MLPerf" and ("LLM" in winfo[0] or "ML" in winfo[0]))
        ]
        if not matching_workloads:
            _warn(f"Keine Workloads für {std_id} direkt zugeordnet. Siehe --profiles für Profile mit mehreren Standards.")
        else:
            print(f"  {'Workload-ID':<28} {'Familie':<20} {'Beschreibung'}")
            print(f"  {'─'*28} {'─'*20} {'─'*40}")
            for wid, (family, desc) in matching_workloads:
                print(f"  {wid:<28} {family:<20} {desc}")

    if args.compliance:
        std_id = args.compliance.strip()
        reqs = get_standard_compliance_requirements()
        req = reqs.get(std_id)
        print()
        if req is None:
            _err(f"Compliance-Anforderungen nicht gefunden: {std_id}")
            _info(f"Verfügbar: {', '.join(reqs.keys())}")
            return 1

        _header(f"Compliance-Anforderungen: {std_id}")
        _info(f"Workloads: {len(req.workloads)}")
        _info(f"Erforderliche Metriken: {len(req.required_metrics)}")
        _info(f"Profile: {len(req.benchmark_profiles)}")
        if hasattr(req, "compliance_notes") and req.compliance_notes:
            _info(f"Hinweise: {req.compliance_notes}")

    return 0


# ---------------------------------------------------------------------------
# Hilfsfunktion: Verbindungsparameter aus args → EnduranceConfig
# ---------------------------------------------------------------------------

def _conn_cfg_from_args(args: argparse.Namespace):
    from run_endurance import EnduranceConfig
    cfg = EnduranceConfig()
    cfg.host = args.host
    cfg.port = args.port
    cfg.database = args.db
    cfg.username = args.user
    cfg.password = args.password
    cfg.use_tls = args.tls
    cfg.ca_cert = args.ca_cert
    cfg.verify_tls = not args.no_verify_tls
    return cfg


# ---------------------------------------------------------------------------
# Entry-Point
# ---------------------------------------------------------------------------

# ---------------------------------------------------------------- serve ----

def _cmd_serve(args: argparse.Namespace) -> int:
    _header("CHIMERA REST-API-Server")

    try:
        from chimera_server import ServerConfig, run_server
    except ImportError as exc:
        _err(f"chimera_server.py nicht gefunden oder Flask fehlt: {exc}")
        _err("Bitte 'pip install flask' ausführen.")
        return 1

    cfg = ServerConfig(
        results_dir=args.results_dir,
        host=args.server_host,
        port=args.server_port,
        tls_cert=args.tls_cert,
        tls_key=args.tls_key,
        results_cache_ttl=args.cache_ttl,
        title=args.title,
        debug=args.debug,
    )

    if cfg.tls_cert and not cfg.tls_key:
        _err("--tls-cert angegeben, aber --tls-key fehlt.")
        return 1
    if cfg.tls_key and not cfg.tls_cert:
        _err("--tls-key angegeben, aber --tls-cert fehlt.")
        return 1

    scheme = "https" if cfg.tls_cert else "http"
    _info(f"Ergebnisverzeichnis: {args.results_dir}")
    _info(f"Lausche auf:         {scheme}://{cfg.host}:{cfg.port}")
    _info(f"Dashboard:           {scheme}://localhost:{cfg.port}/dashboard")
    _info(f"API:                 {scheme}://localhost:{cfg.port}/api/v1/status")
    if cfg.tls_cert:
        _ok(f"HTTPS aktiv  (Zertifikat: {cfg.tls_cert})")
    else:
        _warn("HTTP (kein TLS) – nur für lokale/interne Nutzung geeignet.")

    print()
    try:
        run_server(cfg)
    except FileNotFoundError as exc:
        _err(str(exc))
        return 1
    except KeyboardInterrupt:
        print()
        _ok("Server beendet.")
    return 0


# ---------------------------------------------------------------------------
# Einstiegspunkt
# ---------------------------------------------------------------------------


def main() -> int:
    parser = _build_parser()
    args = parser.parse_args()

    dispatch = {
        "run": _cmd_run,
        "endurance": _cmd_endurance,
        "export": _cmd_export,
        "status": _cmd_status,
        "validate": _cmd_validate,
        "connect": _cmd_connect,
        "list": _cmd_list,
        "serve": _cmd_serve,
    }

    handler = dispatch.get(args.command)
    if handler is None:
        parser.print_help()
        return 1
    return handler(args)


if __name__ == "__main__":
    sys.exit(main())
