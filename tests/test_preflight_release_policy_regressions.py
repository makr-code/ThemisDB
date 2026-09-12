#!/usr/bin/env python3

import unittest
from pathlib import Path


REPO_ROOT = Path(__file__).resolve().parent.parent
BUILD_MAINLINE_WORKFLOW = REPO_ROOT / ".github" / "workflows" / "build-mainline.yml"
TESTS_CMAKELISTS = REPO_ROOT / "tests" / "CMakeLists.txt"


class PreflightReleasePolicyRegressionTests(unittest.TestCase):
    def test_macos_kqueue_lane_installs_googletest(self) -> None:
        workflow_text = BUILD_MAINLINE_WORKFLOW.read_text(encoding="utf-8")

        self.assertIn(
            "brew install cmake ninja fmt spdlog nlohmann-json tbb openssl@3 boost rocksdb yaml-cpp googletest",
            workflow_text,
        )

    def test_ai_safety_chaos_links_themis_llm_when_available(self) -> None:
        cmake_text = TESTS_CMAKELISTS.read_text(encoding="utf-8")
        start = cmake_text.index('if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/security/ai_safety/test_ai_safety_chaos.cpp")')
        end = cmake_text.index('message(STATUS "  AiSafetyChaos: CHAOS-01..CHAOS-12', start)
        ai_safety_chaos_block = cmake_text[start:end]

        self.assertIn("if(TARGET themis_llm)", ai_safety_chaos_block)
        self.assertIn(
            "target_link_libraries(test_ai_safety_chaos PRIVATE themis_llm)",
            ai_safety_chaos_block,
        )


if __name__ == "__main__":
    unittest.main()
