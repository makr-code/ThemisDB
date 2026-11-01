#!/usr/bin/env python3
"""Validate the checked-in RAG evaluation smoke-fixture files."""

from __future__ import annotations

import argparse
import json
from pathlib import Path
from typing import Any


DATASET_IDS = {
    "wikipedia_rag_2k.json": "wikipedia_rag_2k_v1",
    "code_rag_1k.json": "code_rag_1k_v1",
    "multihop_qa_500.json": "multihop_qa_500_v1",
    "crosslingual_500.json": "crosslingual_ir_500_v1",
}


def _validate_dataset(path: Path, expected_id: str) -> list[str]:
    errors: list[str] = []
    try:
        dataset: Any = json.loads(path.read_text(encoding="utf-8"))
    except (OSError, json.JSONDecodeError) as exc:
        return [f"{path}: cannot read valid JSON: {exc}"]

    if not isinstance(dataset, dict):
        return [f"{path}: top-level JSON value must be an object"]
    if dataset.get("schema_version") != 1:
        errors.append(f"{path}: schema_version must be 1")
    if dataset.get("dataset_id") != expected_id:
        errors.append(f"{path}: dataset_id must be {expected_id!r}")
    if dataset.get("data_kind") != "synthetic_smoke_fixture":
        errors.append(f"{path}: data_kind must identify a synthetic smoke fixture")
    if not isinstance(dataset.get("provenance"), str) or not dataset["provenance"].strip():
        errors.append(f"{path}: provenance must be a non-empty string")

    records = dataset.get("records")
    if not isinstance(records, list) or not records:
        return errors + [f"{path}: records must be a non-empty array"]

    record_ids: set[str] = set()
    for index, record in enumerate(records):
        label = f"{path}: records[{index}]"
        if not isinstance(record, dict):
            errors.append(f"{label} must be an object")
            continue

        record_id = record.get("id")
        if not isinstance(record_id, str) or not record_id.strip():
            errors.append(f"{label}.id must be a non-empty string")
        elif record_id in record_ids:
            errors.append(f"{label}.id duplicates {record_id!r}")
        else:
            record_ids.add(record_id)

        if not isinstance(record.get("query"), str) or not record["query"].strip():
            errors.append(f"{label}.query must be a non-empty string")

        documents = record.get("documents")
        if not isinstance(documents, list) or not documents:
            errors.append(f"{label}.documents must be a non-empty array")
            continue

        document_ids: set[str] = set()
        for doc_index, document in enumerate(documents):
            doc_label = f"{label}.documents[{doc_index}]"
            if not isinstance(document, dict):
                errors.append(f"{doc_label} must be an object")
                continue
            doc_id = document.get("id")
            if not isinstance(doc_id, str) or not doc_id.strip():
                errors.append(f"{doc_label}.id must be a non-empty string")
            elif doc_id in document_ids:
                errors.append(f"{doc_label}.id duplicates {doc_id!r}")
            else:
                document_ids.add(doc_id)
            if not isinstance(document.get("text"), str) or not document["text"].strip():
                errors.append(f"{doc_label}.text must be a non-empty string")

        relevant = record.get("relevant_document_ids")
        if not isinstance(relevant, list) or not relevant:
            errors.append(f"{label}.relevant_document_ids must be a non-empty array")
            continue
        if any(not isinstance(doc_id, str) for doc_id in relevant):
            errors.append(f"{label}.relevant_document_ids must contain strings")
            continue
        if len(set(relevant)) != len(relevant):
            errors.append(f"{label}.relevant_document_ids must be unique")
        for doc_id in relevant:
            if doc_id not in document_ids:
                errors.append(f"{label}: relevant document {doc_id!r} is absent from documents")

    return errors


def validate_dataset_directory(dataset_dir: Path) -> list[str]:
    errors: list[str] = []
    for filename, dataset_id in DATASET_IDS.items():
        path = dataset_dir / filename
        if not path.is_file():
            errors.append(f"{path}: required dataset file is missing")
            continue
        errors.extend(_validate_dataset(path, dataset_id))
    return errors


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument(
        "--dataset-dir",
        type=Path,
        default=Path(__file__).parent / "eval_datasets_v1",
        help="Directory containing the four version-1 JSON fixtures.",
    )
    args = parser.parse_args()

    errors = validate_dataset_directory(args.dataset_dir)
    if errors:
        for error in errors:
            print(f"ERROR: {error}")
        return 1

    print(f"Validated {len(DATASET_IDS)} RAG evaluation smoke fixtures in {args.dataset_dir}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
