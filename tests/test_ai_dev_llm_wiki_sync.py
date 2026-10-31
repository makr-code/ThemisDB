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
        self.assertIn("must not redefine project status", index)
        self.assertIn("[Importer module context](IMPORTERS.md)", modules)
        self.assertIn("[Updates module context](UPDATES.md)", modules)

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
                "IMPORTERS.md",
                "UPDATES.md",
            ):
                (wiki_dir / name).write_text(f"# {name}\n", encoding="utf-8")

            sources = wiki_sync.collect_sources(repo_root, full_sync=True)

        source_paths = {source.path for source in sources}
        self.assertEqual(
            source_paths,
            {
                "ai_context/developer_llm_wiki/IMPORTERS.md",
                "ai_context/developer_llm_wiki/UPDATES.md",
            },
        )


if __name__ == "__main__":
    unittest.main()
