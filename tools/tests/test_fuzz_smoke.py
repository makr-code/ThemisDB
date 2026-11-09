from __future__ import annotations

import tempfile
import unittest
from pathlib import Path
from unittest.mock import patch

from tools.ci import fuzz_smoke
from tools.ci.fuzz_smoke import affected_targets


ROOT = Path(__file__).resolve().parents[2]


class FuzzSmokeScopeTests(unittest.TestCase):
    def test_pr_workflows_filter_canonical_branches(self):
        branches = (
            "develop",
            "community",
            "enterprise",
            "hyperscaler",
            "military",
            "minimal",
        )
        for name in (
            ".github/workflows/build-sanitizer-nightly.yml",
            ".github/workflows/security-fuzzing.yml",
        ):
            workflow = (ROOT / name).read_text(encoding="utf-8")
            pull_request = workflow.split("  pull_request:", 1)[1].split(
                "\n  schedule:", 1
            )[0]
            self.assertIn("    branches:", pull_request)
            for branch in branches:
                self.assertIn(f"      - {branch}", pull_request)

    def test_parser_paths_select_only_the_affected_harness(self):
        cases = {
            "src/query/aql_parser.cpp": ["aql_parser"],
            "src/server/http_parser.cpp": ["http_parser"],
            "tests/network/test_http_server_network.cpp": ["http_parser"],
            "tests/integration/test_http_server_hardening.cpp": ["http_parser"],
            "src/network/wire_protocol_server_ws.cpp": ["wire_protocol_parser"],
            "src/network/websocket_binary_frame_parser.cpp": ["wire_protocol_parser"],
            "src/importers/postgres_importer.cpp": ["postgres_importer"],
        }
        for path, expected in cases.items():
            with self.subTest(path=path):
                self.assertEqual(affected_targets([path]), expected)

    def test_fuzz_build_configuration_change_selects_the_parser_suite(self):
        expected = [
            "aql_parser",
            "http_parser",
            "postgres_importer",
            "wire_protocol_parser",
        ]
        for path in (
            ".github/workflows/security-fuzzing.yml",
            "cmake/ModularBuild.cmake",
            "CMakeLists.txt",
        ):
            with self.subTest(path=path):
                self.assertEqual(affected_targets([path]), expected)
        for workflow_path in (
            ".github/workflows/build-sanitizer-nightly.yml",
            ".github/workflows/security-fuzzing.yml",
        ):
            with self.subTest(workflow=workflow_path):
                workflow = (ROOT / workflow_path).read_text(encoding="utf-8")
                self.assertIn("- 'CMakeLists.txt'", workflow)

    def test_unrelated_paths_select_no_fuzz_targets(self):
        for path in (
            "README.md",
            "src/graph/traversal.cpp",
            "tests/storage/test_storage.cpp",
            "tests/network/test_storage_contract.cpp",
            "tests/integration/test_graph_query.cpp",
            "include/themis/network/unrelated.h",
        ):
            with self.subTest(path=path):
                self.assertEqual(affected_targets([path]), [])

    def test_http_test_path_filters_cover_network_and_integration_cases(self):
        workflow = (ROOT / ".github/workflows/security-fuzzing.yml").read_text(
            encoding="utf-8"
        )
        self.assertIn("'tests/network/test_http*'", workflow)
        self.assertIn("'tests/integration/test_http*'", workflow)

    def test_fuzz_lane_is_fail_closed_and_bounded(self):
        workflow = (ROOT / ".github/workflows/security-fuzzing.yml").read_text(encoding="utf-8")
        smoke = (ROOT / "tools/ci/fuzz_smoke.py").read_text(encoding="utf-8")
        self.assertIn("parser budget=300s", smoke)
        self.assertIn("timeout=remaining", smoke)
        self.assertIn("time.monotonic() + 300", smoke)
        self.assertNotIn("|| true", workflow)
        self.assertIn("timeout_minutes: '45'", workflow)
        self.assertIn("REQUIRED_SEEDS", smoke)
        fuzz_cmake = (ROOT / "fuzz/CMakeLists.txt").read_text(encoding="utf-8")
        self.assertIn("src/query/aql_parser.cpp", fuzz_cmake)
        self.assertIn("src/importers/postgres_importer.cpp", fuzz_cmake)
        self.assertNotIn('"stubs/postgres_importer_stub.cpp"', fuzz_cmake)
        aql_harness = (ROOT / "fuzz/harnesses/aql_parser_harness.cpp").read_text(encoding="utf-8")
        self.assertIn("themis::query::AQLParser", aql_harness)
        self.assertNotIn("class Parser", aql_harness)
        wire_harness = (
            ROOT / "fuzz/harnesses/wire_protocol_parser_harness.cpp"
        ).read_text(encoding="utf-8")
        ws_session = (
            ROOT / "src/network/wire_protocol_server_ws.cpp"
        ).read_text(encoding="utf-8")
        parser_call = "detail::parseWebSocketBinaryFrame"
        self.assertIn(parser_call, wire_harness)
        self.assertIn(parser_call, ws_session)

    def test_smoke_results_are_persisted_for_the_artifact(self):
        workflow = (ROOT / ".github/workflows/security-fuzzing.yml").read_text(
            encoding="utf-8"
        )
        self.assertIn("build-fuzz/fuzz-smoke-results.log", workflow)

        with tempfile.TemporaryDirectory() as temporary_directory:
            temporary_root = Path(temporary_directory)
            corpus = temporary_root / "fuzz/corpus/aql"
            corpus.mkdir(parents=True)
            seed = corpus / "seed_select.aql"
            seed.write_text("FOR doc IN docs RETURN doc\n", encoding="utf-8")

            binary = (
                temporary_root
                / "build-fuzz/fuzz/bin/aql_parser_harness"
            )
            binary.parent.mkdir(parents=True)
            binary.write_text(
                "#!/usr/bin/env python3\n"
                "print('harness output captured')\n",
                encoding="utf-8",
            )
            binary.chmod(0o755)

            with patch.object(fuzz_smoke, "ROOT", temporary_root):
                self.assertEqual(fuzz_smoke.run_smoke(["aql_parser"]), 0)

            results = (
                temporary_root / "build-fuzz/fuzz-smoke-results.log"
            ).read_text(encoding="utf-8")
            self.assertIn("Selected parser targets: aql_parser", results)
            self.assertIn("OUTPUT aql_parser/seed_select.aql", results)
            self.assertIn("harness output captured", results)
            self.assertIn("PASS aql_parser/seed_select.aql", results)

    def test_sanitizer_lane_has_narrow_pr_scope_and_tsan_targets(self):
        workflow = (ROOT / ".github/workflows/build-sanitizer-nightly.yml").read_text(encoding="utf-8")
        self.assertIn("  pull_request:", workflow)
        self.assertIn("- 'src/storage/**'", workflow)
        self.assertIn("- 'include/sharding/**'", workflow)
        self.assertIn('SAN_FLAGS="-fsanitize=address,undefined', workflow)
        for module in ("storage", "transaction", "query", "aql", "server", "auth", "network", "sharding"):
            self.assertIn(f"- 'src/{module}/**'", workflow)
            self.assertIn(f"- 'include/{module}/**'", workflow)
            self.assertIn(f"- 'tests/{module}/**'", workflow)
        root_module_tests = (
            "tests/test_transaction_auditor.cpp",
            "tests/test_shard_router_focused.cpp",
            "tests/test_query_engine.cpp",
            "tests/test_themis_wire_protocol_server.cpp",
        )
        for path in root_module_tests:
            with self.subTest(path=path):
                self.assertIn(f"- '{path}'", workflow)
                self.assertTrue((ROOT / path).is_file())
        self.assertNotIn("- 'tests/test_*.cpp'", workflow)
        self.assertNotIn("- 'src/graph/**'", workflow)
        for target in (
            "module_storage_test_storage_contract_hardening_focused_focused",
            "test_query_engine_focused",
            "module_aql_test_aql_query_validator_focused",
            "module_server_test_server_phase2_focused_focused",
            "module_auth_test_wave8_auth_hardening_focused",
            "module_network_test_network_contract_hardening_focused_focused",
            "module_network_test_wire_protocol_v2_focused",
            "test_transaction_auditor_focused",
            "test_shard_router_focused",
            "module_llm_test_llm_phase1_hardening_focused",
        ):
            self.assertIn(target, workflow)
        network_tests = (ROOT / "tests/network/CMakeLists.txt").read_text(
            encoding="utf-8"
        )
        self.assertIn(
            'set(_target "module_network_${_stem}_focused")',
            network_tests,
        )
        self.assertIn('set(_ctest "${_stem}_network_FocusedTests")', network_tests)
        self.assertTrue((ROOT / "tests/network/test_wire_protocol_v2.cpp").is_file())
        self.assertIn(
            "test_wire_protocol_v2_network_FocusedTests",
            workflow,
        )
        self.assertNotIn("WireProtocolV2FocusedTests", workflow)
        self.assertIn("FAIL/CANCELLED (not PASS", workflow)
        self.assertIn("actions/runs/${GITHUB_RUN_ID}", workflow)
        self.assertIn('-R "$TEST_REGEX"', workflow)
        tsan_job = workflow.split("  tsan-build:", 1)[1]
        self.assertIn("TEST_REGEX+=\"ShardRouterFocusedTests|\"", tsan_job)
        self.assertIn(
            "TEST_REGEX+=\"test_llm_phase1_hardening_LlmFocusedTests\"",
            tsan_job,
        )

    def test_sanitizer_evidence_is_fail_closed_and_uses_pinned_artifact_tier(self):
        workflow = (ROOT / ".github/workflows/build-sanitizer-nightly.yml").read_text(
            encoding="utf-8"
        )
        for field in (
            "Run URL:",
            "Tested commit SHA:",
            "Sanitizer targets:",
            "Workflow elapsed duration:",
            "Outcome:",
            "FAIL/CANCELLED (not PASS",
        ):
            self.assertIn(field, workflow)
        self.assertIn("actions/upload-artifact@v5", workflow)
        self.assertNotIn("actions/upload-artifact@v4", workflow)
        self.assertIn("if-no-files-found: error", workflow)
        self.assertIn("retention-days: 30", workflow)

    def test_fuzz_smoke_is_not_duplicated_on_schedule(self):
        workflow = (ROOT / ".github/workflows/security-fuzzing.yml").read_text(
            encoding="utf-8"
        )
        parser_job = workflow.split("  parser-smoke:", 1)[1].split(
            "  afl-fuzz:", 1
        )[0]
        self.assertIn("github.event_name == 'pull_request'", parser_job)
        self.assertIn("github.event_name == 'workflow_dispatch'", parser_job)
        self.assertNotIn("github.event_name == 'schedule'", parser_job)
        self.assertIn("github.event_name == 'schedule'", workflow.split("  afl-fuzz:", 1)[1])

    def test_fuzz_run_evidence_and_scheduled_artifact_failures_are_retained(self):
        workflow = (ROOT / ".github/workflows/security-fuzzing.yml").read_text(
            encoding="utf-8"
        )
        for field in (
            "Run URL:",
            "Tested commit SHA:",
            "Target:",
            "Workflow elapsed duration:",
            "FAIL/CANCELLED (not PASS",
        ):
            self.assertIn(field, workflow)
        self.assertIn("actions/upload-artifact@v5", workflow)
        self.assertNotIn("actions/upload-artifact@v4", workflow)
        self.assertIn("if-no-files-found: error", workflow)
        self.assertIn("artifact_retention_days: '30'", workflow)
        afl_job = workflow.split("  afl-fuzz:", 1)[1]
        self.assertIn("AFL++ selected file-mode targets: ", workflow)
        self.assertIn(
            "aql_parser, postgres_importer, wire_protocol_parser", workflow
        )
        self.assertIn("PARTIAL (not PASS; unsupported AFL++ targets were skipped)", workflow)
        self.assertIn("cmake --build build-afl --target \"${targets[@]}\"", afl_job)
        self.assertNotIn("--target fuzz_targets", afl_job)
        build_command = afl_job.split("build_command: |", 1)[1].split(
            "validation_command: |", 1
        )[0]
        for target in (
            "aql_parser_harness",
            "postgres_importer_harness",
            "wire_protocol_parser_harness",
        ):
            self.assertIn(target, build_command)
        for skipped in (
            "gguf_loader_harness",
            "grammar_harness",
            "http_parser_harness",
            "jwt_rbac_config_harness",
            "ldap_dn_harness",
            "pii_redaction_harness",
            "security_input_validator_harness",
            "security_policy_engine_harness",
        ):
            self.assertNotIn(skipped, build_command)
        for skipped in (
            "gguf_loader", "grammar", "http_parser", "jwt_rbac_config",
            "ldap_dn", "pii_redaction", "security_input_validator",
            "security_policy_engine",
        ):
            self.assertIn(skipped, workflow)
        self.assertIn("AFL failed for $harness", afl_job)
        self.assertIn("AFL finding for $harness", afl_job)
        self.assertIn("artifact_name: afl-fuzz-findings-", afl_job)


if __name__ == "__main__":
    unittest.main()
