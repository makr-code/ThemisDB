from __future__ import annotations

from scripts.sync_soll_ist_gaps import ModuleImplStatus, build_concrete_acceptance_criteria, build_concrete_module_work_items, build_issue_title


def test_build_concrete_module_work_items_uses_real_findings() -> None:
    row = {
        "docs": {"gap_open": False, "missing_doc_types": []},
        "implementation": {"actionable_total": 2, "gap_open": True, "severity_counts": {"CRITICAL": 1, "HIGH": 1, "MEDIUM": 0}},
        "release_gates": {"gap_open": False, "tests_missing": False, "benchmarks_missing": False, "failing_tests": 0, "failing_benchmarks": 0},
        "release_gate_details": {"test_files": [], "benchmark_files": [], "failing_tests": [], "failing_benchmarks": []},
        "developer_docs_alignment": {"status": "ok", "missing_core_docs": []},
    }
    impl_status = ModuleImplStatus(
        module="network",
        actionable_total=2,
        severity_counts={"CRITICAL": 1, "HIGH": 1, "MEDIUM": 0},
        findings=[
            {
                "file": "src/network/qos_manager.cpp",
                "line": 663,
                "classification": "command_injection",
                "verified_severity": "CRITICAL",
                "pattern": "std::system(cmd) with user-supplied iface string",
            },
            {
                "file": "src/network/socket_timeout_manager.cpp",
                "line": 71,
                "classification": "missing_dtor",
                "verified_severity": "HIGH",
                "pattern": "destructor missing timeout cleanup",
            },
        ],
    )

    tasks = build_concrete_module_work_items("network", row, impl_status, row["release_gates"], row["docs"], row["developer_docs_alignment"], row["release_gate_details"])

    assert any("qos_manager.cpp" in task for task in tasks)
    assert any("std::system" in task for task in tasks)
    assert any("socket_timeout_manager.cpp" in task for task in tasks)
    assert any("regression" in task.lower() for task in tasks)


def test_build_concrete_acceptance_criteria_includes_module_specific_validation() -> None:
    row = {
        "docs": {"gap_open": False},
        "implementation": {"actionable_total": 1, "gap_open": True, "severity_counts": {"CRITICAL": 1, "HIGH": 0, "MEDIUM": 0}},
        "release_gates": {"gap_open": False, "tests_missing": False, "failing_tests": 0, "failing_benchmarks": 0},
    }
    impl_status = ModuleImplStatus(
        module="llm",
        actionable_total=1,
        severity_counts={"CRITICAL": 1, "HIGH": 0, "MEDIUM": 0},
        findings=[
            {
                "file": "src/llm/model_downloader.cpp",
                "line": 120,
                "classification": "path_traversal",
                "verified_severity": "CRITICAL",
                "pattern": "model path accepted from untrusted input",
            }
        ],
    )

    criteria = build_concrete_acceptance_criteria("llm", row, impl_status, row["release_gates"], row["docs"])

    assert any("model_downloader.cpp" in c for c in criteria)
    assert any("ctest" in c.lower() for c in criteria)
    assert any("path traversal" in c.lower() or "untrusted input" in c.lower() for c in criteria)


def test_build_concrete_module_work_items_docs_only_is_specific() -> None:
    row = {
        "docs": {
            "gap_open": True,
            "missing_doc_types": ["README", "ROADMAP", "ARCH", "CHANGELOG", "FUTURE"],
            "status": "LOW",
            "score_percent": 30,
        },
        "implementation": {"actionable_total": 0, "gap_open": False, "severity_counts": {"CRITICAL": 0, "HIGH": 0, "MEDIUM": 0}},
        "release_gates": {"gap_open": False, "tests_missing": False, "benchmarks_missing": False, "failing_tests": 0, "failing_benchmarks": 0},
        "developer_docs_alignment": {"status": "stale", "missing_core_docs": ["CHANGELOG.md", "FUTURE_ENHANCEMENTS.md"]},
        "release_gate_details": {"test_files": [], "benchmark_files": [], "failing_tests": [], "failing_benchmarks": []},
    }
    tasks = build_concrete_module_work_items(
        "access_model",
        row,
        None,
        row["release_gates"],
        row["docs"],
        row["developer_docs_alignment"],
        row["release_gate_details"],
    )
    assert any("README" in task for task in tasks)
    assert any("ARCHITECTURE" in task for task in tasks)
    assert any("CHANGELOG" in task for task in tasks)


def test_build_issue_title_uses_action_root_cause() -> None:
    row = {
        "docs": {"gap_open": False, "missing_doc_types": []},
        "implementation": {"actionable_total": 1, "gap_open": True},
        "release_gates": {"gap_open": False, "tests_missing": False, "benchmarks_missing": False, "failing_tests": 0, "failing_benchmarks": 0},
    }
    impl_status = ModuleImplStatus(
        module="network",
        actionable_total=1,
        severity_counts={"CRITICAL": 1, "HIGH": 0, "MEDIUM": 0},
        findings=[{
            "file": "src/network/qos_manager.cpp",
            "line": 663,
            "classification": "command_injection",
            "pattern": "std::system(cmd) with user-supplied iface string",
        }],
    )

    title = build_issue_title("network", row, impl_status, row["release_gates"], row["docs"])
    assert "remove shell-command injection risk" in title.lower()
    assert "qos_manager" in title.lower()


def test_build_issue_title_for_docs_gap_is_action_oriented() -> None:
    row = {
        "docs": {"gap_open": True, "missing_doc_types": ["README", "ARCHITECTURE", "CHANGELOG"]},
        "implementation": {"actionable_total": 0, "gap_open": False},
        "release_gates": {"gap_open": False, "tests_missing": False, "benchmarks_missing": False, "failing_tests": 0, "failing_benchmarks": 0},
    }

    title = build_issue_title("access_model", row, None, row["release_gates"], row["docs"])
    assert "restore missing governance docs" in title.lower()
    assert "readme" in title.lower()
    assert "architecture" in title.lower()
