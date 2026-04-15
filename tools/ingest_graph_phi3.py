"""
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            ingest_graph_phi3.py                               ║
  Version:         0.0.7                                              ║
  Last Modified:   2026-04-15 05:58:25                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     132                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 3db23979ed  2026-04-06  feat: Enable HTTP server by default + LLM API routing + c... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
"""

#!/usr/bin/env python3
"""CLI wrapper around ingest_graph_phi3_lib with textual progress feedback."""

from __future__ import annotations

import argparse
import logging
import os
import sys
from typing import Any, Dict, List

from ingest_graph_phi3_lib import DEFAULT_EXTENSIONS, IngestionConfig, IngestionStats, run_ingestion


def parse_args(argv: List[str]) -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description="Ingest files into a ThemisDB test instance and build a graph using Phi3.",
    )
    parser.add_argument("--source", default=r"Y:\data", help="Source directory (default: Y:\\data)")
    parser.add_argument("--themis-url", default="http://127.0.0.1:8765", help="ThemisDB base URL")
    parser.add_argument("--bearer-token", default=os.getenv("THEMIS_BEARER_TOKEN", ""), help="JWT bearer token")
    parser.add_argument("--model-id", default="phi3", help="LLM model id")
    parser.add_argument("--model-path", default="", help="Optional model path for /api/v1/llm/models/load")
    parser.add_argument("--skip-model-load", action="store_true", help="Skip /api/v1/llm/models/load")
    parser.add_argument("--include-ext", nargs="*", default=sorted(DEFAULT_EXTENSIONS), help="Allowed file extensions")
    parser.add_argument("--max-file-size-mb", type=float, default=8.0, help="Skip files larger than this")
    parser.add_argument("--max-chars-per-file", type=int, default=12000, help="Text slice sent to LLM per file")
    parser.add_argument("--max-files", type=int, default=0, help="Optional limit of processed files (0 = unlimited)")
    parser.add_argument("--max-nodes-per-file", type=int, default=40, help="Extraction limit per file")
    parser.add_argument("--max-edges-per-file", type=int, default=80, help="Extraction limit per file")
    parser.add_argument("--llm-max-tokens", type=int, default=1600, help="max_tokens for /api/v1/llm/inference")
    parser.add_argument("--llm-temperature", type=float, default=0.1, help="temperature for /api/v1/llm/inference")
    parser.add_argument("--dry-run", action="store_true", help="Do not write to ThemisDB")
    parser.add_argument("--log-level", default="INFO", choices=["DEBUG", "INFO", "WARNING", "ERROR"], help="Log level")
    return parser.parse_args(argv)


def to_config(args: argparse.Namespace) -> IngestionConfig:
    include_ext = {ext.lower() if ext.startswith(".") else f".{ext.lower()}" for ext in args.include_ext}
    return IngestionConfig(
        source=args.source,
        themis_url=args.themis_url,
        bearer_token=args.bearer_token,
        model_id=args.model_id,
        model_path=args.model_path,
        skip_model_load=args.skip_model_load,
        include_ext=include_ext,
        max_file_size_mb=args.max_file_size_mb,
        max_chars_per_file=args.max_chars_per_file,
        max_files=args.max_files,
        max_nodes_per_file=args.max_nodes_per_file,
        max_edges_per_file=args.max_edges_per_file,
        llm_max_tokens=args.llm_max_tokens,
        llm_temperature=args.llm_temperature,
        dry_run=args.dry_run,
        timeout_sec=45.0,
    )


def cli_progress(event: str, data: Dict[str, Any], stats: IngestionStats) -> None:
    log = logging.getLogger("ingest-graph-phi3")
    if event == "scan_start":
        log.info("Scan started. candidates=%d", data.get("total_candidates", 0))
    elif event == "file_processed":
        log.info(
            "Processed %s (%d/%d, nodes=%d, edges=%d)",
            data.get("file", "?"),
            stats.scanned,
            max(stats.total_candidates, 1),
            stats.nodes,
            stats.edges,
        )
    elif event == "file_failed":
        log.warning("Failed %s: %s", data.get("file", "?"), data.get("error", ""))
    elif event == "done":
        log.info("Ingestion finished (dry_run=%s)", data.get("dry_run", False))


def cli_log(message: str) -> None:
    logging.getLogger("ingest-graph-phi3").info(message)


def main(argv: List[str]) -> int:
    args = parse_args(argv)
    logging.basicConfig(level=getattr(logging, args.log_level), format="%(asctime)s %(levelname)s %(message)s")
    log = logging.getLogger("ingest-graph-phi3")

    config = to_config(args)
    try:
        stats = run_ingestion(config, progress_cb=cli_progress, log_cb=cli_log)
    except Exception as ex:
        log.error("Ingestion failed: %s", ex)
        return 1

    log.info(
        "Done. scanned=%d processed=%d skipped=%d failed=%d nodes=%d edges=%d write_failed=%d elapsed=%.2fs",
        stats.scanned,
        stats.processed,
        stats.skipped,
        stats.failed,
        stats.nodes,
        stats.edges,
        stats.write_failed,
        stats.elapsed_sec,
    )
    return 0 if stats.write_failed == 0 else 5


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
