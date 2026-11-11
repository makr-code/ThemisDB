#!/usr/bin/env python3

import re
import unittest
from pathlib import Path


REPO_ROOT = Path(__file__).resolve().parent.parent


class FinalRuntimeInferenceGateConfigurationTests(unittest.TestCase):
    def test_modular_inference_targets_compile_production_embedded_llm(self) -> None:
        cmake = (REPO_ROOT / "tests" / "llm" / "CMakeLists.txt").read_text(
            encoding="utf-8"
        )
        self.assertEqual(
            1, cmake.count("${THEMIS_ROOT_DIR}/src/llm/embedded_llm.cpp")
        )
        self.assertRegex(
            cmake,
            re.compile(
                r'if\(THEMIS_BUILD_MODULAR\)\s+'
                r'if\(NOT TARGET themis_llm\).*?'
                r'target_sources\(\$\{_tgt\} PRIVATE\s+'
                r'\$\{THEMIS_ROOT_DIR\}/src/llm/embedded_llm\.cpp\s+\)\s+'
                r'target_link_libraries\(\$\{_tgt\} PRIVATE themis_llm\)',
                re.DOTALL,
            ),
        )

    def test_runtime_gate_uses_production_llm_configuration(self) -> None:
        workflow = (
            REPO_ROOT
            / ".github"
            / "workflows"
            / "gate-pr-final-runtime-api.yml"
        ).read_text(encoding="utf-8")
        configure_step = workflow.split("name: Configure focused LLM gate build", 1)[1]
        configure_step = configure_step.split("\n      - name:", 1)[0]

        self.assertIn("-DTHEMIS_BUILD_MODULAR=ON", configure_step)
        self.assertIn("-DTHEMIS_ENABLE_LLM=ON", configure_step)

    def test_status_workflow_concurrency_is_scoped_by_operation_and_target(self) -> None:
        workflow = (
            REPO_ROOT
            / ".github"
            / "workflows"
            / "reusable-status-flags-and-issues.yml"
        ).read_text(encoding="utf-8")
        self.assertIn(
            "group: ${{ github.workflow }}-${{ github.ref }}-${{ inputs.operation }}-${{ inputs.issue-number || inputs.issue-marker || inputs.issue-title || 'unscoped' }}",
            workflow,
        )
        self.assertIn("cancel-in-progress: false", workflow)


if __name__ == "__main__":
    unittest.main()
