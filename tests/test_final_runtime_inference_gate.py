#!/usr/bin/env python3

import re
import unittest
from pathlib import Path


REPO_ROOT = Path(__file__).resolve().parent.parent


class FinalRuntimeInferenceGateConfigurationTests(unittest.TestCase):
    def test_release_critical_suite_has_one_pr_build_path(self) -> None:
        core = (
            REPO_ROOT / ".github" / "workflows" / "gate-pr-core.yml"
        ).read_text(encoding="utf-8")
        reusable = (
            REPO_ROOT / ".github" / "workflows" / "reusable-pr-build-components-gate.yml"
        ).read_text(encoding="utf-8")
        self.assertNotIn("quality-test-build:", core)
        self.assertNotIn("quality-test-gate:", core)
        self.assertIn("release-critical-tests:", reusable)
        self.assertIn("target themis_release_critical_tests", reusable)

    def test_final_runtime_gate_is_invoked_once_by_merge_readiness(self) -> None:
        core = (
            REPO_ROOT / ".github" / "workflows" / "gate-pr-core.yml"
        ).read_text(encoding="utf-8")
        components = (
            REPO_ROOT
            / ".github"
            / "workflows"
            / "reusable-pr-build-components-gate.yml"
        ).read_text(encoding="utf-8")
        merge_readiness = (
            REPO_ROOT
            / ".github"
            / "workflows"
            / "gate-pr-merge-readiness.yml"
        ).read_text(encoding="utf-8")
        self.assertNotIn("gate-pr-final-runtime-api.yml", core)
        self.assertNotIn("gate-pr-final-runtime-api.yml", components)
        self.assertEqual(
            merge_readiness.count(
                "uses: ./.github/workflows/gate-pr-final-runtime-api.yml"
            ),
            1,
        )

    def test_build_components_gate_uses_supported_boost_runner(self) -> None:
        workflow = (
            REPO_ROOT
            / ".github"
            / "workflows"
            / "reusable-pr-build-components-gate.yml"
        ).read_text(encoding="utf-8")
        release_job = workflow.split("\n  release-critical-tests:", 1)[1]
        release_job = release_job.split("\n  build-components-summary:", 1)[0]
        self.assertIn("runs-on: ubuntu-24.04", release_job)
        for package in ("libboost-system-dev", "libboost-filesystem-dev"):
            self.assertRegex(release_job, rf"\b{package}\b")

    def test_inference_gate_uses_supported_boost_runner(self) -> None:
        workflow = (
            REPO_ROOT
            / ".github"
            / "workflows"
            / "gate-pr-final-runtime-api.yml"
        ).read_text(encoding="utf-8")
        inference_job = workflow.split("  llm-docsdb-inference-gate:", 1)[1]
        inference_job = inference_job.split("\n  final-runtime-api-gate:", 1)[0]
        self.assertIn("runs-on: ubuntu-24.04", inference_job)
        install_step = inference_job.split("name: Install build dependencies", 1)[1]
        install_step = install_step.split("\n      - name:", 1)[0]
        for package in ("libboost-dev", "libboost-system-dev", "libboost-filesystem-dev"):
            self.assertRegex(install_step, rf"\b{package}\b")

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

    def test_tinyllama_gate_preserves_timeout_diagnostics(self) -> None:
        workflow = (
            REPO_ROOT
            / ".github"
            / "workflows"
            / "gate-pr-final-runtime-api.yml"
        ).read_text(encoding="utf-8")
        inference_step = workflow.split("name: Run TinyLlama inference tests", 1)[1]
        inference_step = inference_step.split("\n      - name:", 1)[0]
        self.assertIn("--verbose", inference_step)
        self.assertIn("--timeout 600", inference_step)
        self.assertIn("ctest-tinyllama.log", inference_step)

        artifact_step = workflow.split("name: Upload LLM/docs.db gate artifacts", 1)[1]
        artifact_step = artifact_step.split("\n  final-runtime-api-gate:", 1)[0]
        self.assertIn("if: always()", artifact_step)
        self.assertIn("Testing/Temporary/LastTest.log", artifact_step)

    def test_docs_db_rag_timeout_matches_ctest_registration(self) -> None:
        cmake = (REPO_ROOT / "tests" / "llm" / "CMakeLists.txt").read_text(
            encoding="utf-8"
        )
        self.assertRegex(
            cmake,
            re.compile(
                r"_add_llm_inference_ci_target\(\s*test_llm_doku_rag\s+"
                r'"[^"]+"\s+600\s*\)',
                re.DOTALL,
            ),
        )

        workflow = (
            REPO_ROOT
            / ".github"
            / "workflows"
            / "gate-pr-final-runtime-api.yml"
        ).read_text(encoding="utf-8")
        rag_step = workflow.split("name: Run docs.db RAG inference tests", 1)[1]
        rag_step = rag_step.split("\n      - name:", 1)[0]
        self.assertIn("--timeout 600", rag_step)

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
