import importlib.util
import tempfile
import unittest
from pathlib import Path


REPO_ROOT = Path(__file__).resolve().parents[1]
SCRIPT_PATH = REPO_ROOT / "scripts" / "compendium-drift-scan.py"
SPEC = importlib.util.spec_from_file_location("compendium_drift_scan", SCRIPT_PATH)
compendium_drift_scan = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(compendium_drift_scan)


class CompendiumDriftScanTests(unittest.TestCase):
    def test_target_version_defaults_to_mapping_and_accepts_valid_override(self):
        mapping = {"target_version": "2.4.0-alpha"}

        self.assertEqual(
            compendium_drift_scan._resolve_target_version(None, mapping),
            "2.4.0-alpha",
        )
        self.assertEqual(
            compendium_drift_scan._resolve_target_version("3.0.0-rc.1", mapping),
            "3.0.0-rc.1",
        )

    def test_target_version_rejects_invalid_mapping_or_override(self):
        for override, mapping in (
            ("$(touch /tmp/injected)", {"target_version": "2.4.0-alpha"}),
            (None, {"target_version": ""}),
        ):
            with self.subTest(override=override, mapping=mapping):
                with self.assertRaises(ValueError):
                    compendium_drift_scan._resolve_target_version(override, mapping)

    def test_target_version_drives_staleness_and_chapter_signals(self):
        self.assertTrue(compendium_drift_scan._is_stale_version("2.4.0", "3.0.0"))
        self.assertTrue(compendium_drift_scan._is_stale_version("3.0.0-rc.1", "3.0.0"))
        self.assertTrue(
            compendium_drift_scan._chapter_has_target_version_signals(
                "The current release is v3.0.0-rc.1.", "3.0.0-rc.1"
            )
        )
        self.assertFalse(
            compendium_drift_scan._chapter_has_target_version_signals(
                "The current release is v2.4.0-alpha.", "3.0.0"
            )
        )

    def test_alignment_summary_does_not_count_as_topic_integration(self):
        topics = ["voice", "anti-spoof", "liveness"]
        sot_cache = {"ROADMAP.md": "voice anti-spoof liveness"}
        chapter = (
            "## v2.4.0-alpha roadmap alignment\n"
            "This chapter covers voice, anti-spoof, and liveness.\n"
            "## Overview\n"
            "Voice streaming runtime overview.\n"
        )

        missing = compendium_drift_scan._missing_sot_topics(
            sot_cache, ["ROADMAP.md"], topics, chapter
        )

        self.assertEqual(missing, ["anti-spoof", "liveness"])

    def test_topics_in_chapter_body_are_not_reported_missing(self):
        topics = ["voice", "anti-spoof", "liveness"]
        sot_cache = {"ROADMAP.md": "voice anti-spoof liveness"}
        chapter = (
            "## v2.4.0-alpha roadmap alignment\n"
            "This chapter covers voice.\n"
            "## Security status\n"
            "Voice anti-spoof tests and heuristic liveness remain uncalibrated.\n"
        )

        missing = compendium_drift_scan._missing_sot_topics(
            sot_cache, ["ROADMAP.md"], topics, chapter
        )

        self.assertEqual(missing, [])

    def test_alignment_section_exclusion_uses_target_version(self):
        topics = ["voice"]
        sot_cache = {"ROADMAP.md": "voice"}
        chapter = (
            "## v3.0.0-rc.1 roadmap alignment\n"
            "This chapter covers voice.\n"
            "## Overview\n"
            "General overview.\n"
        )

        missing = compendium_drift_scan._missing_sot_topics(
            sot_cache, ["ROADMAP.md"], topics, chapter, "3.0.0-rc.1"
        )

        self.assertEqual(missing, ["voice"])

    def test_nested_version_keys_are_not_product_release_versions(self):
        text = (
            "model_card_version: 1.0\n"
            "themis_version=1.3.4\n"
            "Version: 1.0\n"
            "Version: 2.4.0-alpha\n"
        )

        self.assertEqual(
            compendium_drift_scan._extract_versions(text),
            ["1.0", "2.4.0-alpha"],
        )

    def test_missing_topics_are_reported_by_chapter_scan(self):
        with tempfile.TemporaryDirectory() as temp_dir:
            chapter_path = Path(temp_dir) / "chapter.md"
            chapter_path.write_text(
                "## v2.4.0-alpha roadmap alignment\n"
                "This chapter covers voice.\n"
                "## Overview\n"
                "Voice streaming runtime overview.\n",
                encoding="utf-8",
            )
            entry = {
                "file": "chapter.md",
                "title": "Voice",
                "priority": "high",
                "sot_paths": ["ROADMAP.md"],
                "roadmap_topics": ["voice", "anti-spoof"],
            }

            result = compendium_drift_scan.scan_chapter(
                entry,
                Path(temp_dir),
                {"ROADMAP.md": "voice anti-spoof"},
                "2.4.0-alpha",
            )

        self.assertEqual(result["verdict"], "stale")
        self.assertEqual(result["active_sot_topics"], ["anti-spoof"])


if __name__ == "__main__":
    unittest.main()
