import unittest
from pathlib import Path

from scripts.doxygen_governance_gate import (
    build_coverage_command,
    filter_blocking_doxygen_warnings,
)


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


if __name__ == "__main__":
    unittest.main()
