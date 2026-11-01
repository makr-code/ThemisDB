import importlib.util
import json
import sys
import tempfile
import unittest
from pathlib import Path


_SCRIPT = Path(__file__).parent.parent / "scripts" / "ai-dev-llm-wiki-sync.py"


def _load_module():
    spec = importlib.util.spec_from_file_location("ai_dev_llm_wiki_sync", _SCRIPT)
    module = importlib.util.module_from_spec(spec)
    assert spec and spec.loader
    sys.modules[spec.name] = module
    spec.loader.exec_module(module)
    return module


class CuratedModuleIndexTests(unittest.TestCase):
    def test_curated_module_contexts_are_kept_in_generated_indexes(self):
        wiki_sync = _load_module()

        index = wiki_sync.render_index([], "2026-10-31T00:00:00+00:00", False, "digest")
        modules = wiki_sync.render_modules_and_apis(Path(__file__).parent.parent, [])

        self.assertIn("[IMPORTERS.md](IMPORTERS.md)", index)
        self.assertIn("[UPDATES.md](UPDATES.md)", index)
        self.assertIn("[PROCESS.md](PROCESS.md)", index)
        self.assertIn("[AQL module context](AQL.md)", index)
        self.assertIn("[Graph module context](GRAPH.md)", index)
        self.assertIn("[Ethics AI module context](ETHICS_AI.md)", index)
        self.assertIn("must not redefine project status", index)
        self.assertIn("[AQL module context](AQL.md)", modules)
        self.assertIn("[Graph module context](GRAPH.md)", modules)
        self.assertIn("[Ethics AI module context](ETHICS_AI.md)", modules)
        self.assertIn("[Importer module context](IMPORTERS.md)", modules)
        self.assertIn("[Updates module context](UPDATES.md)", modules)
        self.assertIn("[Process module context](PROCESS.md)", modules)
        self.assertIn("[Ethics AI module context](ETHICS_AI.md)", modules)
        self.assertIn("[DISTRIBUTED_TENSOR.md](DISTRIBUTED_TENSOR.md)", index)
        self.assertIn("[MAINTENANCE.md](MAINTENANCE.md)", index)
        self.assertIn("[Maintenance module context](MAINTENANCE.md)", modules)
        self.assertIn("AI_Used: true", index)
        self.assertIn("[Distributed tensor module context](DISTRIBUTED_TENSOR.md)", modules)
        self.assertIn("[Temporal module context](TEMPORAL.md)", modules)
        self.assertIn("[Storage module context](STORAGE.md)", modules)
        self.assertIn("[Encrypted user storage module context](USER_STORAGE_ENCRYPTED.md)", modules)
        self.assertIn("AI_Used: true", modules)

    def test_previous_source_manifest_is_used_for_delta(self):
        wiki_sync = _load_module()

        with tempfile.TemporaryDirectory() as temp_dir:
            status_path = Path(temp_dir) / "WIKI_STATUS.json"
            status_path.write_text('{"source_count": 1}', encoding="utf-8")
            (Path(temp_dir) / "SOURCE_MANIFEST.json").write_text(
                json.dumps(
                    {
                        "sources": [
                            {"path": "src/updates/README.md", "sha256": "old-hash"}
                        ]
                    }
                ),
                encoding="utf-8",
            )

            previous = wiki_sync.load_previous_status(status_path)
            delta = wiki_sync.build_delta(
                previous,
                [
                    wiki_sync.SourceEntry(
                        path="src/updates/README.md",
                        category="modules_and_api",
                        size=1,
                        sha256="new-hash",
                        modified_utc="2026-10-31T00:00:00+00:00",
                    ),
                    wiki_sync.SourceEntry(
                        path="src/updates/API.md",
                        category="modules_and_api",
                        size=1,
                        sha256="added-hash",
                        modified_utc="2026-10-31T00:00:00+00:00",
                    ),
                ],
            )

        self.assertEqual(delta["added"], ["src/updates/API.md"])
        self.assertEqual(delta["removed"], [])
        self.assertEqual(delta["changed"], ["src/updates/README.md"])

    def test_source_scan_excludes_generated_wiki_but_keeps_curated_pages(self):
        wiki_sync = _load_module()

        with tempfile.TemporaryDirectory() as temp_dir:
            repo_root = Path(temp_dir)
            wiki_dir = repo_root / "ai_context" / "developer_llm_wiki"
            wiki_dir.mkdir(parents=True)
            for name in (
                "INDEX.md",
                "MODULES_AND_APIS.md",
                "BUILD_TEST_CI_AND_OPERATIONS.md",
                "GOVERNANCE_AND_ROADMAP.md",
                "WIKI_DELTA_REPORT.md",
                "AQL.md",
                "GRAPH.md",
                "ETHICS_AI.md",
                "IMPORTERS.md",
                "UPDATES.md",
                "DISTRIBUTED_TENSOR.md",
                "PROCESS.md",
                "MAINTENANCE.md",
                "TEMPORAL.md",
                "STORAGE.md",
                "USER_STORAGE_ENCRYPTED.md",
            ):
                (wiki_dir / name).write_text(f"# {name}\n", encoding="utf-8")

            sources = wiki_sync.collect_sources(repo_root, full_sync=True)

        source_paths = {source.path for source in sources}
        self.assertEqual(
            source_paths,
            {
                "ai_context/developer_llm_wiki/IMPORTERS.md",
                "ai_context/developer_llm_wiki/AQL.md",
                "ai_context/developer_llm_wiki/GRAPH.md",
                "ai_context/developer_llm_wiki/ETHICS_AI.md",
                "ai_context/developer_llm_wiki/UPDATES.md",
                "ai_context/developer_llm_wiki/DISTRIBUTED_TENSOR.md",
                "ai_context/developer_llm_wiki/PROCESS.md",
                "ai_context/developer_llm_wiki/MAINTENANCE.md",
                "ai_context/developer_llm_wiki/TEMPORAL.md",
                "ai_context/developer_llm_wiki/STORAGE.md",
                "ai_context/developer_llm_wiki/USER_STORAGE_ENCRYPTED.md",
            },
        )

    def test_source_scan_preserves_timestamp_for_unchanged_content(self):
        wiki_sync = _load_module()

        with tempfile.TemporaryDirectory() as temp_dir:
            repo_root = Path(temp_dir)
            source = repo_root / "ai_context" / "maintenance.md"
            source.parent.mkdir(parents=True)
            source.write_text("# Maintenance\n", encoding="utf-8")
            content_hash = wiki_sync.hashlib.sha256(source.read_bytes()).hexdigest()
            manifest_path = repo_root / wiki_sync.SOURCES_JSON
            manifest_path.parent.mkdir(parents=True)
            manifest_path.write_text(
                json.dumps(
                    {
                        "sources": [
                            {
                                "path": "ai_context/maintenance.md",
                                "size": source.stat().st_size,
                                "sha256": content_hash,
                                "modified_utc": "2026-10-30T12:00:00+00:00",
                            }
                        ]
                    }
                ),
                encoding="utf-8",
            )

            unchanged = wiki_sync.collect_sources(repo_root, full_sync=False)
            self.assertEqual(
                unchanged[0].modified_utc, "2026-10-30T12:00:00+00:00"
            )

            source.write_text("# Maintenance updated\n", encoding="utf-8")
            changed = wiki_sync.collect_sources(repo_root, full_sync=False)
            self.assertNotEqual(
                changed[0].modified_utc, "2026-10-30T12:00:00+00:00"
            )


if __name__ == "__main__":
    unittest.main()
