#!/usr/bin/env python3
"""Focused tests for the generated benchmark module evidence matrix."""

import sys
import tempfile
import unittest
from pathlib import Path

REPO_ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(REPO_ROOT / "tools" / "ci"))

import audit_benchmark_coverage


class BenchmarkModuleMatrixTests(unittest.TestCase):
    def test_maps_sources_targets_and_unassessed_modules(self):
        with tempfile.TemporaryDirectory() as temp_dir:
            repo = Path(temp_dir)
            (repo / "src" / "alpha").mkdir(parents=True)
            (repo / "src" / "beta").mkdir()
            benchmark_dir = repo / "benchmarks" / "alpha"
            benchmark_dir.mkdir(parents=True)
            (benchmark_dir / "bench_alpha.cpp").write_text("int main() { return 0; }\n")
            (repo / "benchmarks" / "shared").mkdir()
            (repo / "benchmarks" / "shared" / "benchmark_shared.cpp").write_text(
                "int main() { return 0; }\n"
            )
            (benchmark_dir / "CMakeLists.txt").write_text(
                "themis_add_standard_benchmark(alpha_target bench_alpha.cpp)\n"
                "themis_register_benchmark_ctest("
                "NAME \"alpha_smoke\" TARGET \"alpha_target\" "
                "EXTRA_LABELS release_critical)\n"
            )

            matrix = audit_benchmark_coverage.generate_module_matrix(
                repo,
                source_revision="develop-test-sha",
                source_branch="develop",
                latest_attempt={
                    "run_url": "https://example.invalid/run/1",
                    "run_sha": "run-sha",
                    "run_conclusion": "failure",
                    "benchmark_results_available": False,
                },
            )

        alpha = next(module for module in matrix["modules"] if module["module"] == "alpha")
        beta = next(module for module in matrix["modules"] if module["module"] == "beta")

        self.assertEqual(matrix["source_snapshot"], {
            "revision": "develop-test-sha",
            "branch": "develop",
            "modules_root": "src/",
            "benchmark_root": "benchmarks/",
        })
        self.assertEqual(alpha["benchmark_source_count"], 1)
        self.assertEqual(alpha["benchmarks"][0]["targets"], ["alpha_target"])
        self.assertEqual(alpha["benchmarks"][0]["ctest_entries"], ["alpha_smoke"])
        self.assertTrue(alpha["benchmarks"][0]["release_critical"])
        self.assertEqual(alpha["priority"], "high-risk")
        self.assertEqual(alpha["benchmarks"][0]["status"], "blocked")
        self.assertEqual(beta["benchmark_source_count"], 0)
        self.assertEqual(beta["status"], "gap")
        self.assertEqual(matrix["unassigned_benchmark_sources"], [
            "benchmarks/shared/benchmark_shared.cpp"
        ])

    def test_does_not_promote_a_claim_without_workload_evidence(self):
        with tempfile.TemporaryDirectory() as temp_dir:
            repo = Path(temp_dir)
            (repo / "src" / "query").mkdir(parents=True)
            benchmark_dir = repo / "benchmarks" / "query"
            benchmark_dir.mkdir(parents=True)
            (benchmark_dir / "bench_query_limit_scaling.cpp").write_text(
                "int main() { return 0; }\n"
            )
            (benchmark_dir / "CMakeLists.txt").write_text(
                "themis_add_standard_benchmark(query_limit bench_query_limit_scaling.cpp)\n"
                "themis_register_benchmark_ctest(NAME query_limit_smoke TARGET query_limit)\n"
            )
            (repo / "src" / "query" / "PERFORMANCE_EXPECTATIONS.md").write_text(
                "# Query performance expectations\n"
            )
            (repo / "benchmarks" / "benchmark_target_mapping.json").write_text(
                '{"modules":{"query":{"Q-Limit":{"label":"LIMIT scan scale",'
                '"file":"bench_query_limit_scaling.cpp",'
                '"primary_case":"BM_QueryLimit_10_10M",'
                '"slo_target":"10M rows","status":"mapped"}}}}'
            )

            matrix = audit_benchmark_coverage.generate_module_matrix(
                repo,
                source_revision="develop-test-sha",
                latest_attempt={
                    "run_url": "https://example.invalid/run/2",
                    "run_sha": "run-sha",
                    "run_conclusion": "success",
                    "benchmark_results_available": True,
                },
            )

        query = next(module for module in matrix["modules"] if module["module"] == "query")
        withdrawn_llm_claim = next(
            claim for claim in matrix["release_claims"]
            if claim["id"] == "llm-1024-concurrency-claim"
        )

        self.assertEqual(query["benchmarks"][0]["status"], "gap")
        self.assertEqual(query["claim_review"]["status"], "gap")
        self.assertEqual(query["claim_inventory_review"]["status"], "gap")
        self.assertEqual(query["claim_inventory_review"]["owner_issue"], 7488)
        self.assertEqual(
            query["claim_inventory_review"]["mapped_performance_targets"][0],
            {
                "target_id": "Q-Limit",
                "label": "LIMIT scan scale",
                "benchmark_file": "bench_query_limit_scaling.cpp",
                "primary_case": "BM_QueryLimit_10_10M",
                "fallback_file": None,
                "fallback_case": None,
                "declared_target": "10M rows",
                "mapping_status": "mapped",
            },
        )
        self.assertIn(
            "partial index",
            query["claim_inventory_review"]["mapping_completeness"],
        )
        self.assertEqual(withdrawn_llm_claim["claim_disposition"], "withdrawn")
        self.assertEqual(withdrawn_llm_claim["status"], "not applicable")
        self.assertEqual(withdrawn_llm_claim["benchmark_applicability"], "not applicable")

    def test_unmapped_high_risk_claims_keep_portfolio_owner_and_gap(self):
        with tempfile.TemporaryDirectory() as temp_dir:
            repo = Path(temp_dir)
            (repo / "src" / "server").mkdir(parents=True)
            (repo / "src" / "server" / "PERFORMANCE_EXPECTATIONS.md").write_text(
                "# Server performance expectations\n"
            )
            benchmark_dir = repo / "benchmarks" / "server"
            benchmark_dir.mkdir(parents=True)
            (benchmark_dir / "bench_server_hotpaths.cpp").write_text(
                "int main() { return 0; }\n"
            )

            matrix = audit_benchmark_coverage.generate_module_matrix(
                repo,
                source_revision="develop-test-sha",
            )

        server = next(module for module in matrix["modules"] if module["module"] == "server")
        review = server["claim_inventory_review"]
        self.assertEqual(review["status"], "gap")
        self.assertEqual(review["owner_issue"], 7828)
        self.assertEqual(review["source_documents"], [
            "src/server/PERFORMANCE_EXPECTATIONS.md"
        ])
        self.assertEqual(review["mapped_performance_targets"], [])
        self.assertEqual(review["benchmark_source_candidates"], [
            "benchmarks/server/bench_server_hotpaths.cpp"
        ])
        self.assertIn("retained raw result", review["follow_up"])


if __name__ == "__main__":
    unittest.main()
