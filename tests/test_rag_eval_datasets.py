#!/usr/bin/env python3
"""Regression tests for the checked-in RAG evaluation smoke fixtures."""

import json
import sys
import tempfile
import unittest
from pathlib import Path

REPOSITORY_ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(REPOSITORY_ROOT))

from benchmarks.rag.validate_eval_datasets import (
    DATASET_IDS,
    validate_dataset_directory,
)


DATASET_DIRECTORY = REPOSITORY_ROOT / "benchmarks" / "rag" / "eval_datasets_v1"


class RagEvaluationDatasetTests(unittest.TestCase):
    def test_checked_in_fixtures_are_valid(self):
        self.assertEqual(validate_dataset_directory(DATASET_DIRECTORY), [])

    def test_missing_dataset_is_reported(self):
        with tempfile.TemporaryDirectory() as temporary_directory:
            errors = validate_dataset_directory(Path(temporary_directory))
        self.assertEqual(len(errors), len(DATASET_IDS))
        self.assertTrue(all("required dataset file is missing" in error for error in errors))

    def test_relevant_document_must_exist_in_record(self):
        with tempfile.TemporaryDirectory() as temporary_directory:
            dataset_directory = Path(temporary_directory)
            for filename in DATASET_IDS:
                fixture = json.loads((DATASET_DIRECTORY / filename).read_text(encoding="utf-8"))
                if filename == "wikipedia_rag_2k.json":
                    fixture["records"][0]["relevant_document_ids"] = ["missing-document"]
                (dataset_directory / filename).write_text(
                    json.dumps(fixture),
                    encoding="utf-8",
                )
            errors = validate_dataset_directory(dataset_directory)

        self.assertTrue(any("relevant document 'missing-document' is absent" in e for e in errors))


if __name__ == "__main__":
    unittest.main()
