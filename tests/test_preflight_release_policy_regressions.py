#!/usr/bin/env python3

import re
import unittest
from pathlib import Path


REPO_ROOT = Path(__file__).resolve().parent.parent
BUILD_MAINLINE_WORKFLOW = REPO_ROOT / ".github" / "workflows" / "build-mainline.yml"
RELEASE_BUILD_MATRIX_WORKFLOW = REPO_ROOT / ".github" / "workflows" / "release-build-matrix.yml"
WORDPRESS_PRESS_WORKFLOW = REPO_ROOT / ".github" / "workflows" / "release-wordpress-press.yml"
TESTS_CMAKELISTS = REPO_ROOT / "tests" / "CMakeLists.txt"
DOCS_ROCKSDB_GENERATOR = REPO_ROOT / "scripts" / "generate_docs_rocksdb.py"


def extract_yaml_job_block(text: str, job_id: str) -> str:
    lines = text.splitlines()
    anchor = f"  {job_id}:"

    for index, line in enumerate(lines):
        if line.rstrip() == anchor:
            block_lines = [line]
            for candidate in lines[index + 1 :]:
                stripped_candidate = candidate.strip()
                if stripped_candidate and candidate.startswith("  ") and not candidate.startswith("    "):
                    break
                if stripped_candidate and not candidate.startswith("  "):
                    break
                block_lines.append(candidate)
            return "\n".join(block_lines)

    raise AssertionError(f"Could not find YAML job block for {job_id!r}")


def extract_cmake_if_block(text: str, anchor: str) -> str:
    lines = text.splitlines()

    for index, line in enumerate(lines):
        if line.strip() == anchor:
            depth = 0
            block_lines = []
            for candidate in lines[index:]:
                stripped = candidate.strip()
                normalized = stripped.lower()
                opens_if = re.match(r"if\s*\(", normalized) is not None
                closes_if = re.match(r"endif\s*\(", normalized) is not None

                if opens_if:
                    depth += 1
                elif closes_if and depth == 0:
                    raise AssertionError(f"Encountered unmatched endif while parsing {anchor!r}")

                if depth > 0:
                    block_lines.append(candidate)

                if closes_if:
                    depth -= 1
                    if depth == 0:
                        return "\n".join(block_lines)
            break

    raise AssertionError(f"Could not find balanced CMake block for {anchor!r}")


