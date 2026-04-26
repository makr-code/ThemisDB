#!/usr/bin/env python3
"""
ThemisDB Log Query Helper (8.3 name: tlogqry.py)

Dual-mode tool:
- UI/TUI mode (default): interactive prompts
- CLI mode (fallback): pass --action and options
"""

from __future__ import annotations

import argparse
import csv
import datetime as dt
import json
import re
import sys
from dataclasses import dataclass
from pathlib import Path
from typing import Any, Dict, Iterable, List, Optional

DEFAULT_LOG_DIR = Path("./logs")


@dataclass
class QueryConfig:
    log_dir: Path
    level: Optional[str]
    pattern: Optional[str]
    since_minutes: Optional[int]
    limit: int
    output_format: str
    output_file: Optional[Path]


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description="ThemisDB log query helper (dual-mode UI + CLI)",
        formatter_class=argparse.RawTextHelpFormatter,
        epilog=(
            "Examples:\n"
            "  python tlogqry.py\n"
            "  python tlogqry.py --action query --log-dir ./logs --level ERROR --limit 50\n"
            "  python tlogqry.py --action query --pattern timeout --since-minutes 120 --format csv --output out.csv"
        ),
    )
    parser.add_argument("--action", choices=["query"], help="Action to run (CLI mode)")
    parser.add_argument("--log-dir", type=Path, default=DEFAULT_LOG_DIR, help="Directory with .log/.jsonl files")
    parser.add_argument("--level", type=str, help="Filter by level, e.g. ERROR, WARN, INFO")
    parser.add_argument("--pattern", type=str, help="Regex pattern for message text")
    parser.add_argument("--since-minutes", type=int, help="Only include entries newer than N minutes")
    parser.add_argument("--limit", type=int, default=100, help="Maximum number of results (default: 100)")
    parser.add_argument("--format", choices=["json", "csv", "text"], default="json", help="Output format")
    parser.add_argument("--output", type=Path, help="Optional output file path")
    parser.add_argument("--headless", action="store_true", help="Force non-interactive mode")
    parser.add_argument("--verbose", "-v", action="store_true", help="Verbose logging to stderr")
    return parser.parse_args()


def load_records(log_dir: Path) -> Iterable[Dict[str, Any]]:
    if not log_dir.exists() or not log_dir.is_dir():
        return []

    records: List[Dict[str, Any]] = []
    for path in sorted(log_dir.glob("*")):
        if path.suffix.lower() not in {".jsonl", ".log", ".json"}:
            continue

        try:
            with path.open("r", encoding="utf-8", errors="replace") as fh:
                for line_no, line in enumerate(fh, start=1):
                    line = line.strip()
                    if not line:
                        continue
                    rec = parse_line(line, path.name, line_no)
                    if rec is not None:
                        records.append(rec)
        except OSError:
            continue

    return records


def parse_line(line: str, source: str, line_no: int) -> Optional[Dict[str, Any]]:
    try:
        obj = json.loads(line)
        if isinstance(obj, dict):
            return normalize_record(obj, source, line_no)
    except json.JSONDecodeError:
        pass

    # Fallback parser for plaintext lines.
    # Expected loose format: "<timestamp> <level> message"
    m = re.match(r"^(?P<ts>\S+)\s+(?P<lvl>[A-Za-z]+)\s+(?P<msg>.*)$", line)
    if m:
        return {
            "timestamp": m.group("ts"),
            "level": m.group("lvl").upper(),
            "message": m.group("msg"),
            "component": "unknown",
            "source": source,
            "line": line_no,
        }

    return {
        "timestamp": "",
        "level": "UNKNOWN",
        "message": line,
        "component": "unknown",
        "source": source,
        "line": line_no,
    }


def normalize_record(obj: Dict[str, Any], source: str, line_no: int) -> Dict[str, Any]:
    ts = str(obj.get("timestamp", obj.get("time", "")))
    lvl = str(obj.get("level", obj.get("severity", "UNKNOWN"))).upper()
    msg = str(obj.get("message", obj.get("msg", "")))
    component = str(obj.get("component", obj.get("module", "unknown")))

    return {
        "timestamp": ts,
        "level": lvl,
        "message": msg,
        "component": component,
        "source": source,
        "line": line_no,
    }


def parse_timestamp(value: str) -> Optional[dt.datetime]:
    if not value:
        return None
    value = value.replace("Z", "+00:00")
    for fmt in (None, "%Y-%m-%d %H:%M:%S", "%Y-%m-%dT%H:%M:%S"):
        try:
            if fmt is None:
                return dt.datetime.fromisoformat(value)
            return dt.datetime.strptime(value, fmt)
        except ValueError:
            continue
    return None


