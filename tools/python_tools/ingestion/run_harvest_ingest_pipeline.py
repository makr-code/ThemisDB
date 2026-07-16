from __future__ import annotations

import argparse
import hashlib
import json
import logging
import shutil
import sys
from pathlib import Path
from typing import Any

import yaml

THIS_DIR = Path(__file__).resolve().parent
sys.path.insert(0, str(THIS_DIR))

from ingest import IngestionConfig, IngestionEngine  # type: ignore
from source_harvester.cli import _to_source_configs  # type: ignore
from source_harvester.pipeline import HarvesterPipeline  # type: ignore
from source_harvester.storage import HarvestStateStore, JsonlSink  # type: ignore

logger = logging.getLogger("harvest-ingest-pipeline")


def _load_yaml(path: Path) -> dict[str, Any]:
    with path.open("r", encoding="utf-8") as handle:
        return yaml.safe_load(handle)


def _safe_slug(text: str) -> str:
    sanitized = "".join(ch if ch.isalnum() or ch in {"-", "_"} else "-" for ch in text.lower())
    sanitized = "-".join(part for part in sanitized.split("-") if part)
    return sanitized[:80] or "document"


def _materialize_jsonl_to_stage(jsonl_path: Path, stage_dir: Path) -> int:
    stage_dir.mkdir(parents=True, exist_ok=True)
    count = 0
    with jsonl_path.open("r", encoding="utf-8") as handle:
        for line in handle:
            if not line.strip():
                continue
            payload = json.loads(line)
            title = str(payload.get("title") or "untitled")
            url = str(payload.get("url") or title)
            digest = hashlib.sha256(url.encode("utf-8", errors="ignore")).hexdigest()[:12]
            filename = f"{count:06d}-{_safe_slug(title)}-{digest}.json"
            out_path = stage_dir / filename
            doc = {
                "source": payload.get("source"),
                "url": payload.get("url"),
                "title": payload.get("title"),
                "content": payload.get("content_clean"),
                "content_raw": payload.get("content_raw"),
                "metadata": payload.get("metadata", {}),
                "fetched_at": payload.get("fetched_at"),
                "ingested_at": payload.get("ingested_at"),
                "content_sha256": payload.get("content_sha256"),
            }
            with out_path.open("w", encoding="utf-8") as out_handle:
                json.dump(doc, out_handle, ensure_ascii=False, indent=2)
            count += 1
    return count


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description="Run source harvester and feed its output into the existing ingestion engine")
    parser.add_argument("--harvester-config", type=Path, required=True, help="Path to source harvester YAML config")
    parser.add_argument("--stage-dir", type=Path, required=True, help="Directory used for staged harvested JSON files")
    parser.add_argument("--output", type=Path, default=Path("harvest_ingestion_output.json"), help="Final ingestion output JSON")
    parser.add_argument("--db", type=Path, default=Path("harvest_ingestion_tracker.db"), help="SQLite tracker for ingestion engine")
    parser.add_argument("--clean-stage", action="store_true", help="Delete and recreate the stage directory before materializing documents")
    parser.add_argument("--log-level", default="INFO", choices=["DEBUG", "INFO", "WARNING", "ERROR"], help="Log level")
    return parser.parse_args()


def main() -> int:
    args = parse_args()
    logging.basicConfig(level=getattr(logging, args.log_level), format="%(asctime)s %(levelname)s %(name)s %(message)s")

    harvester_cfg = _load_yaml(args.harvester_config)
    storage_cfg = harvester_cfg.get("storage", {})
    http_cfg = harvester_cfg.get("http", {})
    output_jsonl = Path(str(storage_cfg.get("output_jsonl", ".harvester_documents.jsonl")))
    sqlite_path = str(storage_cfg.get("sqlite_path", ".harvester_state.db"))

    state_store = HarvestStateStore(sqlite_path)
    sink = JsonlSink(str(output_jsonl))
    pipeline = HarvesterPipeline(
        state_store=state_store,
        sink=sink,
        timeout_seconds=int(http_cfg.get("timeout_seconds", 20)),
        user_agent=str(http_cfg.get("user_agent", "ThemisDB-SourceHarvester/0.1")),
        max_retries=int(http_cfg.get("max_retries", 3)),
        retry_backoff_seconds=float(http_cfg.get("retry_backoff_seconds", 1.5)),
    )

    try:
        source_configs = _to_source_configs(harvester_cfg.get("sources", []))
        harvest_result = pipeline.run(source_configs)
        logger.info("Harvest complete: fetched=%d written=%d", harvest_result["fetched"], harvest_result["written"])
    finally:
        pipeline.close()

    if args.clean_stage and args.stage_dir.exists():
        shutil.rmtree(args.stage_dir)
    materialized_count = _materialize_jsonl_to_stage(output_jsonl, args.stage_dir)
    logger.info("Materialized %d harvested documents into %s", materialized_count, args.stage_dir)

    ingest_config = IngestionConfig(
        source_dir=str(args.stage_dir),
        output_file=str(args.output),
        db_path=str(args.db),
        include_extensions=[".json"],
        exclude_extensions=[],
        exclude_patterns=[".git", "__pycache__", ".venv"],
        max_file_size_mb=100.0,
        generate_vector_metadata=True,
        generate_graph_metadata=True,
        generate_relational_metadata=True,
    )
    engine = IngestionEngine(ingest_config)
    try:
        result = engine.ingest()
        logger.info("Ingestion complete: processed=%s", result.get("statistics", {}).get("files_processed"))
    finally:
        engine.tracker.close()

    return 0


if __name__ == "__main__":
    raise SystemExit(main())