class PreflightReleasePolicyRegressionTests(unittest.TestCase):
    def test_release_build_matrix_submodule_sync_only_references_declared_paths(self) -> None:
        workflow_text = RELEASE_BUILD_MATRIX_WORKFLOW.read_text(encoding="utf-8")
        gitmodules_text = (REPO_ROOT / ".gitmodules").read_text(encoding="utf-8")
        declared_paths = set(re.findall(r"^\s*path = (.+)$", gitmodules_text, re.MULTILINE))

        for job_id in ("linux-release", "windows-release"):
            job_block = extract_yaml_job_block(workflow_text, job_id)
            sync_prefix = "git submodule sync -- "
            update_prefix = "git submodule update --init --depth 1 "
            sync_matches = [
                line.split(sync_prefix, 1)[1].split("||", 1)[0].strip().split()
                for line in job_block.splitlines()
                if sync_prefix in line
            ]
            update_matches = [
                line.split(update_prefix, 1)[1].split("||", 1)[0].strip().split()
                for line in job_block.splitlines()
                if update_prefix in line
            ]

            self.assertEqual(len(sync_matches), 1, msg=f"expected one sync command for {job_id}")
            self.assertEqual(len(update_matches), 1, msg=f"expected one update command for {job_id}")

            sync_paths = sync_matches[0]
            update_paths = update_matches[0]

            self.assertTrue(sync_paths)
            self.assertTrue(update_paths)
            self.assertEqual(set(sync_paths), set(update_paths))
            # Guard the exact stale path that broke the release packaging setup step.
            self.assertFalse(any(path == "llama.cpp" or path.endswith("/llama.cpp") for path in sync_paths))
            self.assertFalse(any(path == "llama.cpp" or path.endswith("/llama.cpp") for path in update_paths))
            self.assertTrue(set(sync_paths).issubset(declared_paths))
            self.assertTrue(set(update_paths).issubset(declared_paths))

    def test_macos_kqueue_lane_installs_googletest(self) -> None:
        workflow_text = BUILD_MAINLINE_WORKFLOW.read_text(encoding="utf-8")
        macos_kqueue_job = extract_yaml_job_block(workflow_text, "macos-kqueue-validation")

        self.assertRegex(
            macos_kqueue_job,
            re.compile(r"brew install[^\n]*\bgoogletest\b"),
        )

    def test_macos_kqueue_lane_retries_without_sccache_on_backend_failure(self) -> None:
        workflow_text = BUILD_MAINLINE_WORKFLOW.read_text(encoding="utf-8")
        macos_kqueue_job = extract_yaml_job_block(workflow_text, "macos-kqueue-validation")
        helper_match = re.search(
            r"disable_sccache_and_reconfigure\(\) \{(?P<body>.*?)\n\s+\}",
            macos_kqueue_job,
            re.DOTALL,
        )

        self.assertIn("SCCACHE_FALLBACK_APPLIED=0", macos_kqueue_job)
        self.assertIn(
            "grep -Eq 'sccache: error: Server startup failed|cache storage failed to read' \"${build_log}\"",
            macos_kqueue_job,
        )
        self.assertIsNotNone(helper_match)
        helper_body = helper_match.group("body")
        self.assertIn('cmake -S . -B build-macos \\', helper_body)
        self.assertIn("-DCMAKE_C_COMPILER_LAUNCHER=", helper_body)
        self.assertIn("-DCMAKE_CXX_COMPILER_LAUNCHER=", helper_body)

    def test_ai_safety_chaos_links_themis_llm_when_available(self) -> None:
        cmake_text = TESTS_CMAKELISTS.read_text(encoding="utf-8")
        ai_safety_chaos_block = extract_cmake_if_block(
            cmake_text,
            'if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/security/ai_safety/test_ai_safety_chaos.cpp")',
        )

        self.assertIn("if(TARGET themis_llm)", ai_safety_chaos_block)
        self.assertIn(
            "target_link_libraries(test_ai_safety_chaos PRIVATE themis_llm)",
            ai_safety_chaos_block,
        )

    def test_docs_importer_uses_raw_pointer_for_rocksdb_open(self) -> None:
        generator_text = DOCS_ROCKSDB_GENERATOR.read_text(encoding="utf-8")

        self.assertIn("DB* raw_db = nullptr;", generator_text)
        self.assertIn("Status status = DB::Open(options, db_path, &raw_db);", generator_text)
        self.assertIn("std::unique_ptr<DB> db(raw_db);", generator_text)
        self.assertNotIn("Status status = DB::Open(options, db_path, &db);", generator_text)

    def test_wordpress_press_dispatch_falls_back_to_synthetic_context(self) -> None:
        workflow_text = WORDPRESS_PRESS_WORKFLOW.read_text(encoding="utf-8")

        self.assertIn("async function buildSyntheticDispatchRelease(tagName)", workflow_text)
        self.assertIn("github.rest.git.getRef({", workflow_text)
        self.assertIn(
            "Dispatch tag ${dispatchTag} has no GitHub Release object; using synthetic context.",
            workflow_text,
        )
        self.assertIn("core.setOutput('has_release_object', hasReleaseObject ? 'true' : 'false');", workflow_text)

    def test_wordpress_press_live_publish_requires_real_release_object(self) -> None:
        workflow_text = WORDPRESS_PRESS_WORKFLOW.read_text(encoding="utf-8")

        self.assertIn(
            "if: steps.context.outputs.should_publish == 'true' && steps.context.outputs.has_release_object == 'true' && steps.dry-run.outputs.enabled != 'true'",
            workflow_text,
        )


if __name__ == "__main__":
    unittest.main()
