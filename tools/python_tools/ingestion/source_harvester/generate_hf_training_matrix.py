from __future__ import annotations

import argparse
import json
import logging
import os
import sys
from dataclasses import dataclass
from pathlib import Path
from typing import Any

import httpx
import yaml

if __package__ in {None, ""}:
    sys.path.insert(0, str(Path(__file__).resolve().parent.parent))
    from source_harvester.fetchers.huggingface import HuggingFaceDatasetFetcher
else:
    from .fetchers.huggingface import HuggingFaceDatasetFetcher

logger = logging.getLogger("hf-training-matrix")


@dataclass
class MatrixRow:
    dataset_id: str
    source_name: str
    priority_score: int
    use_case: str
    license: str
    gated: str
    size_category: str
    rows: str
    size_mb: str
    downloads: str
    likes: str
    risk: str
    notes: str


def _load_yaml(path: Path) -> dict[str, Any]:
    with path.open("r", encoding="utf-8") as handle:
        return yaml.safe_load(handle)


def _parse_size_payload(payload: dict[str, Any]) -> tuple[str, str, str]:
    size = payload.get("size", {}) if isinstance(payload.get("size", {}), dict) else {}
    dataset = size.get("dataset", {}) if isinstance(size.get("dataset", {}), dict) else {}
    rows = dataset.get("num_rows")
    bytes_parquet = dataset.get("num_bytes_parquet_files") or dataset.get("num_bytes_original_files") or dataset.get("num_bytes_memory")
    size_mb = "n/a"
    if isinstance(bytes_parquet, (int, float)):
        size_mb = f"{bytes_parquet / (1024 * 1024):.2f}"
    return (
        str(rows) if rows is not None else "n/a",
        size_mb,
        str(size.get("dataset", {}).get("num_bytes_original_files") or "n/a"),
    )


def _license_score(license_name: str, gated: str) -> int:
    normalized = (license_name or "").lower()
    if gated and gated != "false":
        return -8
    if normalized in {"mit", "apache-2.0", "bsd-3-clause", "bsd-2-clause"}:
        return 12
    if normalized.startswith("cc-by"):
        return 5
    if normalized:
        return 0
    return -3


def _size_score(size_category: str) -> int:
    text = (size_category or "").lower()
    if "100k<n<1m" in text:
        return 10
    if "1m<n<10m" in text:
        return 10
    if "10m<n<100m" in text:
        return 8
    if "100k" in text:
        return 8
    if "1b<n<10b" in text:
        return 6
    if text:
        return 4
    return 0


def _use_case_score(tags: list[str], dataset_id: str) -> tuple[int, str, str]:
    combined = " ".join(tags + [dataset_id]).lower()
    if any(token in combined for token in ["text-retrieval", "retrieval", "ms_marco", "beir", "fiqa", "scifact"]):
        return 30, "Retrieval/Rerank", "excellent for retriever/reranker tuning"
    if any(token in combined for token in ["humaneval", "mbpp", "apps", "ds-1000", "codegen", "programming"]):
        return 24, "Coding Eval / SFT", "strong for code instruction and evaluation"
    if any(token in combined for token in ["code-search", "code_search", "language:code", "text-generation"]):
        return 26, "Code Corpus", "good base corpus for code-grounded retrieval"
    return 10, "General", "supporting dataset"


def _risk_notes(license_name: str, gated: str, tags: list[str]) -> tuple[str, str]:
    risks = []
    notes = []
    if gated and gated != "false":
        risks.append("gated")
        notes.append("requires access approval")
    normalized = (license_name or "").lower()
    if normalized in {"other", "unknown", "no_license"}:
        risks.append("license-unclear")
        notes.append("license review needed")
    if any("text-generation" in tag for tag in tags):
        notes.append("usually better for SFT than eval")
    if not risks:
        risks.append("low")
    return ", ".join(risks), "; ".join(notes) if notes else "good default candidate"


def _priority_score(license_name: str, gated: str, size_category: str, tags: list[str], dataset_id: str, downloads: int | None, likes: int | None) -> int:
    base, _, _ = _use_case_score(tags, dataset_id)
    score = base
    score += _license_score(license_name, gated)
    score += _size_score(size_category)
    if downloads:
        score += 2 if downloads >= 1000 else 1
    if likes:
        score += 2 if likes >= 50 else 1
    if gated and gated != "false":
        score -= 3
    return max(score, 0)


