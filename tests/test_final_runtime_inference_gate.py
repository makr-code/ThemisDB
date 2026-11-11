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
        self.assertRegex(
            cmake,
            re.compile(
                r'if\(_stem STREQUAL "test_llm_tinyllama_inference"\s+'
                r'OR _stem STREQUAL "test_llm_doku_rag"\)\s+'
                r'target_sources\(\$\{_tgt\} PRIVATE\s+'
                r'\$\{THEMIS_ROOT_DIR\}/src/llm/embedded_llm\.cpp',
                re.MULTILINE,
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

        self.assertIn("-DTHEMIS_BUILD_MODULAR=OFF", configure_step)
        self.assertIn("-DTHEMIS_ENABLE_LLM=ON", configure_step)


if __name__ == "__main__":
    unittest.main()
