import importlib.util
import sys
import unittest
from pathlib import Path


_SCRIPT = Path(__file__).parent.parent / "scripts" / "ai-context-lint.py"


def _load_module():
    spec = importlib.util.spec_from_file_location("ai_context_lint", _SCRIPT)
    module = importlib.util.module_from_spec(spec)
    assert spec and spec.loader
    sys.modules[spec.name] = module
    spec.loader.exec_module(module)
    return module


class ArchiveAwareStaleDetectionTests(unittest.TestCase):
    def test_stale_claims_ignores_archived_historical_ai_context(self):
        lint = _load_module()
        repo = Path(__file__).parent.parent
        active = repo / "ai_context" / "developer_llm_wiki"
        active.mkdir(parents=True, exist_ok=True)
        historical = repo / "ai_context" / "legacy_status"
        historical.mkdir(parents=True, exist_ok=True)
        archived = repo / "docs" / "ARCHIVED" / "ai-working-history"
        archived.mkdir(parents=True, exist_ok=True)

        active_file = active / "INDEX.md"
        active_file.write_text("# Active\n\nDatum: 2026-09-23\n", encoding="utf-8")

        historical_file = historical / "STATUS_2026_08_01.md"
        historical_file.write_text("# Historical\n\nDatum: 2026-08-01\n", encoding="utf-8")

        archived_file = archived / "LEGACY_SUMMARY.md"
        archived_file.write_text("# Archived\n\nDatum: 2026-08-01\n", encoding="utf-8")

        findings = lint.check_stale_claims(repo, [active_file, historical_file, archived_file], stale_days=30)

        self.assertEqual(len(findings), 0)
        self.assertTrue(all(f.file != "ai_context/legacy_status/STATUS_2026_08_01.md" for f in findings))
        self.assertTrue(all(f.file != "docs/ARCHIVED/ai-working-history/LEGACY_SUMMARY.md" for f in findings))


if __name__ == "__main__":
    unittest.main()