def _collect_rows(config_path: Path, timeout: int = 30) -> list[dict[str, Any]]:
    cfg = _load_yaml(config_path)
    http_cfg = cfg.get("http", {})
    client = httpx.Client(
        timeout=timeout if timeout is None else int(http_cfg.get("timeout_seconds", timeout)),
        follow_redirects=True,
        headers={"User-Agent": str(http_cfg.get("user_agent", "ThemisDB-SourceHarvester/0.1"))},
    )
    rows: list[dict[str, Any]] = []
    try:
        for source in cfg.get("sources", []):
            if source.get("kind") != "huggingface_dataset_metadata" or not source.get("enabled", True):
                continue
            fetcher = HuggingFaceDatasetFetcher(
                source_name=str(source.get("name", "hf")),
                client=client,
                dataset_ids=list(source.get("dataset_ids", [])),
                search_queries=list(source.get("search_queries", [])),
                search_limit=int(source.get("search_limit", 10)),
            )
            for doc in fetcher.fetch():
                payload = json.loads(doc.content_clean)
                metadata = payload or {}
                size_meta = metadata.get("size", {}) if isinstance(metadata.get("size", {}), dict) else {}
                dataset_meta = metadata.get("dataset_id", doc.title)
                rows_count, size_mb, _ = _parse_size_payload(payload)
                tags = metadata.get("tags", []) or []
                license_name = str(metadata.get("license") or "")
                gated = str(metadata.get("gated") or "false")
                downloads = metadata.get("downloads")
                likes = metadata.get("likes")
                priority = _priority_score(license_name, gated, str(metadata.get("size_categories") or ""), tags, dataset_meta, downloads, likes)
                use_case_bonus, use_case, notes = _use_case_score(tags, dataset_meta)
                risk, risk_notes = _risk_notes(license_name, gated, tags)
                rows.append(
                    {
                        "dataset_id": dataset_meta,
                        "source_name": source.get("name", "hf"),
                        "priority_score": priority,
                        "use_case": use_case,
                        "license": license_name or "n/a",
                        "gated": gated,
                        "size_category": str(metadata.get("size_categories") or "n/a"),
                        "rows": rows_count,
                        "size_mb": size_mb,
                        "downloads": str(downloads if downloads is not None else "n/a"),
                        "likes": str(likes if likes is not None else "n/a"),
                        "risk": risk,
                        "notes": f"{notes}; {risk_notes}",
                    }
                )
    finally:
        client.close()
    rows.sort(key=lambda item: (-item["priority_score"], item["dataset_id"]))
    return rows


def _markdown_table(rows: list[dict[str, Any]]) -> str:
    headers = [
        "Dataset",
        "Priority",
        "Use Case",
        "License",
        "Gated",
        "Size Category",
        "Rows",
        "Size MB",
        "Downloads",
        "Likes",
        "Risk",
        "Notes",
    ]
    lines = ["# Hugging Face Training Matrix", "", "Prioritized from the configured HF shortlist and live metadata.", "", "| " + " | ".join(headers) + " |", "|" + "---|" * len(headers)]
    for row in rows:
        lines.append(
            "| " + " | ".join(
                [
                    row["dataset_id"],
                    str(row["priority_score"]),
                    row["use_case"],
                    row["license"],
                    row["gated"],
                    row["size_category"],
                    row["rows"],
                    row["size_mb"],
                    row["downloads"],
                    row["likes"],
                    row["risk"],
                    row["notes"],
                ]
            ) + " |"
        )
    return "\n".join(lines) + "\n"


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description="Generate a prioritized Hugging Face training matrix")
    parser.add_argument("--config", type=Path, required=True, help="Path to the Hugging Face shortlist YAML")
    parser.add_argument("--output-md", type=Path, required=True, help="Markdown output path")
    parser.add_argument("--output-json", type=Path, default=None, help="Optional JSON output path")
    parser.add_argument("--log-level", default="INFO", choices=["DEBUG", "INFO", "WARNING", "ERROR"], help="Log level")
    return parser.parse_args()


def main() -> int:
    args = parse_args()
    logging.basicConfig(level=getattr(logging, args.log_level), format="%(asctime)s %(levelname)s %(name)s %(message)s")
    rows = _collect_rows(args.config)
    args.output_md.parent.mkdir(parents=True, exist_ok=True)
    args.output_md.write_text(_markdown_table(rows), encoding="utf-8")
    if args.output_json:
        args.output_json.parent.mkdir(parents=True, exist_ok=True)
        args.output_json.write_text(json.dumps(rows, indent=2, ensure_ascii=False), encoding="utf-8")
    logger.info("Wrote %d matrix rows to %s", len(rows), args.output_md)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
