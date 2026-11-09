#!/usr/bin/env python3
"""Select focused ASan/UBSan tests for modules changed in a pull request."""

from __future__ import annotations

import argparse


MODULE_TARGETS = {
    "storage": (
        "module_storage_test_storage_contract_hardening_focused_focused",
        "test_storage_contract_hardening_focused_StorageFocusedTests",
    ),
    "transaction": (
        "test_transaction_auditor_focused",
        "TransactionAuditorFocusedTests",
    ),
    "query": ("test_query_engine_focused", "QueryEngineFocusedTests"),
    "aql": (
        "module_aql_test_aql_query_validator_focused",
        "test_aql_query_validator_AqlFocusedTests",
    ),
    "server": (
        "module_server_test_server_phase2_focused_focused",
        "test_server_phase2_focused_server_FocusedTests",
    ),
    "auth": (
        "module_auth_test_wave8_auth_hardening_focused",
        "test_wave8_auth_hardening_auth_FocusedTests",
    ),
    "network": (
        "module_network_test_network_contract_hardening_focused_focused",
        "test_network_contract_hardening_focused_network_FocusedTests",
    ),
    "wire_protocol": (
        "module_network_test_wire_protocol_v2_focused",
        "test_wire_protocol_v2_network_FocusedTests",
    ),
    "sharding": ("test_shard_router_focused", "ShardRouterFocusedTests"),
}

FULL_BUILD_TARGETS = (
    "themis_base",
    *(targets[0] for targets in MODULE_TARGETS.values()),
)
FULL_TEST_REGEX = "|".join(targets[1] for targets in MODULE_TARGETS.values())


def _is_under(path: str, *prefixes: str) -> bool:
    return any(path.startswith(prefix) for prefix in prefixes)


def affected_modules(paths: list[str]) -> list[str]:
    """Return sanitizer test groups matching changed repository-relative paths."""
    modules: set[str] = set()
    for path in paths:
        normalized = path.replace("\\", "/").removeprefix("./")
        if normalized == "CMakeLists.txt":
            return list(MODULE_TARGETS)
        if _is_under(
            normalized,
            "src/storage/",
            "include/storage/",
            "tests/storage/",
        ):
            modules.add("storage")
        if _is_under(
            normalized,
            "src/transaction/",
            "include/transaction/",
            "tests/transaction/",
        ) or normalized == "tests/test_transaction_auditor.cpp":
            modules.add("transaction")
        if _is_under(
            normalized,
            "src/query/",
            "include/query/",
            "tests/query/",
        ) or normalized == "tests/test_query_engine.cpp":
            modules.add(
                "aql"
                if normalized.startswith(
                    (
                        "src/query/aql_parser",
                        "include/query/aql_parser",
                        "tests/query/aql_parser",
                    )
                )
                else "query"
            )
        if _is_under(normalized, "src/aql/", "include/aql/", "tests/aql/"):
            modules.add("aql")
        if _is_under(
            normalized,
            "src/server/",
            "include/server/",
            "tests/server/",
        ):
            modules.add("server")
        if _is_under(normalized, "src/auth/", "include/auth/", "tests/auth/"):
            modules.add("auth")
        if _is_under(
            normalized,
            "src/network/",
            "include/network/",
            "tests/network/",
        ) or normalized in (
            "tests/test_themis_wire_protocol_server.cpp",
            "tests/themis/test_themis_wire_protocol_server.cpp",
        ):
            modules.add("network")
        if (
            normalized.startswith(
                (
                    "src/network/wire_protocol",
                    "src/network/websocket_binary_frame_parser",
                    "include/network/wire_protocol",
                    "tests/network/test_wire_protocol",
                    "tests/themis/test_themis_wire_protocol",
                )
            )
            or normalized
            == "tests/test_themis_wire_protocol_server.cpp"
        ):
            modules.add("wire_protocol")
        if _is_under(
            normalized,
            "src/sharding/",
            "include/sharding/",
            "tests/sharding/",
        ) or normalized == "tests/test_shard_router_focused.cpp":
            modules.add("sharding")
    return [module for module in MODULE_TARGETS if module in modules]


def selected_targets(
    paths: list[str], *, all_targets: bool = False
) -> tuple[list[str], str]:
    """Return build targets and a CTest regex for the selected changed-module scope."""
    modules = selected_modules(paths, all_targets=all_targets)
    full_suite = all_targets or len(modules) == len(MODULE_TARGETS)
    build_targets = list(FULL_BUILD_TARGETS) if full_suite else [
        MODULE_TARGETS[module][0] for module in modules
    ]
    test_regex = FULL_TEST_REGEX if full_suite else "|".join(
        MODULE_TARGETS[module][1] for module in modules
    )
    return build_targets, test_regex


def selected_modules(paths: list[str], *, all_targets: bool = False) -> list[str]:
    """Return sanitizer module labels for the selected PR or full nightly scope."""
    modules = list(MODULE_TARGETS) if all_targets else affected_modules(paths)
    if not modules and any(
        path.replace("\\", "/").removeprefix("./")
        == ".github/workflows/build-sanitizer-nightly.yml"
        for path in paths
    ):
        modules = ["wire_protocol"]
    return modules


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument(
        "--mode", choices=("build", "regex", "modules"), required=True
    )
    parser.add_argument("--all", action="store_true")
    parser.add_argument("--changed-files", nargs="*", default=[])
    args = parser.parse_args()
    build_targets, test_regex = selected_targets(
        args.changed_files, all_targets=args.all
    )
    if args.mode == "build":
        print("\n".join(build_targets))
    elif args.mode == "regex":
        print(test_regex)
    else:
        print(",".join(selected_modules(args.changed_files, all_targets=args.all)))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
