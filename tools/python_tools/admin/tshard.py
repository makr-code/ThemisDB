#!/usr/bin/env python3
"""
ThemisDB Shard Helper (8.3 name: tshard.py)

Dual-mode tool:
- UI/TUI mode (default): interactive prompts
- CLI mode (fallback): pass --action and options

Actions:
- status: show shard summary from a cluster-state JSON file
- rebalance: calculate a dry-run migration plan between shards
"""

from __future__ import annotations

import argparse
import csv
import json
import sys
from dataclasses import dataclass
from pathlib import Path
from typing import Any, Dict, List, Optional

DEFAULT_STATE_FILE = Path("./config/cluster_state.json")


@dataclass
class Shard:
    shard_id: str
    node_id: str
    load: float
    data_gb: float


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description="ThemisDB shard helper (dual-mode UI + CLI)",
        formatter_class=argparse.RawTextHelpFormatter,
        epilog=(
            "Examples:\n"
            "  python tshard.py\n"
            "  python tshard.py --action status --state-file ./config/cluster_state.json --format text\n"
            "  python tshard.py --action rebalance --state-file ./config/cluster_state.json --from shard-a --to shard-b --percent 15"
        ),
    )

    parser.add_argument("--action", choices=["status", "rebalance"], help="Action to run in CLI mode")
    parser.add_argument("--state-file", type=Path, default=DEFAULT_STATE_FILE, help="Cluster state JSON file")
    parser.add_argument("--from", dest="from_shard", type=str, help="Source shard id for rebalance")
    parser.add_argument("--to", dest="to_shard", type=str, help="Target shard id for rebalance")
    parser.add_argument("--percent", type=float, default=10.0, help="Data percentage to move (0-100)")
    parser.add_argument("--format", choices=["json", "csv", "text"], default="json", help="Output format")
    parser.add_argument("--output", type=Path, help="Optional output file")
    parser.add_argument("--headless", action="store_true", help="Force non-interactive mode")
    parser.add_argument("--verbose", "-v", action="store_true", help="Verbose output")

    return parser.parse_args()


def load_shards(path: Path) -> List[Shard]:
    if not path.exists() or not path.is_file():
        raise FileNotFoundError(f"state file not found: {path}")

    raw = json.loads(path.read_text(encoding="utf-8"))
    items = raw.get("shards", []) if isinstance(raw, dict) else []
    shards: List[Shard] = []

    for item in items:
        if not isinstance(item, dict):
            continue
        shard_id = str(item.get("id", "")).strip()
        node_id = str(item.get("node", "")).strip()
        if not shard_id:
            continue
        shards.append(
            Shard(
                shard_id=shard_id,
                node_id=node_id or "unknown",
                load=float(item.get("load", 0.0)),
                data_gb=float(item.get("data_gb", 0.0)),
            )
        )

    return shards


def shard_status(shards: List[Shard]) -> Dict[str, Any]:
    total_data = sum(x.data_gb for x in shards)
    avg_load = (sum(x.load for x in shards) / len(shards)) if shards else 0.0
    hottest = max(shards, key=lambda s: s.load).shard_id if shards else ""
    coldest = min(shards, key=lambda s: s.load).shard_id if shards else ""

    return {
        "status": "success",
        "shard_count": len(shards),
        "total_data_gb": round(total_data, 3),
        "avg_load": round(avg_load, 3),
        "hottest_shard": hottest,
        "coldest_shard": coldest,
        "shards": [
            {
                "id": s.shard_id,
                "node": s.node_id,
                "load": s.load,
                "data_gb": s.data_gb,
            }
            for s in sorted(shards, key=lambda x: x.shard_id)
        ],
    }


def rebalance_plan(shards: List[Shard], from_shard: str, to_shard: str, percent: float) -> Dict[str, Any]:
    if percent <= 0 or percent > 100:
        raise ValueError("--percent must be in range (0,100]")

    src = next((s for s in shards if s.shard_id == from_shard), None)
    dst = next((s for s in shards if s.shard_id == to_shard), None)
    if src is None:
        raise ValueError(f"source shard not found: {from_shard}")
    if dst is None:
        raise ValueError(f"target shard not found: {to_shard}")

    move_gb = round(src.data_gb * (percent / 100.0), 3)
    src_after_data = round(src.data_gb - move_gb, 3)
    dst_after_data = round(dst.data_gb + move_gb, 3)

    # Simple load re-estimation proportional to moved data fraction.
    src_after_load = round(max(0.0, src.load * (1.0 - percent / 100.0)), 3)
    dst_after_load = round(dst.load + (src.load - src_after_load) * 0.8, 3)

    return {
        "status": "success",
        "mode": "dry-run",
        "action": "rebalance",
        "from": src.shard_id,
        "to": dst.shard_id,
        "percent": percent,
        "estimated_move_gb": move_gb,
        "before": {
            "from_data_gb": src.data_gb,
            "to_data_gb": dst.data_gb,
            "from_load": src.load,
            "to_load": dst.load,
        },
        "after": {
            "from_data_gb": src_after_data,
            "to_data_gb": dst_after_data,
            "from_load": src_after_load,
            "to_load": dst_after_load,
        },
    }


