#!/usr/bin/env python3

import re
import unittest
from pathlib import Path


REPO_ROOT = Path(__file__).resolve().parent.parent
BUILD_MAINLINE_WORKFLOW = REPO_ROOT / ".github" / "workflows" / "build-mainline.yml"
TESTS_CMAKELISTS = REPO_ROOT / "tests" / "CMakeLists.txt"


def extract_yaml_job_block(text: str, job_id: str) -> str:
    lines = text.splitlines()
    anchor = f"  {job_id}:"

    for index, line in enumerate(lines):
        if line.strip() == anchor.strip():
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
        if line.strip() == anchor.strip():
            depth = 0
            block_lines = []
            for candidate in lines[index:]:
                stripped = candidate.strip()
                normalized = stripped.lower()
                opens_if = normalized.startswith("if(")
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
    def test_macos_kqueue_lane_installs_googletest(self) -> None:
        workflow_text = BUILD_MAINLINE_WORKFLOW.read_text(encoding="utf-8")
        macos_kqueue_job = extract_yaml_job_block(workflow_text, "macos-kqueue-validation")

        self.assertRegex(
            macos_kqueue_job,
            re.compile(r"brew install[^\n]*\bgoogletest\b"),
        )

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


if __name__ == "__main__":
    unittest.main()
