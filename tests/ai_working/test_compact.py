import contextlib
import importlib.util
import io
import os
import sys
import tempfile
import unittest
from datetime import datetime, timedelta, timezone
from pathlib import Path
from unittest.mock import patch


SCRIPT = Path(__file__).resolve().parents[2] / "scripts" / "ai-working-compact.py"
SPEC = importlib.util.spec_from_file_location("ai_working_compact", SCRIPT)
COMPACT = importlib.util.module_from_spec(SPEC)
sys.modules[SPEC.name] = COMPACT
SPEC.loader.exec_module(COMPACT)

ARCHIVE_SCRIPT = Path(__file__).resolve().parents[2] / "scripts" / "archive_ai_working.py"
ARCHIVE_SPEC = importlib.util.spec_from_file_location("archive_ai_working", ARCHIVE_SCRIPT)
ARCHIVE = importlib.util.module_from_spec(ARCHIVE_SPEC)
sys.modules[ARCHIVE_SPEC.name] = ARCHIVE
ARCHIVE_SPEC.loader.exec_module(ARCHIVE)


class CompactTests(unittest.TestCase):
    def test_generated_summary_is_non_normative(self):
        with tempfile.TemporaryDirectory() as temp_dir:
            source = Path(temp_dir) / "source.md"
            source.write_text("Evidence source", encoding="utf-8")
            entry = COMPACT.FileEntry(
                path=source,
                mtime=datetime(2026, 11, 18, tzinfo=timezone.utc),
                size=source.stat().st_size,
            )

            page = COMPACT._build_compact_page("MISC", [entry], "2026-11-18")

        self.assertIn("non-normative generated summary", page)
        self.assertIn("Canonical repository sources remain authoritative.", page)

    def test_freshness_counter_excludes_all_whitespace(self):
        self.assertEqual(ARCHIVE._substantive_chars("a \t\n\r\v\f\u00a0b"), 2)

    def test_unreadable_source_fails_instead_of_omitting_content(self):
        with tempfile.TemporaryDirectory() as temp_dir:
            source = Path(temp_dir) / "missing.md"

            with self.assertRaisesRegex(RuntimeError, "missing.md"):
                COMPACT._safe_read(source)

    def test_archive_collision_never_overwrites_existing_entries(self):
        fixed_now = datetime(2026, 11, 18, 12, tzinfo=timezone.utc)
        archive_name = (fixed_now - timedelta(days=1)).strftime("ARCHIVE_PRE_%Y_%m")
        previous_cwd = Path.cwd()

        class FixedDateTime(datetime):
            @classmethod
            def now(cls, tz=None):
                return fixed_now

        with tempfile.TemporaryDirectory() as temp_dir:
            root = Path(temp_dir)
            source_dir = root / "ai_working"
            archive_dir = source_dir / archive_name
            source_dir.mkdir()
            archive_dir.mkdir()
            source = source_dir / "report.md"
            original_collision = archive_dir / "report.md"
            duplicate_collision = archive_dir / "report_dup.md"
            source.write_text("new report", encoding="utf-8")
            original_collision.write_text("existing report", encoding="utf-8")
            duplicate_collision.write_text("existing duplicate", encoding="utf-8")
            old_timestamp = (fixed_now - timedelta(days=2)).timestamp()
            os.utime(source, (old_timestamp, old_timestamp))

            try:
                os.chdir(root)
                with patch.object(COMPACT, "AI_WORKING", Path("ai_working")), patch.object(
                    COMPACT, "datetime", FixedDateTime
                ), contextlib.redirect_stdout(io.StringIO()):
                    COMPACT.archive(archive_days=1, dry_run=False, stats=COMPACT.Stats())
            finally:
                os.chdir(previous_cwd)

            self.assertFalse(source.exists())
            self.assertEqual(original_collision.read_text(encoding="utf-8"), "existing report")
            self.assertEqual(duplicate_collision.read_text(encoding="utf-8"), "existing duplicate")
            self.assertEqual(
                (archive_dir / "report_dup_1.md").read_text(encoding="utf-8"),
                "new report",
            )


if __name__ == "__main__":
    unittest.main()
