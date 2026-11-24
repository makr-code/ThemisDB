"""Static catalog contracts; no server or performance workload is run."""

import ast
import json
import unittest
from pathlib import Path

import jsonschema

from benchmarks import benchmark_batch_operations as batch
from benchmarks import core_http_aql_index as aql
from benchmarks import core_http_mix as mix


ROOT = Path(__file__).resolve().parents[2]
BENCHMARKS = ROOT / "benchmarks"
CRUD_ID = "themisdb:e2e/core-http-crud-mix@1"
AQL_ID = "themisdb:e2e/core-http-aql-index-equality@1"


def load(name):
    return json.loads((BENCHMARKS / name).read_text(encoding="utf-8"))


def cli_defaults(script):
    tree = ast.parse((BENCHMARKS / script).read_text(encoding="utf-8"))
    defaults = {}
    for node in ast.walk(tree):
        if (isinstance(node, ast.Call) and isinstance(node.func, ast.Attribute)
                and node.func.attr == "add_argument" and node.args
                and isinstance(node.args[0], ast.Constant)):
            for keyword in node.keywords:
                if keyword.arg == "default":
                    defaults[node.args[0].value] = ast.literal_eval(keyword.value)
    return defaults


class WorkloadProfilesTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.catalog = load("workload_profiles.json")
        cls.suite_map = load("benchmark_suite_map.json")
        cls.profiles = {p["workload_id"]: p for p in cls.catalog["profiles"]}

    def test_schema_unique_ids_classes_and_sources(self):
        schema = load("workload_profiles.schema.json")
        jsonschema.Draft202012Validator.check_schema(schema)
        jsonschema.Draft202012Validator(schema, format_checker=jsonschema.FormatChecker()).validate(self.catalog)
        self.assertEqual(len(self.profiles), len(self.catalog["profiles"]))
        for profile in self.catalog["profiles"]:
            self.assertTrue(profile["workload_id"].startswith(f"themisdb:{profile['workload_class']}/"))
            for source in profile["sources"]:
                self.assertTrue((ROOT / source).is_file(), source)

    def test_suite_references_and_http_relationship_are_separate(self):
        self.assertEqual(self.suite_map["workload_profile_catalog"], "workload_profiles.json")
        self.assertEqual(self.suite_map["suites"]["core"]["binary_regex"],
                         "bench_(crud|mvcc|wal_stress|simple_insert_test|cross_functional_end_to_end|latency_comprehensive|scalability_comprehensive)")
        self.assertEqual(self.suite_map["suites"]["index"]["binary_regex"],
                         "bench_(ann_.*|cuda_vs_cpu|simd_distance|category_b_gpu_cpu_parity|tensor_cpu_gpu_dispatch|gpu_backends)")
        expected = {"core": {CRUD_ID, AQL_ID}, "index": {AQL_ID}}
        for name, suite in self.suite_map["suites"].items():
            binary_ids = suite["workload_profile_ids"]
            http_ids = suite.get("related_http_workload_profile_ids", [])
            for ids in (binary_ids, http_ids):
                self.assertEqual(len(ids), len(set(ids)), name)
                self.assertTrue(set(ids) <= self.profiles.keys(), name)
            self.assertEqual(set(http_ids), expected.get(name, set()))
            self.assertFalse(set(binary_ids) & {CRUD_ID, AQL_ID})

    def test_observed_case_links_are_exact_source_verified_targets(self):
        links = self.suite_map["observed_case_workload_links"]
        self.assertEqual(set(links), {"bench_p0_crud_baseline", "bench_mmdb"})
        for target, link in links.items():
            self.assertIn(link["workload_id"], self.profiles)
            self.assertTrue((ROOT / link["source"]).is_file())
            self.assertTrue(link["exact_observed_cases"])
            profile = self.profiles[link["workload_id"]]
            self.assertIn(link["source"], profile["sources"])

    def test_http_profiles_match_code_defaults_and_report_identity(self):
        crud, indexed = self.profiles[CRUD_ID], self.profiles[AQL_ID]
        self.assertEqual((mix.WORKLOAD_ID, aql.WORKLOAD_ID), (CRUD_ID, AQL_ID))
        self.assertNotEqual(mix.WORKLOAD_ID, aql.WORKLOAD_ID)
        defaults = cli_defaults("core_http_mix.py")
        self.assertEqual(crud["dataset"]["records_per_operation_default"], defaults["--records"])
        self.assertEqual(crud["concurrency"]["default"], defaults["--concurrency"])
        self.assertEqual(crud["concurrency"]["maximum"], mix.MAX_CONCURRENCY)
        self.assertEqual(crud["duration"]["request_timeout_seconds_default"], defaults["--timeout"])
        defaults = cli_defaults("core_http_aql_index.py")
        self.assertEqual(indexed["dataset"]["records_default"], defaults["--records"])
        self.assertEqual(indexed["dataset"]["queries_default"], defaults["--queries"])
        self.assertEqual(indexed["duration"]["request_timeout_seconds_default"], defaults["--timeout"])
        for profile in (crud, indexed):
            self.assertEqual(profile["status"], "source-described")
            self.assertEqual(profile["seed"]["value"], batch.RANDOM_SEED)
            self.assertEqual(profile["warmup"]["batch_writes"], batch.WARMUP_WRITES)
            self.assertEqual(profile["warmup"]["sequential_reads"], batch.WARMUP_SEQUENTIAL_READS)
            self.assertEqual(profile["warmup"]["seeded_random_reads"], batch.WARMUP_RANDOM_READS)
            self.assertFalse(profile["semantics"]["external_comparison_ready"])
        self.assertEqual(crud["semantics"]["phases"], ["load", "warmup", "measurement", "validation", "cleanup"])
        self.assertEqual(indexed["semantics"]["phases"],
                         ["load_and_index_setup", "warmup_and_plan_check", "measurement", "validation", "cleanup"])
        self.assertFalse(indexed["metrics"]["raw_latency_samples_in_report"])

    def test_http_documented_limits_are_enforced_without_io(self):
        crud, indexed = self.profiles[CRUD_ID], self.profiles[AQL_ID]
        mix.CoreHttpMix("http://example.test", records=1, concurrency=crud["concurrency"]["maximum"])
        with self.assertRaises(ValueError):
            mix.CoreHttpMix("http://example.test", records=1, concurrency=crud["concurrency"]["maximum"] + 1)
        limits = indexed["dataset"]
        aql.AqlIndexBenchmark("http://example.test", records=limits["records_maximum"],
                              queries=limits["queries_maximum"])
        for records, queries in ((0, 1), (limits["records_maximum"] + 1, 1),
                                 (1, 0), (1, limits["queries_maximum"] + 1)):
            with self.subTest(records=records, queries=queries), self.assertRaises(ValueError):
                aql.AqlIndexBenchmark("http://example.test", records=records, queries=queries)
        for profile in (crud, indexed):
            self.assertIn("Optional", profile["metrics"]["process_resources"])
            self.assertIn("--server-pid", profile["evidence_outputs"][1]["availability"])

    def test_http_profiles_documented_and_linked(self):
        catalog_doc = (BENCHMARKS / "SOURCE_CATALOG.md").read_text(encoding="utf-8")
        for workload_id, doc in ((CRUD_ID, "CORE_HTTP_MIX.md"), (AQL_ID, "CORE_HTTP_AQL_INDEX.md")):
            with self.subTest(workload_id=workload_id):
                self.assertIn(workload_id, catalog_doc)
                self.assertIn(workload_id, (BENCHMARKS / "docs" / doc).read_text(encoding="utf-8"))
                self.assertIn(f"benchmarks/docs/{doc}", self.profiles[workload_id]["sources"])


if __name__ == "__main__":
    unittest.main()
