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
                "themis_register_benchmark_ctest(NAME alpha_smoke TARGET alpha_target)\n"
            )

            matrix = audit_benchmark_coverage.generate_module_matrix(
                repo,
                source_revision="develop-test-sha",
                source_branch="develop",
                latest_attempt={
                    "run_url": "https://example.invalid/run/1",
                    "result_sha": "run-sha",
                    "result": "failure",
                    "benchmark_results_available": False,
                    "workload_metadata_complete": False,
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

            matrix = audit_benchmark_coverage.generate_module_matrix(
                repo,
                source_revision="develop-test-sha",
                latest_attempt={
                    "run_url": "https://example.invalid/run/2",
                    "result_sha": "run-sha",
                    "result": "success",
                    "benchmark_results_available": True,
                    "workload_metadata_complete": False,
                },
            )

        query = next(module for module in matrix["modules"] if module["module"] == "query")
        withdrawn_llm_claim = next(
            claim for claim in matrix["release_claims"]
            if claim["id"] == "llm-1024-concurrency-claim"
        )

        self.assertEqual(query["benchmarks"][0]["status"], "gap")
        self.assertEqual(query["claim_review"]["status"], "gap")
        self.assertEqual(withdrawn_llm_claim["claim_disposition"], "withdrawn")
        self.assertEqual(withdrawn_llm_claim["benchmark_applicability"], "not applicable")


if __name__ == "__main__":
    unittest.main()
