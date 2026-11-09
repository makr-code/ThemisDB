from __future__ import annotations

import unittest

from tools.ci.sanitizer_targets import (
    FULL_BUILD_TARGETS,
    MODULE_TARGETS,
    affected_modules,
    selected_modules,
    selected_targets,
)


class SanitizerTargetSelectionTests(unittest.TestCase):
    def test_module_paths_select_only_the_matching_sanitizer_test(self):
        cases = {
            "src/storage/wal_storage.cpp": ["storage"],
            "include/transaction/transaction.h": ["transaction"],
            "tests/query/test_query_engine.cpp": ["query"],
            "tests/test_query_engine.cpp": ["query"],
            "tests/test_transaction_auditor.cpp": ["transaction"],
            "src/query/aql_parser.cpp": ["aql"],
            "src/server/http_server.cpp": ["server"],
            "tests/auth/test_auth.cpp": ["auth"],
            "src/network/wire_protocol_server_ws.cpp": [
                "network",
                "wire_protocol",
            ],
            "src/network/websocket_binary_frame_parser.cpp": [
                "network",
                "wire_protocol",
            ],
            "tests/test_themis_wire_protocol_server.cpp": [
                "network",
                "wire_protocol",
            ],
            "src/sharding/shard_router.cpp": ["sharding"],
            "tests/test_shard_router_focused.cpp": ["sharding"],
        }
        for path, expected in cases.items():
            with self.subTest(path=path):
                self.assertEqual(affected_modules([path]), expected)

    def test_unrelated_paths_select_no_sanitizer_tests(self):
        for path in (
            "README.md",
            "src/graph/traversal.cpp",
            "tests/integration/test_graph_query.cpp",
        ):
            with self.subTest(path=path):
                self.assertEqual(affected_modules([path]), [])

    def test_top_level_build_configuration_selects_all_modules(self):
        self.assertEqual(
            affected_modules(["CMakeLists.txt"]),
            list(MODULE_TARGETS),
        )
        targets, _ = selected_targets(["CMakeLists.txt"])
        self.assertEqual(targets, list(FULL_BUILD_TARGETS))

    def test_workflow_only_change_uses_a_small_sanitizer_smoke(self):
        targets, test_regex = selected_targets(
            [".github/workflows/build-sanitizer-nightly.yml"]
        )
        self.assertEqual(
            targets,
            [MODULE_TARGETS["wire_protocol"][0]],
        )
        self.assertEqual(test_regex, MODULE_TARGETS["wire_protocol"][1])

    def test_nightly_selection_keeps_the_complete_sanitizer_suite(self):
        targets, test_regex = selected_targets([], all_targets=True)
        self.assertEqual(targets, list(FULL_BUILD_TARGETS))
        self.assertEqual(selected_modules([], all_targets=True), list(MODULE_TARGETS))
        self.assertEqual(
            test_regex,
            "|".join(targets[1] for targets in MODULE_TARGETS.values()),
        )
