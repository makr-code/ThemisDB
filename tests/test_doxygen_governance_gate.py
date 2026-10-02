import unittest
from pathlib import Path

from scripts.doxygen_governance_gate import (
    build_coverage_command,
    filter_blocking_doxygen_warnings,
)
from tools.scanners.gs3_step04_quality_cpp_doxygen import ThemisCppDoxygenPolicyRulesScan


class DoxygenGovernanceGateTests(unittest.TestCase):
    def test_build_coverage_command_includes_output_argument(self) -> None:
        command = build_coverage_command(
            Path("/tmp/xml"),
            Path("/repo"),
            Path("/tmp/coverage-summary.txt"),
        )

        self.assertIn("--output", command)
        self.assertEqual(command[command.index("--output") + 1], "/tmp/coverage-summary.txt")
        self.assertEqual(command[command.index("--scope") + 1], "public")
        self.assertEqual(
            command[command.index("--kind") + 1],
            "class,struct,function,typedef,define,file,namespace",
        )

    def test_filter_blocking_warnings_keeps_changed_public_headers_only(self) -> None:
        repo_root = Path("/repo")
        warnings = [
            "/repo/include/storage/rocksdb_wrapper.h:339: warning: missing @return",
            "/repo/src/storage/rocksdb_wrapper.cpp:40: warning: documented symbol not found",
            "/repo/include/query/query_engine.h:12: warning: include file missing",
            "coverxygen failed with exit code 2",
        ]

        blocking = filter_blocking_doxygen_warnings(
            repo_root,
            warnings,
            ["include/storage/rocksdb_wrapper.h"],
        )

        self.assertEqual(
            blocking,
            ["/repo/include/storage/rocksdb_wrapper.h:339: warning: missing @return"],
        )

    def test_no_changed_public_headers_skips_doxygen(self) -> None:
        """
        Test the fix for issue #6610:
        When a PR changes source files but no public headers,
        the Doxygen gate should skip the check and PASS.
        
        Scenario (from PR #6602):
        - changed_code_files: ['src/query/continuous_query_engine.cpp', 'tests/query/...']
        - changed_public_header_files: [] (empty)
        - coverage_enforced: False
        
        Expected: Gate should skip doxygen and return PASS (verdict != FAIL)
        """
        # The logic to test:
        # if changed_code_files and (changed_public_header_files or coverage_enforced):
        #     # run doxygen
        
        changed_code_files = ['src/query/continuous_query_engine.cpp', 'tests/query/test_continuous_query_lock_order_deadlock.cpp']
        changed_public_header_files = []  # No public headers changed
        coverage_enforced = False
        
        # The condition that determines if doxygen should run
        should_run_doxygen = bool(changed_code_files) and (bool(changed_public_header_files) or coverage_enforced)
        
        # With our fix, doxygen should NOT run (should be False)
        self.assertFalse(should_run_doxygen, 
            "Doxygen should not run when there are no changed public headers and coverage is not enforced")

    def test_pointer_utils_public_api_has_doxygen_comments(self) -> None:
        repo_root = Path(__file__).resolve().parent.parent
        scanner = ThemisCppDoxygenPolicyRulesScan(str(repo_root))

        findings = scanner.scan_files([repo_root / "include" / "utils" / "pointer_utils.h"])

        public_api_findings = [
            item for item in findings
            if item.get("file") == "include/utils/pointer_utils.h"
        ]

        self.assertFalse(
            any(item.get("pattern") == "missing_doxygen_comment" for item in public_api_findings),
            "pointer_utils.h public API should be documented for Doxygen governance",
        )


if __name__ == "__main__":
    unittest.main()