def emit(payload: Dict[str, Any], fmt: str, output: Optional[Path]) -> int:
    if fmt == "json":
        text = json.dumps(payload, indent=2, ensure_ascii=True)
        if output:
            output.write_text(text, encoding="utf-8")
        else:
            print(text)
        return 0

    if fmt == "csv":
        if output:
            fh = output.open("w", newline="", encoding="utf-8")
            should_close = True
        else:
            fh = sys.stdout
            should_close = False
        try:
            if payload.get("action") == "rebalance":
                rows = [
                    {"field": "from", "value": payload.get("from", "")},
                    {"field": "to", "value": payload.get("to", "")},
                    {"field": "percent", "value": payload.get("percent", "")},
                    {"field": "estimated_move_gb", "value": payload.get("estimated_move_gb", "")},
                ]
                writer = csv.DictWriter(fh, fieldnames=["field", "value"])
                writer.writeheader()
                writer.writerows(rows)
            else:
                writer = csv.DictWriter(fh, fieldnames=["id", "node", "load", "data_gb"])
                writer.writeheader()
                writer.writerows(payload.get("shards", []))
        finally:
            if should_close:
                fh.close()
        return 0

    # text
    if payload.get("action") == "rebalance":
        text = (
            f"Dry-run rebalance: {payload.get('from')} -> {payload.get('to')}\n"
            f"Move: {payload.get('percent')}% ({payload.get('estimated_move_gb')} GB)\n"
            f"Before: from_load={payload.get('before', {}).get('from_load')} to_load={payload.get('before', {}).get('to_load')}\n"
            f"After:  from_load={payload.get('after', {}).get('from_load')} to_load={payload.get('after', {}).get('to_load')}"
        )
    else:
        lines = [
            f"Shards: {payload.get('shard_count')} total_data_gb={payload.get('total_data_gb')} avg_load={payload.get('avg_load')}",
            f"Hottest: {payload.get('hottest_shard')} | Coldest: {payload.get('coldest_shard')}",
        ]
        for item in payload.get("shards", []):
            lines.append(f"- {item.get('id')} node={item.get('node')} load={item.get('load')} data_gb={item.get('data_gb')}")
        text = "\n".join(lines)

    if output:
        output.write_text(text + "\n", encoding="utf-8")
    else:
        print(text)
    return 0


def ask(prompt: str, default: str = "") -> str:
    suffix = f" [{default}]" if default else ""
    value = input(f"{prompt}{suffix}: ").strip()
    return value if value else default


def interactive_mode() -> int:
    print("ThemisDB Shard Helper (tshard)")
    print("=" * 29)
    print("1) Show shard status")
    print("2) Rebalance dry-run")
    print("3) Exit")

    choice = ask("Choose action", "1")
    if choice == "3":
        return 0

    state_file = Path(ask("State file", str(DEFAULT_STATE_FILE)))
    out_fmt = ask("Output format (json/csv/text)", "text").lower()
    out_file_raw = ask("Output file (empty=stdout)", "")
    out_file = Path(out_file_raw) if out_file_raw else None

    try:
        shards = load_shards(state_file)
    except Exception as exc:
        print(f"ERROR: {exc}", file=sys.stderr)
        return 4

    try:
        if choice == "1":
            payload = shard_status(shards)
            return emit(payload, out_fmt, out_file)

        if choice == "2":
            src = ask("From shard id", "")
            dst = ask("To shard id", "")
            percent = float(ask("Percent to move", "10"))
            payload = rebalance_plan(shards, src, dst, percent)
            return emit(payload, out_fmt, out_file)

        print("ERROR: invalid selection", file=sys.stderr)
        return 2
    except Exception as exc:
        print(f"ERROR: {exc}", file=sys.stderr)
        return 4


def main() -> int:
    args = parse_args()

    if args.action is None and not args.headless:
        return interactive_mode()

    try:
        shards = load_shards(args.state_file)
    except Exception as exc:
        print(f"ERROR: {exc}", file=sys.stderr)
        return 4

    try:
        if args.action == "status":
            payload = shard_status(shards)
            return emit(payload, args.format, args.output)

        if args.action == "rebalance":
            if not args.from_shard or not args.to_shard:
                print("ERROR: --from and --to are required for --action rebalance", file=sys.stderr)
                return 2
            payload = rebalance_plan(shards, args.from_shard, args.to_shard, args.percent)
            return emit(payload, args.format, args.output)

        print("ERROR: --action is required in CLI mode", file=sys.stderr)
        return 2
    except ValueError as exc:
        print(f"ERROR: {exc}", file=sys.stderr)
        return 2
    except Exception as exc:
        print(f"ERROR: {exc}", file=sys.stderr)
        return 4


if __name__ == "__main__":
    sys.exit(main())
