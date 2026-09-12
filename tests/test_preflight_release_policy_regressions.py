#!/usr/bin/env python3

import re
import unittest
from pathlib import Path


REPO_ROOT = Path(__file__).resolve().parent.parent
BUILD_MAINLINE_WORKFLOW = REPO_ROOT / ".github" / "workflows" / "build-mainline.yml"
TESTS_CMAKELISTS = REPO_ROOT / "tests" / "CMakeLists.txt"


class PreflightReleasePolicyRegressionTests(unittest.TestCase):
    def test_macos_kqueue_lane_installs_googletest(self) -> None:
        workflow_text = BUILD_MAINLINE_WORKFLOW.read_text(encoding="utf-8")

        self.assertRegex(
            workflow_text,
            re.compile(r"brew install[^\n]*\bgoogletest\b"),
        )

    def test_ai_safety_chaos_links_themis_llm_when_available(self) -> None:
        cmake_text = TESTS_CMAKELISTS.read_text(encoding="utf-8")
        self.assertRegex(
            cmake_text,
            re.compile(
                r'if\(EXISTS "\$\{CMAKE_CURRENT_SOURCE_DIR\}/security/ai_safety/test_ai_safety_chaos\.cpp"\)'
                r".*?if\(TARGET themis_llm\)\s+target_link_libraries\(test_ai_safety_chaos PRIVATE themis_llm\)\s+endif\(\)",
                re.DOTALL,
            ),
        )


if __name__ == "__main__":
    unittest.main()