def filter_records(records: Iterable[Dict[str, Any]], cfg: QueryConfig) -> List[Dict[str, Any]]:
    filtered: List[Dict[str, Any]] = []
    now_utc = dt.datetime.now(dt.timezone.utc)

    regex = re.compile(cfg.pattern, re.IGNORECASE) if cfg.pattern else None
    level_filter = cfg.level.upper() if cfg.level else None

    for rec in records:
        if level_filter and rec.get("level", "").upper() != level_filter:
            continue

        if regex and not regex.search(rec.get("message", "")):
            continue

        if cfg.since_minutes is not None:
            ts = parse_timestamp(str(rec.get("timestamp", "")))
            if ts is None:
                continue
            if ts.tzinfo is None:
                ts = ts.replace(tzinfo=dt.timezone.utc)
            age = now_utc - ts.astimezone(dt.timezone.utc)
            if age.total_seconds() > cfg.since_minutes * 60:
                continue

        filtered.append(rec)
        if len(filtered) >= cfg.limit:
            break

    return filtered


def write_output(records: List[Dict[str, Any]], cfg: QueryConfig) -> int:
    if cfg.output_format == "json":
        payload = {
            "status": "success",
            "count": len(records),
            "data": records,
        }
        text = json.dumps(payload, indent=2, ensure_ascii=True)
        if cfg.output_file:
            cfg.output_file.write_text(text, encoding="utf-8")
        else:
            print(text)
        return 0

    if cfg.output_format == "csv":
        headers = ["timestamp", "level", "component", "message", "source", "line"]
        if cfg.output_file:
            with cfg.output_file.open("w", newline="", encoding="utf-8") as fh:
                writer = csv.DictWriter(fh, fieldnames=headers)
                writer.writeheader()
                writer.writerows(records)
        else:
            writer = csv.DictWriter(sys.stdout, fieldnames=headers)
            writer.writeheader()
            writer.writerows(records)
        return 0

    # text format
    lines = []
    for rec in records:
        lines.append(
            f"[{rec.get('level','UNKNOWN')}] {rec.get('timestamp','')} "
            f"{rec.get('component','unknown')}: {rec.get('message','')} "
            f"({rec.get('source','?')}:{rec.get('line','?')})"
        )
    out = "\n".join(lines) if lines else "No records matched."
    if cfg.output_file:
        cfg.output_file.write_text(out + "\n", encoding="utf-8")
    else:
        print(out)
    return 0


def run_query(cfg: QueryConfig, verbose: bool = False) -> int:
    if cfg.limit <= 0:
        print("ERROR: --limit must be > 0", file=sys.stderr)
        return 2

    if verbose:
        print(f"Reading logs from: {cfg.log_dir}", file=sys.stderr)

    all_records = load_records(cfg.log_dir)
    results = filter_records(all_records, cfg)

    if verbose:
        print(f"Matched records: {len(results)}", file=sys.stderr)

    try:
        return write_output(results, cfg)
    except OSError as exc:
        print(f"ERROR: Could not write output: {exc}", file=sys.stderr)
        return 4


def ask(prompt: str, default: str = "") -> str:
    suffix = f" [{default}]" if default else ""
    value = input(f"{prompt}{suffix}: ").strip()
    return value if value else default


def interactive_mode() -> int:
    print("ThemisDB Log Query Helper (tlogqry)")
    print("=" * 38)

    log_dir = Path(ask("Log directory", str(DEFAULT_LOG_DIR)))
    level = ask("Level filter (empty=all)", "").upper() or None
    pattern = ask("Regex pattern (empty=none)", "") or None
    since_raw = ask("Since minutes (empty=all)", "")
    limit_raw = ask("Max results", "100")
    out_fmt = ask("Output format (json/csv/text)", "text").lower()
    out_file_raw = ask("Output file (empty=stdout)", "")

    since_minutes = int(since_raw) if since_raw else None
    limit = int(limit_raw)
    output_file = Path(out_file_raw) if out_file_raw else None

    cfg = QueryConfig(
        log_dir=log_dir,
        level=level,
        pattern=pattern,
        since_minutes=since_minutes,
        limit=limit,
        output_format=out_fmt if out_fmt in {"json", "csv", "text"} else "text",
        output_file=output_file,
    )
    return run_query(cfg, verbose=False)


def main() -> int:
    args = parse_args()

    if args.action is None and not args.headless:
        return interactive_mode()

    if args.action != "query":
        print("ERROR: --action query is required in CLI mode", file=sys.stderr)
        return 2

    cfg = QueryConfig(
        log_dir=args.log_dir,
        level=args.level,
        pattern=args.pattern,
        since_minutes=args.since_minutes,
        limit=args.limit,
        output_format=args.format,
        output_file=args.output,
    )
    return run_query(cfg, verbose=args.verbose)


if __name__ == "__main__":
    sys.exit(main())
