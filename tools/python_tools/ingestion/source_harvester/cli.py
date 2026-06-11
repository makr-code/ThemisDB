from __future__ import annotations

import argparse
import logging
from pathlib import Path
import sys
from typing import List

import yaml

if __package__ in {None, ""}:
    sys.path.insert(0, str(Path(__file__).resolve().parent.parent))
    from source_harvester.models import SourceConfig
    from source_harvester.pipeline import HarvesterPipeline
    from source_harvester.storage import HarvestStateStore, JsonlSink
else:
    from .models import SourceConfig
    from .pipeline import HarvesterPipeline
    from .storage import HarvestStateStore, JsonlSink


def _load_config(path: Path):
    with path.open("r", encoding="utf-8") as handle:
        return yaml.safe_load(handle)


def _to_source_configs(raw_sources: list[dict]) -> List[SourceConfig]:
    sources: List[SourceConfig] = []
    for item in raw_sources:
        name = item.get("name", "unnamed")
        kind = item.get("kind", "")
        enabled = bool(item.get("enabled", True))
        options = {k: v for k, v in item.items() if k not in {"name", "kind", "enabled"}}
        sources.append(SourceConfig(name=name, kind=kind, enabled=enabled, options=options))
    return sources


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description="ThemisDB source harvester")
    parser.add_argument(
        "--config",
        type=Path,
        required=True,
        help="Path to YAML config file",
    )
    parser.add_argument(
        "--log-level",
        default="INFO",
        choices=["DEBUG", "INFO", "WARNING", "ERROR"],
        help="Log level",
    )
    return parser.parse_args()


def main() -> int:
    args = parse_args()
    logging.basicConfig(
        level=getattr(logging, args.log_level),
        format="%(asctime)s %(levelname)s %(name)s %(message)s",
    )

    cfg = _load_config(args.config)
    storage_cfg = cfg.get("storage", {})
    http_cfg = cfg.get("http", {})

    sqlite_path = str(storage_cfg.get("sqlite_path", ".harvester_state.db"))
    output_jsonl = str(storage_cfg.get("output_jsonl", ".harvester_documents.jsonl"))

    state_store = HarvestStateStore(sqlite_path)
    sink = JsonlSink(output_jsonl)
    pipeline = HarvesterPipeline(
        state_store=state_store,
        sink=sink,
        timeout_seconds=int(http_cfg.get("timeout_seconds", 20)),
        user_agent=str(http_cfg.get("user_agent", "ThemisDB-SourceHarvester/0.1")),
        max_retries=int(http_cfg.get("max_retries", 3)),
        retry_backoff_seconds=float(http_cfg.get("retry_backoff_seconds", 1.5)),
    )

    try:
        sources = _to_source_configs(cfg.get("sources", []))
        result = pipeline.run(sources)
        print(f"Fetched: {result['fetched']} | Written: {result['written']}")
        return 0
    finally:
        pipeline.close()


if __name__ == "__main__":
    raise SystemExit(main())
