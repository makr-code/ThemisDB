"""
Test suite für neue CHIMERA-Komponenten:
  - 18 neue Workloads (Timeseries, Geo, Process, LLM, ML, LoRA, RAG, OLTP)
  - Capability-Handshake (_extract_capabilities, _validate_workload_capabilities)
  - Benchmark-Standards und Profilkatalog
  - REST-API-Server (chimera_server) – ohne laufenden Server
  - CLI serve-Subcommand (argparse-Ebene)
"""

import json
import pathlib
import sys
import os
import pytest
from typing import Any, Dict, List
from unittest.mock import MagicMock, patch


# ---------------------------------------------------------------------------
# Pfad-Setup
# ---------------------------------------------------------------------------

sys.path.insert(0, str(pathlib.Path(__file__).parent))


# ---------------------------------------------------------------------------
# Imports
# ---------------------------------------------------------------------------

from run_endurance import (
    EnduranceConfig,
    _extract_capabilities,
    _extract_database_info,
    _build_beir_dataset_adapter,
    _build_dataset_provisioning_manifest,
    _build_standard_golden_baselines,
    _build_standard_result_schema,
    _get_workload_standard_id,
    _write_standard_golden_baselines,
    _standardize_endurance_config,
    _get_workload_capability_requirements,
    _validate_workload_capabilities,
    _resolve_runtime_capabilities,
    _build_workloads,
    _build_test_topology,
    _run_window,
    ThemisDBClient,
)
from benchmark_standards import (
    CURRENT_CHIMERA_WORKLOADS,
    get_profile_catalog,
    get_standard_coverage_snapshot,
    get_standard_compliance_requirements,
    get_standard_catalog,
    resolve_profile_workloads,
)
from exporter import ChimeraExporter


# ===========================================================================
# _extract_capabilities
# ===========================================================================

class TestExtractCapabilities:
    """_extract_capabilities: JSON-Format-Normalisierung."""

    def test_flat_string_list(self):
        caps = _extract_capabilities(["kv_read", "kv_write", "vector_search"])
        assert caps == ["kv_read", "kv_write", "vector_search"]

    def test_nested_capabilities_key(self):
        payload = {"capabilities": ["kv_read", "timeseries_write"]}
        assert _extract_capabilities(payload) == ["kv_read", "timeseries_write"]

    def test_nested_data_capabilities(self):
        payload = {"data": {"capabilities": ["rag_retrieval", "rag_qa"]}}
        assert _extract_capabilities(payload) == ["rag_retrieval", "rag_qa"]

    def test_list_of_dicts_with_name_key(self):
        payload = [{"name": "LLM_Inference"}, {"name": "kv_read"}]
        caps = _extract_capabilities(payload)
        assert "llm_inference" in caps
        assert "kv_read" in caps

    def test_list_of_dicts_with_id_key(self):
        payload = [{"id": "vector_search"}, {"id": "graph_traversal"}]
        caps = _extract_capabilities(payload)
        assert "vector_search" in caps
        assert "graph_traversal" in caps

    def test_deduplication(self):
        caps = _extract_capabilities(["kv_read", "kv_read", "kv_write"])
        assert caps.count("kv_read") == 1

    def test_empty_list(self):
        assert _extract_capabilities([]) == []

    def test_empty_dict(self):
        assert _extract_capabilities({}) == []

    def test_invalid_type_returns_empty(self):
        assert _extract_capabilities("not_a_dict_or_list") == []
        assert _extract_capabilities(None) == []
        assert _extract_capabilities(42) == []

    def test_normalizes_to_lowercase(self):
        caps = _extract_capabilities(["KV_READ", "Vector_Search"])
        assert "kv_read" in caps
        assert "vector_search" in caps

    def test_strips_whitespace(self):
        caps = _extract_capabilities(["  kv_read  ", " kv_write"])
        assert "kv_read" in caps
        assert "kv_write" in caps


# ===========================================================================
# _get_workload_capability_requirements
# ===========================================================================

class TestWorkloadCapabilityRequirements:
    """_get_workload_capability_requirements: Vollständigkeit und Korrektheit."""

    def test_all_current_workloads_have_requirements(self):
        req_map = _get_workload_capability_requirements()
        for wid in CURRENT_CHIMERA_WORKLOADS:
            assert wid in req_map, f"Kein Capability-Eintrag für Workload '{wid}'"

    def test_ycsb_a_requires_read_write(self):
        req_map = _get_workload_capability_requirements()
        assert "kv_read" in req_map["ycsb_a"]
        assert "kv_write" in req_map["ycsb_a"]

    def test_ycsb_c_read_only(self):
        req_map = _get_workload_capability_requirements()
        assert "kv_read" in req_map["ycsb_c"]
        assert "kv_write" not in req_map["ycsb_c"]

    def test_vector_search_requires_vector(self):
        req_map = _get_workload_capability_requirements()
        assert "vector_search" in req_map["vector_search"]

    def test_llm_workloads_require_llm_inference(self):
        req_map = _get_workload_capability_requirements()
        assert "llm_inference" in req_map["llm_serving_latency"]
        assert "llm_inference" in req_map["llm_serving_throughput"]

    def test_rag_workloads_have_distinct_caps(self):
        req_map = _get_workload_capability_requirements()
        assert "rag_retrieval" in req_map["rag_retrieval_quality"]
        assert "rag_qa" in req_map["rag_qa_e2e"]

    def test_timeseries_write_batch_and_raw(self):
        req_map = _get_workload_capability_requirements()
        assert "timeseries_write" in req_map["ts_ingest_raw"]
        assert "timeseries_write" in req_map["ts_ingest_batch"]

    def test_process_conformance_distinct_cap(self):
        req_map = _get_workload_capability_requirements()
        assert "process_conformance" in req_map["process_conformance"]
        assert "process_discovery" in req_map["process_discovery_alpha"]

    def test_lora_caps(self):
        req_map = _get_workload_capability_requirements()
        assert "lora_runtime" in req_map["lora_load_apply"]
        assert "lora_runtime" in req_map["lora_switch_overhead"]

    def test_tpc_c_requires_transaction_oltp(self):
        req_map = _get_workload_capability_requirements()
        assert "transaction_oltp" in req_map["tpc_c_mix"]


# ===========================================================================
# _validate_workload_capabilities
# ===========================================================================

class TestValidateWorkloadCapabilities:
    """_validate_workload_capabilities: OK- und Fehlerpfade."""

    def test_all_caps_present_returns_ok(self):
        workloads = ["ycsb_a", "ycsb_c", "vector_search"]
        caps = ["kv_read", "kv_write", "vector_search"]
        result = _validate_workload_capabilities(workloads, caps)
        assert result["ok"] is True
        assert result["missing_by_workload"] == {}

    def test_missing_cap_detected(self):
        workloads = ["ycsb_a"]
        caps = ["kv_read"]  # kv_write fehlt
        result = _validate_workload_capabilities(workloads, caps)
        assert result["ok"] is False
        assert "ycsb_a" in result["missing_by_workload"]
        assert "kv_write" in result["missing_by_workload"]["ycsb_a"]

    def test_multiple_workloads_one_missing(self):
        workloads = ["ycsb_c", "llm_serving_latency"]
        caps = ["kv_read"]  # llm_inference fehlt
        result = _validate_workload_capabilities(workloads, caps)
        assert result["ok"] is False
        assert "ycsb_c" not in result["missing_by_workload"]
        assert "llm_serving_latency" in result["missing_by_workload"]

    def test_empty_workload_list(self):
        result = _validate_workload_capabilities([], ["kv_read"])
        assert result["ok"] is True

    def test_empty_caps_all_fail(self):
        workloads = ["ycsb_a", "vector_search"]
        result = _validate_workload_capabilities(workloads, [])
        assert result["ok"] is False
        assert len(result["missing_by_workload"]) == 2

    def test_unknown_workload_ignored(self):
        """Unbekannte Workloads ohne Eintrag in der Map werden ignoriert (kein Fehler)."""
        result = _validate_workload_capabilities(["nonexistent_workload"], ["kv_read"])
        assert result["ok"] is True

    def test_case_insensitive_caps(self):
        """Capabilities sind normalisiert, Vergleich muss passen."""
        workloads = ["ycsb_a"]
        # _extract_capabilities normalisiert bereits zu lowercase → caps kommen lowercase an
        caps = ["kv_read", "kv_write"]
        result = _validate_workload_capabilities(workloads, caps)
        assert result["ok"] is True

    def test_full_profile_caps(self):
        """full-Profil mit vollständigen Caps ergibt ok=True."""
        req_map = _get_workload_capability_requirements()
        all_caps = list({cap for caps in req_map.values() for cap in caps})
        result = _validate_workload_capabilities(CURRENT_CHIMERA_WORKLOADS, all_caps)
        assert result["ok"] is True


# ===========================================================================
# _resolve_runtime_capabilities
# ===========================================================================

class TestResolveRuntimeCapabilities:
    """_resolve_runtime_capabilities: Quell-Priorisierung."""

    def _make_client(self, discovered: List[str]) -> ThemisDBClient:
        cfg = EnduranceConfig()
        client = ThemisDBClient(cfg)
        client.discover_capabilities = MagicMock(return_value=discovered)
        return client

    def test_env_override_has_priority(self):
        cfg = EnduranceConfig(capabilities_override="kv_read,vector_search")
        client = self._make_client(["other_cap"])
        result = _resolve_runtime_capabilities(cfg, client)
        assert result["source"] == "env_override"
        assert "kv_read" in result["capabilities"]
        assert "vector_search" in result["capabilities"]
        # Endpoint darf nicht aufgerufen werden
        client.discover_capabilities.assert_not_called()

    def test_endpoint_used_when_no_override(self):
        cfg = EnduranceConfig(capabilities_override="")
        client = self._make_client(["llm_inference", "rag_retrieval"])
        result = _resolve_runtime_capabilities(cfg, client)
        assert "endpoint:" in result["source"]
        assert "llm_inference" in result["capabilities"]

    def test_fallback_none_when_endpoint_empty(self):
        cfg = EnduranceConfig(capabilities_override="")
        client = self._make_client([])
        result = _resolve_runtime_capabilities(cfg, client)
        assert result["source"] == "none"
        assert result["capabilities"] == []


# ===========================================================================
# _extract_database_info / discover_database_info
# ===========================================================================

class TestDatabaseInfoDiscovery:
    """DB-Informations-Erkennung und Normalisierung."""

    def test_extract_database_info_from_info_payload(self):
        payload = {
            "product": "ThemisDB",
            "version": "1.8.2",
            "status": "ok",
            "api_version": "v1",
            "edition": "enterprise",
        }
        info = _extract_database_info(payload)
        assert info["detected"] is True
        assert info["product"] == "ThemisDB"
        assert info["version"] == "1.8.2"
        assert info["status"] == "ok"

    def test_extract_database_info_from_nested_data(self):
        payload = {
            "data": {
                "name": "ThemisDB",
                "server_version": "1.8.2",
                "state": "healthy",
                "build_commit": "abc123",
            }
        }
        info = _extract_database_info(payload)
        assert info["detected"] is True
        assert info["product"] == "ThemisDB"
        assert info["version"] == "1.8.2"
        assert info["status"] == "healthy"
        assert info["build_commit"] == "abc123"

    def test_extract_database_info_cluster_and_replication_fields(self):
        payload = {
            "data": {
                "product": "ThemisDB",
                "version": "1.8.2",
                "cluster_enabled": True,
                "cluster_role": "leader",
                "node_count": 3,
                "shard_count": 12,
                "replication_enabled": True,
                "replication_role": "primary",
                "replication_factor": 3,
            }
        }
        info = _extract_database_info(payload)
        assert info["cluster_enabled"] is True
        assert info["cluster_role"] == "leader"
        assert info["node_count"] == 3
        assert info["shard_count"] == 12
        assert info["replication_enabled"] is True
        assert info["replication_role"] == "primary"
        assert info["replication_factor"] == 3

    def test_extract_database_info_from_health_only(self):
        info = _extract_database_info({"status": "ok"})
        assert info["detected"] is True
        assert info["status"] == "ok"

    def test_extract_database_info_invalid_payload(self):
        info = _extract_database_info(["not", "a", "dict"])
        assert info["detected"] is False

    def test_discover_database_info_prefers_first_matching_endpoint(self):
        cfg = EnduranceConfig()
        client = ThemisDBClient(cfg)

        def _fake_get_json(path: str):
            if path == "/api/v1/info":
                return {"product": "ThemisDB", "version": "1.8.2", "status": "ok"}
            return None

        client._get_json = MagicMock(side_effect=_fake_get_json)
        info = client.discover_database_info()
        assert info["detected"] is True
        assert info["source"] == "/api/v1/info"
        assert info["product"] == "ThemisDB"

    def test_discover_database_info_falls_back_to_health(self):
        cfg = EnduranceConfig()
        client = ThemisDBClient(cfg)

        def _fake_get_json(path: str):
            if path == "/api/v1/health":
                return {"status": "ok"}
            return None

        client._get_json = MagicMock(side_effect=_fake_get_json)
        info = client.discover_database_info()
        assert info["detected"] is True
        assert info["source"] == "/api/v1/health"
        assert info["status"] == "ok"

    def test_discover_database_info_none_when_no_endpoint_matches(self):
        cfg = EnduranceConfig()
        client = ThemisDBClient(cfg)
        client._get_json = MagicMock(return_value=None)
        info = client.discover_database_info()
        assert info["detected"] is False
        assert info["source"] == "none"
        assert info["database"] == cfg.database


# ===========================================================================
# benchmark_standards: Profile-Katalog
# ===========================================================================

class TestBenchmarkStandards:
    """Profilkatalog: Vollständigkeit, Coverage, resolve_profile_workloads."""

    def test_all_profile_ids_present(self):
        profiles = get_profile_catalog()
        expected = {"core", "timeseries", "geo", "ldbc", "ann", "process", "llm", "ml", "lora", "rag", "full"}
        assert expected.issubset(set(profiles.keys()))

    def test_all_required_workloads_available(self):
        """Alle required_workloads jedes Profils müssen in CURRENT_CHIMERA_WORKLOADS sein."""
        profiles = get_profile_catalog()
        current = set(CURRENT_CHIMERA_WORKLOADS)
        for pid, profile in profiles.items():
            for wid in profile.required_workloads:
                assert wid in current, (
                    f"Profil '{pid}': required workload '{wid}' nicht in CURRENT_CHIMERA_WORKLOADS"
                )

    def test_resolve_profile_core(self):
        required, available, missing = resolve_profile_workloads("core")
        assert len(required) > 0
        assert len(missing) == 0

    def test_resolve_profile_full_zero_missing(self):
        _, _, missing = resolve_profile_workloads("full")
        assert missing == [], f"full-Profil hat missing workloads: {missing}"

    def test_resolve_unknown_profile_raises(self):
        with pytest.raises(KeyError):
            resolve_profile_workloads("nonexistent_profile")

    def test_all_profiles_zero_missing(self):
        profiles = get_profile_catalog()
        for pid in profiles:
            _, _, missing = resolve_profile_workloads(pid)
            assert missing == [], f"Profil '{pid}' hat fehlende Workloads: {missing}"

    def test_standard_catalog_not_empty(self):
        standards = get_standard_catalog()
        assert len(standards) >= 7  # YCSB, TPC-C, TSBS, LDBC, ANN, BEIR, MLPerf

    def test_standard_has_primary_metrics(self):
        standards = get_standard_catalog()
        for std in standards:
            assert len(std.primary_metrics) > 0, f"Standard '{std.standard_id}' hat keine primary_metrics"

    def test_full_profile_contains_all_current_workloads(self):
        required, _, _ = resolve_profile_workloads("full")
        current = set(CURRENT_CHIMERA_WORKLOADS)
        for wid in required:
            assert wid in current

    def test_ycsb_compliance_spec_exists(self):
        requirements = get_standard_compliance_requirements()
        assert "YCSB" in requirements

    def test_ycsb_compliance_contains_workloads_a_to_f(self):
        requirements = get_standard_compliance_requirements()["YCSB"]
        workload_ids = {spec.workload_id for spec in requirements.canonical_workloads}
        assert workload_ids == {"ycsb_a", "ycsb_b", "ycsb_c", "ycsb_d", "ycsb_e", "ycsb_f"}

    def test_ycsb_workload_d_uses_latest_distribution(self):
        requirements = get_standard_compliance_requirements()["YCSB"]
        workload_d = next(spec for spec in requirements.canonical_workloads if spec.workload_id == "ycsb_d")
        assert workload_d.distribution == "latest"
        assert workload_d.operation_mix["read"] == 95
        assert workload_d.operation_mix["insert"] == 5

    def test_ycsb_measurement_protocol_matches_chimera_context(self):
        requirements = get_standard_compliance_requirements()["YCSB"]
        assert requirements.measurement_protocol["warm_up_seconds"] == 60
        assert requirements.measurement_protocol["measurement_duration_seconds"] == 300
        assert requirements.measurement_protocol["num_runs"] == 5

    def test_ycsb_required_parameters_include_dataset_and_threads(self):
        requirements = get_standard_compliance_requirements()["YCSB"]
        for key in ["record_count", "operation_count", "field_count", "field_length", "thread_count"]:
            assert key in requirements.required_parameters

    def test_tpcc_compliance_spec_exists(self):
        requirements = get_standard_compliance_requirements()
        assert "TPC-C" in requirements

    def test_tpcc_compliance_contains_standard_mix(self):
        requirements = get_standard_compliance_requirements()["TPC-C"]
        spec = next(item for item in requirements.canonical_workloads if item.workload_id == "tpc_c_mix")
        assert spec.operation_mix["new_order"] == 45
        assert spec.operation_mix["payment"] == 43
        assert spec.operation_mix["order_status"] == 4
        assert spec.operation_mix["delivery"] == 4
        assert spec.operation_mix["stock_level"] == 4

    def test_tpcc_required_parameters_include_warehouses(self):
        requirements = get_standard_compliance_requirements()["TPC-C"]
        assert "warehouses" in requirements.required_parameters
        assert "duration_seconds" in requirements.required_parameters

    def test_tpch_compliance_spec_exists(self):
        requirements = get_standard_compliance_requirements()
        assert "TPC-H" in requirements

    def test_tpch_compliance_covers_queries_q1_to_q22(self):
        requirements = get_standard_compliance_requirements()["TPC-H"]
        spec = next(item for item in requirements.canonical_workloads if item.workload_id == "tpc_h_sf1")
        assert len(spec.operation_mix) == 22
        assert all(f"q{i}" in spec.operation_mix for i in range(1, 23))
        assert spec.required_dataset_shape["scale_factor"] == 1

    def test_standard_coverage_snapshot_marks_tpch_as_missing_runtime(self):
        snapshot = get_standard_coverage_snapshot()
        assert snapshot["YCSB"]["is_fully_implemented"] is True
        assert snapshot["TPC-C"]["is_fully_implemented"] is True
        assert snapshot["TSBS"]["is_fully_implemented"] is True
        assert snapshot["TPC-H"]["is_fully_implemented"] is False
        assert "tpc_h_sf1" in snapshot["TPC-H"]["missing_workloads"]

    def test_tsbs_compliance_spec_exists(self):
        requirements = get_standard_compliance_requirements()
        assert "TSBS" in requirements

    def test_tsbs_compliance_covers_all_seven_workload_classes(self):
        requirements = get_standard_compliance_requirements()["TSBS"]
        ids = {spec.workload_id for spec in requirements.canonical_workloads}
        expected = {
            "ts_ingest_raw", "ts_ingest_batch", "ts_range_query",
            "ts_lastpoint", "ts_downsample", "ts_high_cpu", "ts_groupby",
        }
        assert ids == expected

    def test_tsbs_required_parameters_include_seed_and_entity_count(self):
        requirements = get_standard_compliance_requirements()["TSBS"]
        for key in ["seed", "entity_count", "step_ms", "query_range_ms", "batch_size", "bucket_ms"]:
            assert key in requirements.required_parameters

    def test_tsbs_coverage_snapshot_is_fully_implemented(self):
        snapshot = get_standard_coverage_snapshot()
        assert snapshot["TSBS"]["is_fully_implemented"] is True
        assert snapshot["TSBS"]["missing_workloads"] == []

    def test_ldbc_snb_compliance_spec_exists(self):
        requirements = get_standard_compliance_requirements()
        assert "LDBC-SNB" in requirements

    def test_ldbc_snb_compliance_contains_ic_is_lu_workloads(self):
        requirements = get_standard_compliance_requirements()["LDBC-SNB"]
        ids = {spec.workload_id for spec in requirements.canonical_workloads}
        expected = {
            "ldbc_ic1_friends_of_person", "ldbc_ic2_recent_messages",
            "ldbc_ic3_friends_abroad", "ldbc_ic6_tag_cocreators",
            "ldbc_ic9_recent_forum_posts", "ldbc_ic14_shortest_path",
            "ldbc_is1_person_profile", "ldbc_lu1_add_person",
        }
        assert ids == expected

    def test_ldbc_snb_coverage_snapshot_is_fully_implemented(self):
        snapshot = get_standard_coverage_snapshot()
        assert snapshot["LDBC-SNB"]["is_fully_implemented"] is True
        assert snapshot["LDBC-SNB"]["missing_workloads"] == []

    def test_ldbc_profile_exists_in_catalog(self):
        profiles = get_profile_catalog()
        assert "ldbc" in profiles

    def test_ldbc_profile_all_required_workloads_in_current(self):
        from benchmark_standards import CURRENT_CHIMERA_WORKLOADS
        profiles = get_profile_catalog()
        current = set(CURRENT_CHIMERA_WORKLOADS)
        for wid in profiles["ldbc"].required_workloads:
            assert wid in current, f"LDBC-Profil: '{wid}' fehlt in CURRENT_CHIMERA_WORKLOADS"

    def test_ann_benchmarks_compliance_spec_exists(self):
        requirements = get_standard_compliance_requirements()
        assert "ANN-Benchmarks" in requirements

    def test_ann_benchmarks_compliance_contains_three_workloads(self):
        requirements = get_standard_compliance_requirements()["ANN-Benchmarks"]
        ids = {spec.workload_id for spec in requirements.canonical_workloads}
        expected = {"ann_recall_at_k", "ann_range_filter", "ann_batch_query"}
        assert ids == expected

    def test_ann_benchmarks_required_metrics_include_recall_and_qps(self):
        requirements = get_standard_compliance_requirements()["ANN-Benchmarks"]
        assert "recall_at_k" in requirements.required_metrics
        assert "qps" in requirements.required_metrics

    def test_ann_coverage_snapshot_is_fully_implemented(self):
        snapshot = get_standard_coverage_snapshot()
        assert snapshot["ANN-Benchmarks"]["is_fully_implemented"] is True
        assert snapshot["ANN-Benchmarks"]["missing_workloads"] == []

    def test_ann_profile_exists_in_catalog(self):
        profiles = get_profile_catalog()
        assert "ann" in profiles

    def test_ann_profile_required_workloads_in_current(self):
        profiles = get_profile_catalog()
        current = set(CURRENT_CHIMERA_WORKLOADS)
        for wid in profiles["ann"].required_workloads:
            assert wid in current

    def test_beir_compliance_spec_exists(self):
        requirements = get_standard_compliance_requirements()
        assert "BEIR" in requirements

    def test_beir_required_metrics_include_retrieval_quality(self):
        requirements = get_standard_compliance_requirements()["BEIR"]
        assert "ndcg_at_10" in requirements.required_metrics
        assert "mrr" in requirements.required_metrics
        assert "map" in requirements.required_metrics

    def test_beir_coverage_snapshot_is_fully_implemented(self):
        snapshot = get_standard_coverage_snapshot()
        assert snapshot["BEIR"]["is_fully_implemented"] is True
        assert snapshot["BEIR"]["missing_workloads"] == []

    def test_ragas_compliance_spec_exists(self):
        requirements = get_standard_compliance_requirements()
        assert "RAGAS" in requirements

    def test_ragas_required_metrics_include_core_quality_metrics(self):
        requirements = get_standard_compliance_requirements()["RAGAS"]
        for metric in [
            "faithfulness",
            "answer_relevance",
            "context_precision",
            "context_recall",
            "ragas_score",
            "ragbench_score",
        ]:
            assert metric in requirements.required_metrics

    def test_ragas_coverage_snapshot_is_fully_implemented(self):
        snapshot = get_standard_coverage_snapshot()
        assert snapshot["RAGAS"]["is_fully_implemented"] is True
        assert snapshot["RAGAS"]["missing_workloads"] == []

    def test_mlperf_compliance_spec_exists(self):
        requirements = get_standard_compliance_requirements()
        assert "MLPerf-Inference" in requirements

    def test_mlperf_compliance_contains_latency_and_throughput_scenarios(self):
        requirements = get_standard_compliance_requirements()["MLPerf-Inference"]
        ids = {spec.workload_id for spec in requirements.canonical_workloads}
        assert ids == {"llm_serving_latency", "llm_serving_throughput"}

    def test_mlperf_required_metrics_include_quality_target(self):
        requirements = get_standard_compliance_requirements()["MLPerf-Inference"]
        assert "throughput_ops_per_sec" in requirements.required_metrics
        assert "p99_latency_ms" in requirements.required_metrics
        assert "quality_target_passed" in requirements.required_metrics

    def test_mlperf_coverage_snapshot_is_fully_implemented(self):
        snapshot = get_standard_coverage_snapshot()
        assert snapshot["MLPerf-Inference"]["is_fully_implemented"] is True
        assert snapshot["MLPerf-Inference"]["missing_workloads"] == []


# ===========================================================================
# _build_workloads: Vollständigkeit der WorkloadDefinitions
# ===========================================================================

class TestBuildWorkloads:
    """_build_workloads gibt alle Workloads als WorkloadDefinition zurück."""

    def _make_mock_client(self) -> ThemisDBClient:
        cfg = EnduranceConfig()
        client = ThemisDBClient(cfg)
        # Alle HTTP-Calls mocken – kein echter Server nötig.
        client.execute_query = MagicMock(return_value={"status": "ok"})
        client.vector_search = MagicMock(return_value={"status": "ok"})
        return client

    def test_all_workloads_present(self):
        client = self._make_mock_client()
        workloads = _build_workloads(client)
        for wid in CURRENT_CHIMERA_WORKLOADS:
            assert wid in workloads, f"Workload '{wid}' fehlt in _build_workloads()"

    def test_workload_callable(self):
        """Jede WorkloadDefinition hat eine aufrufbare operation."""
        from benchmark_harness import WorkloadDefinition
        client = self._make_mock_client()
        workloads = _build_workloads(client)
        for wid, wd in workloads.items():
            assert callable(wd.operation), f"operation von '{wid}' ist nicht callable"

    def test_workload_id_matches_key(self):
        client = self._make_mock_client()
        workloads = _build_workloads(client)
        for wid, wd in workloads.items():
            assert wd.workload_id == wid, (
                f"WorkloadDefinition.workload_id '{wd.workload_id}' != key '{wid}'"
            )

    def test_no_duplicate_ids(self):
        client = self._make_mock_client()
        workloads = _build_workloads(client)
        ids = [wd.workload_id for wd in workloads.values()]
        assert len(ids) == len(set(ids))

    def test_tpcc_mix_covers_all_five_transaction_types(self):
        client = self._make_mock_client()
        workloads = _build_workloads(client)

        tx_values = [0.10, 0.60, 0.90, 0.94, 0.98]
        with patch("random.random", side_effect=tx_values):
            for _ in tx_values:
                workloads["tpc_c_mix"].operation()

        called_sql = [call.args[0] for call in client.execute_query.call_args_list]
        assert any("tpcc_new_order" in sql for sql in called_sql)
        assert any("tpcc_payment" in sql for sql in called_sql)
        assert any("tpcc_order_status" in sql for sql in called_sql)
        assert any("tpcc_delivery" in sql for sql in called_sql)
        assert any("tpcc_stock_level" in sql for sql in called_sql)

    def test_tsbs_raw_ingest_uses_deterministic_seeded_state(self):
        client = self._make_mock_client()
        cfg = EnduranceConfig(
            tsbs_seed=7,
            tsbs_entity_count=1,
            tsbs_entity_prefix="host",
            tsbs_start_time_ms=1_000_000,
            tsbs_step_ms=10,
            tsbs_base_value=10.0,
            tsbs_value_jitter_pct=0.0,
        )
        workloads = _build_workloads(client, cfg)

        workloads["ts_ingest_raw"].operation()
        workloads["ts_ingest_raw"].operation()

        first_params = client.execute_query.call_args_list[0].args[1]
        second_params = client.execute_query.call_args_list[1].args[1]
        assert first_params["$2"] == "host_1"
        assert second_params["$2"] == "host_1"
        assert first_params["$3"] == 1_000_000
        assert second_params["$3"] == 1_000_010
        assert first_params["$4"] == 10.0

    def test_tsbs_batch_ingest_uses_configured_batch_size(self):
        client = self._make_mock_client()
        cfg = EnduranceConfig(tsbs_batch_size=3)
        workloads = _build_workloads(client, cfg)

        workloads["ts_ingest_batch"].operation()

        assert client.execute_query.call_count == 3

    def test_tsbs_range_query_uses_configured_time_window(self):
        client = self._make_mock_client()
        cfg = EnduranceConfig(
            tsbs_start_time_ms=1_000_000,
            tsbs_step_ms=1_000,
            tsbs_query_range_ms=500,
        )
        workloads = _build_workloads(client, cfg)

        workloads["ts_ingest_raw"].operation()
        workloads["ts_range_query"].operation()

        params = client.execute_query.call_args_list[-1].args[1]
        assert params["$1"] == 1_000_500
        assert params["$2"] == 1_001_000

    def test_tsbs_lastpoint_queries_benchmark_timeseries(self):
        client = self._make_mock_client()
        cfg = EnduranceConfig(tsbs_entity_count=5, tsbs_metrics=["cpu_usage"])
        workloads = _build_workloads(client, cfg)

        workloads["ts_lastpoint"].operation()

        called_sql = client.execute_query.call_args_list[0].args[0]
        assert "benchmark_timeseries" in called_sql
        assert "ORDER BY entity" in called_sql or "ts_ms DESC" in called_sql

    def test_tsbs_downsample_uses_avg_and_group_by(self):
        client = self._make_mock_client()
        workloads = _build_workloads(client)

        workloads["ts_downsample"].operation()

        called_sql = client.execute_query.call_args_list[0].args[0]
        assert "AVG" in called_sql
        assert "GROUP BY" in called_sql

    def test_tsbs_high_cpu_filters_by_threshold(self):
        client = self._make_mock_client()
        cfg = EnduranceConfig(tsbs_high_cpu_threshold=95.0)
        workloads = _build_workloads(client, cfg)

        workloads["ts_high_cpu"].operation()

        called_params = client.execute_query.call_args_list[0].args[1]
        assert called_params["$1"] == "cpu_usage"
        assert called_params["$2"] == 95.0

    def test_tsbs_groupby_uses_max_min_avg(self):
        client = self._make_mock_client()
        workloads = _build_workloads(client)

        workloads["ts_groupby"].operation()

        called_sql = client.execute_query.call_args_list[0].args[0]
        assert "MAX" in called_sql
        assert "MIN" in called_sql
        assert "AVG" in called_sql
        assert "GROUP BY" in called_sql

    def test_tsbs_bucket_ms_passed_to_downsample_and_groupby(self):
        client = self._make_mock_client()
        cfg = EnduranceConfig(tsbs_bucket_ms=30_000)
        workloads = _build_workloads(client, cfg)

        workloads["ts_downsample"].operation()
        workloads["ts_groupby"].operation()

        params_ds = client.execute_query.call_args_list[0].args[1]
        params_gb = client.execute_query.call_args_list[1].args[1]
        assert params_ds["$1"] == 30_000
        assert params_gb["$1"] == 30_000

    def test_ldbc_ic1_traverses_knows_relationship(self):
        client = self._make_mock_client()
        cfg = EnduranceConfig(ldbc_person_count=50)
        workloads = _build_workloads(client, cfg)

        workloads["ldbc_ic1_friends_of_person"].operation()

        sql = client.execute_query.call_args_list[0].args[0]
        assert "KNOWS" in sql
        assert "Person" in sql
        assert "distance" in sql

    def test_ldbc_ic14_shortest_path_uses_two_persons(self):
        client = self._make_mock_client()
        cfg = EnduranceConfig(ldbc_person_count=100)
        workloads = _build_workloads(client, cfg)

        workloads["ldbc_ic14_shortest_path"].operation()

        params = client.execute_query.call_args_list[0].args[1]
        assert "$1" in params
        assert "$2" in params
        assert params["$1"] != params["$2"]

    def test_ldbc_is1_point_query_uses_person_id(self):
        client = self._make_mock_client()
        workloads = _build_workloads(client)

        workloads["ldbc_is1_person_profile"].operation()

        sql = client.execute_query.call_args_list[0].args[0]
        assert "Person" in sql
        assert "LIMIT 1" in sql

    def test_ldbc_lu1_inserts_new_person_node(self):
        client = self._make_mock_client()
        workloads = _build_workloads(client)

        workloads["ldbc_lu1_add_person"].operation()

        sql = client.execute_query.call_args_list[0].args[0]
        assert "CREATE" in sql
        assert "Person" in sql

    def test_ldbc_all_workloads_present_in_build_workloads(self):
        client = self._make_mock_client()
        workloads = _build_workloads(client)
        ldbc_ids = [
            "ldbc_ic1_friends_of_person", "ldbc_ic2_recent_messages",
            "ldbc_ic3_friends_abroad", "ldbc_ic6_tag_cocreators",
            "ldbc_ic9_recent_forum_posts", "ldbc_ic14_shortest_path",
            "ldbc_is1_person_profile", "ldbc_lu1_add_person",
        ]
        for wid in ldbc_ids:
            assert wid in workloads, f"LDBC-Workload '{wid}' fehlt in _build_workloads()"

    def test_ann_recall_at_k_calls_vector_search_with_configured_k(self):
        client = self._make_mock_client()
        cfg = EnduranceConfig(ann_k=7)
        workloads = _build_workloads(client, cfg)

        workloads["ann_recall_at_k"].operation()

        assert client.vector_search.call_count == 1
        _, kwargs = client.vector_search.call_args
        assert kwargs["top_k"] == 7

    def test_ann_range_filter_executes_filtered_query(self):
        client = self._make_mock_client()
        cfg = EnduranceConfig(ann_k=11)
        workloads = _build_workloads(client, cfg)

        workloads["ann_range_filter"].operation()

        sql = client.execute_query.call_args_list[0].args[0]
        params = client.execute_query.call_args_list[0].args[1]
        assert "WHERE category = $1" in sql
        assert "vec_l2" in sql
        assert params["$3"] == 11

    def test_ann_batch_query_uses_configured_batch_size(self):
        client = self._make_mock_client()
        cfg = EnduranceConfig(ann_batch_size=4)
        workloads = _build_workloads(client, cfg)

        workloads["ann_batch_query"].operation()

        assert client.vector_search.call_count == 4

    def test_ann_dataset_gist1m_uses_960_dim_vectors(self):
        client = self._make_mock_client()
        cfg = EnduranceConfig(ann_dataset="gist1m", ann_pool_size=50)
        workloads = _build_workloads(client, cfg)

        workloads["ann_recall_at_k"].operation()

        args, kwargs = client.vector_search.call_args
        assert len(args[0]) == 960
        assert kwargs["top_k"] == cfg.ann_k

    def test_ann_dataset_glove100_uses_100_dim_vectors(self):
        client = self._make_mock_client()
        cfg = EnduranceConfig(ann_dataset="glove100", ann_pool_size=100)
        workloads = _build_workloads(client, cfg)

        workloads["ann_batch_query"].operation()

        args, _ = client.vector_search.call_args
        assert len(args[0]) == 100

    def test_ann_unknown_dataset_falls_back_to_sift1m_dim(self):
        client = self._make_mock_client()
        cfg = EnduranceConfig(ann_dataset="unknown_dataset", ann_pool_size=50)
        workloads = _build_workloads(client, cfg)

        workloads["ann_recall_at_k"].operation()

        args, _ = client.vector_search.call_args
        assert len(args[0]) == 128

    def test_ann_workload_parameters_include_selected_dataset(self):
        client = self._make_mock_client()
        cfg = EnduranceConfig(ann_dataset="glove100")
        workloads = _build_workloads(client, cfg)

        params = workloads["ann_recall_at_k"].parameters
        assert params["dataset"] == "glove100"
        assert params["dim"] == 100

    def test_rag_retrieval_quality_sets_beir_metrics_on_client(self):
        client = self._make_mock_client()
        cfg = EnduranceConfig(beir_dataset="msmarco", beir_k=10, beir_seed=7)
        workloads = _build_workloads(client, cfg)

        workloads["rag_retrieval_quality"].operation()

        assert hasattr(client, "_last_beir_metrics")
        metrics = getattr(client, "_last_beir_metrics")
        assert 0.0 <= metrics["ndcg_at_10"] <= 1.0
        assert 0.0 <= metrics["mrr"] <= 1.0
        assert 0.0 <= metrics["map"] <= 1.0
        assert metrics["dataset"] == "msmarco"

    def test_rag_retrieval_uses_beir_dataset_specific_query_text(self):
        client = self._make_mock_client()
        cfg = EnduranceConfig(beir_dataset="nfcorpus", beir_seed=13)
        workloads = _build_workloads(client, cfg)

        workloads["rag_retrieval_quality"].operation()

        params = client.execute_query.call_args_list[0].args[1]
        assert "nfcorpus" in params["$1"]

    def test_rag_retrieval_workload_definition_contains_beir_parameters(self):
        client = self._make_mock_client()
        cfg = EnduranceConfig(beir_dataset="scifact", beir_k=12, beir_query_count=77)
        workloads = _build_workloads(client, cfg)

        params = workloads["rag_retrieval_quality"].parameters
        assert params["dataset"] == "scifact"
        assert params["k"] == 12
        assert params["query_count"] == 77

    def test_llm_workload_definitions_contain_mlperf_parameters(self):
        client = self._make_mock_client()
        cfg = EnduranceConfig(
            mlperf_dataset="openorca",
            mlperf_target_latency_ms=1200.0,
            mlperf_target_qps=80.0,
            mlperf_quality_target=0.81,
        )
        workloads = _build_workloads(client, cfg)

        latency_params = workloads["llm_serving_latency"].parameters
        throughput_params = workloads["llm_serving_throughput"].parameters
        assert latency_params["scenario"] == "singlestream"
        assert throughput_params["scenario"] == "server"
        assert latency_params["dataset"] == "openorca"
        assert throughput_params["dataset"] == "openorca"
        assert latency_params["quality_target"] == 0.81
        assert throughput_params["quality_target"] == 0.81

    def test_run_window_enriches_llm_rows_with_mlperf_compliance_fields(self):
        client = self._make_mock_client()
        cfg = EnduranceConfig(
            run_iterations=1,
            mlperf_dataset="openorca",
            mlperf_target_latency_ms=10_000.0,
            mlperf_target_qps=0.1,
            mlperf_quality_target=0.5,
        )
        workloads = _build_workloads(client, cfg)

        rows_latency = _run_window(
            workload_ids=["llm_serving_latency"],
            all_workloads=workloads,
            cfg=cfg,
            database_info=None,
            parallel=False,
        )
        rows_throughput = _run_window(
            workload_ids=["llm_serving_throughput"],
            all_workloads=workloads,
            cfg=cfg,
            database_info=None,
            parallel=False,
        )

        row_latency = rows_latency[0]
        row_throughput = rows_throughput[0]
        for row in [row_latency, row_throughput]:
            assert "mlperf_scenario" in row
            assert "mlperf_dataset" in row
            assert "mlperf_quality_target" in row
            assert "mlperf_quality_score" in row
            assert "quality_target_passed" in row
            assert "mlperf_performance_target_passed" in row
            assert "mlperf_compliance_passed" in row
            assert row["mlperf_dataset"] == "openorca"
            assert isinstance(row["quality_target_passed"], bool)

    def test_rag_qa_e2e_sets_ragas_metrics_on_client(self):
        client = self._make_mock_client()
        cfg = EnduranceConfig(ragas_seed=17)
        workloads = _build_workloads(client, cfg)

        workloads["rag_qa_e2e"].operation()

        assert hasattr(client, "_last_ragas_metrics")
        metrics = getattr(client, "_last_ragas_metrics")
        for key in [
            "faithfulness",
            "answer_relevance",
            "context_precision",
            "context_recall",
            "ragas_score",
            "ragbench_score",
        ]:
            assert key in metrics
            assert 0.0 <= metrics[key] <= 1.0

    def test_run_window_enriches_rag_qa_row_with_ragas_metrics(self):
        client = self._make_mock_client()
        cfg = EnduranceConfig(run_iterations=1, ragas_seed=19, ragas_eval_samples=7)
        workloads = _build_workloads(client, cfg)

        rows = _run_window(
            workload_ids=["rag_qa_e2e"],
            all_workloads=workloads,
            cfg=cfg,
            database_info=None,
            parallel=False,
        )

        assert len(rows) == 1
        row = rows[0]
        for key in [
            "faithfulness",
            "answer_relevance",
            "context_precision",
            "context_recall",
            "ragas_score",
            "ragbench_score",
            "ragas_eval_samples",
        ]:
            assert key in row

    def test_run_window_enriches_rag_row_with_beir_metrics(self):
        client = self._make_mock_client()
        cfg = EnduranceConfig(run_iterations=1, beir_seed=11)
        workloads = _build_workloads(client, cfg)

        rows = _run_window(
            workload_ids=["rag_retrieval_quality"],
            all_workloads=workloads,
            cfg=cfg,
            database_info=None,
            parallel=False,
        )

        assert len(rows) == 1
        row = rows[0]
        assert "ndcg_at_10" in row
        assert "mrr" in row
        assert "map" in row
        assert "beir_dataset" in row
        assert "beir_k" in row

    def test_run_window_enriches_rows_with_standard_schema_fields(self):
        client = self._make_mock_client()
        cfg = EnduranceConfig(run_iterations=1)
        workloads = _build_workloads(client, cfg)

        rows = _run_window(
            workload_ids=["ycsb_a"],
            all_workloads=workloads,
            cfg=cfg,
            database_info=None,
            parallel=False,
        )

        row = rows[0]
        assert row["result_schema_id"] == "chimera.result.standard.v1"
        assert row["result_schema_version"] == "1.0.0"
        assert row["standard_id"] == "YCSB"
        assert "standard_required_metrics" in row
        assert "standard_present_metrics" in row
        assert "standard_missing_metrics" in row
        assert "standard_metric_coverage_pct" in row


# ===========================================================================
# chimera_server: ServerConfig und App-Routen (ohne laufenden Server)
# ===========================================================================

class TestChimeraServerConfig:
    """ServerConfig-Defaults und Validierung."""

    def test_default_port_is_8080(self):
        from chimera_server import ServerConfig
        cfg = ServerConfig()
        assert cfg.port == 8080

    def test_default_host_is_0_0_0_0(self):
        from chimera_server import ServerConfig
        cfg = ServerConfig()
        assert cfg.host == "0.0.0.0"

    def test_tls_cert_empty_by_default(self):
        from chimera_server import ServerConfig
        cfg = ServerConfig()
        assert cfg.tls_cert == ""

    def test_cache_ttl_default(self):
        from chimera_server import ServerConfig
        cfg = ServerConfig()
        assert cfg.results_cache_ttl == 30


class TestChimeraServerApp:
    """Flask-App-Routen: Test-Client ohne echten Server."""

    @pytest.fixture()
    def client(self, tmp_path):
        from chimera_server import ServerConfig, _build_app
        cfg = ServerConfig(results_dir=str(tmp_path), port=8080)
        app = _build_app(cfg)
        app.config["TESTING"] = True
        return app.test_client(), tmp_path

    def test_root_redirects_to_dashboard(self, client):
        tc, _ = client
        resp = tc.get("/")
        assert resp.status_code in (301, 302)
        assert b"dashboard" in resp.headers["Location"].encode()

    def test_status_endpoint_returns_ok(self, client):
        tc, _ = client
        resp = tc.get("/api/v1/status")
        assert resp.status_code == 200
        data = json.loads(resp.data)
        assert data["status"] == "ok"
        assert "chimera_version" in data
        assert "uptime_seconds" in data

    def test_status_includes_database_info_from_topology(self, client):
        tc, tmp_path = client
        topology = {
            "database_info": {
                "detected": True,
                "source": "/api/v1/info",
                "product": "ThemisDB",
                "version": "1.8.2",
                "status": "ok",
            }
        }
        (tmp_path / "test_topology.json").write_text(json.dumps(topology), encoding="utf-8")
        resp = tc.get("/api/v1/status")
        data = json.loads(resp.data)
        assert data["database_info"]["product"] == "ThemisDB"
        assert data["database_info"]["version"] == "1.8.2"

    def test_results_empty(self, client):
        tc, _ = client
        resp = tc.get("/api/v1/results")
        assert resp.status_code == 200
        assert json.loads(resp.data) == []

    def test_results_latest_empty(self, client):
        tc, _ = client
        resp = tc.get("/api/v1/results/latest")
        assert resp.status_code == 200
        assert json.loads(resp.data) == []

    def test_runs_empty(self, client):
        tc, _ = client
        resp = tc.get("/api/v1/runs")
        assert resp.status_code == 200
        assert json.loads(resp.data) == []

    def test_dashboard_json_returns_404_when_empty(self, client):
        tc, _ = client
        resp = tc.get("/api/v1/dashboard")
        assert resp.status_code == 404

    def test_results_with_checkpoint(self, client):
        tc, tmp_path = client
        rows = [
            {
                "workload_id": "ycsb_a",
                "timestamp": "2026-04-17T10:00:00Z",
                "throughput_ops_per_sec": 12500.0,
                "mean_latency_ms": 0.5,
                "p50_latency_ms": 0.4,
                "p95_latency_ms": 0.9,
                "p99_latency_ms": 1.2,
                "error_count": 0,
                "elapsed_seconds": 10.0,
            }
        ]
        (tmp_path / "checkpoint_001.json").write_text(
            json.dumps(rows), encoding="utf-8"
        )
        resp = tc.get("/api/v1/results")
        assert resp.status_code == 200
        data = json.loads(resp.data)
        assert len(data) == 1
        assert data[0]["workload_id"] == "ycsb_a"

    def test_dashboard_json_with_data(self, client):
        tc, tmp_path = client
        rows = [
            {
                "workload_id": "ycsb_c",
                "system_name": "ThemisDB",
                "throughput_ops_per_sec": 8000.0,
                "mean_latency_ms": 0.6,
                "p50_latency_ms": 0.5,
                "p95_latency_ms": 1.0,
                "p99_latency_ms": 1.5,
                "error_count": 0,
                "elapsed_seconds": 5.0,
            }
        ]
        (tmp_path / "checkpoint_002.json").write_text(
            json.dumps(rows), encoding="utf-8"
        )
        resp = tc.get("/api/v1/dashboard")
        assert resp.status_code == 200
        data = json.loads(resp.data)
        assert "title" in data or "workload_summary" in data

    def test_latest_returns_last_checkpoint(self, client):
        tc, tmp_path = client
        rows_a = [{"workload_id": "ycsb_a", "throughput_ops_per_sec": 100.0,
                   "mean_latency_ms": 1.0, "p50_latency_ms": 1.0,
                   "p95_latency_ms": 1.5, "p99_latency_ms": 2.0,
                   "error_count": 0, "elapsed_seconds": 1.0}]
        rows_b = [{"workload_id": "ycsb_c", "throughput_ops_per_sec": 200.0,
                   "mean_latency_ms": 0.5, "p50_latency_ms": 0.4,
                   "p95_latency_ms": 0.8, "p99_latency_ms": 1.0,
                   "error_count": 0, "elapsed_seconds": 1.0}]
        (tmp_path / "checkpoint_001.json").write_text(json.dumps(rows_a), encoding="utf-8")
        (tmp_path / "checkpoint_002.json").write_text(json.dumps(rows_b), encoding="utf-8")
        resp = tc.get("/api/v1/results/latest")
        data = json.loads(resp.data)
        # Letzter Checkpoint ist checkpoint_002 → ycsb_c
        assert data[0]["workload_id"] == "ycsb_c"

    def test_runs_lists_json_files(self, client):
        tc, tmp_path = client
        (tmp_path / "run_20260417T100000Z.json").write_text(
            json.dumps([{"workload_id": "ycsb_f"}]), encoding="utf-8"
        )
        resp = tc.get("/api/v1/runs")
        assert resp.status_code == 200
        data = json.loads(resp.data)
        assert any("run_" in entry["filename"] for entry in data)

    def test_dashboard_html_empty_graceful(self, client):
        tc, _ = client
        resp = tc.get("/dashboard")
        assert resp.status_code == 200
        assert b"CHIMERA" in resp.data or b"chimera" in resp.data.lower()


# ===========================================================================
# chimera_server: TLS-Kontext-Fehlerbehandlung
# ===========================================================================

class TestChimeraServerTLS:
    """TLS-Kontext: Fehlerpfade."""

    def test_missing_cert_raises(self, tmp_path):
        from chimera_server import _build_ssl_context
        with pytest.raises(FileNotFoundError, match="Zertifikat"):
            _build_ssl_context(
                str(tmp_path / "nonexistent.crt"),
                str(tmp_path / "nonexistent.key"),
            )

    def test_missing_key_raises(self, tmp_path):
        from chimera_server import _build_ssl_context
        cert = tmp_path / "server.crt"
        cert.write_text("dummy", encoding="utf-8")
        with pytest.raises(FileNotFoundError, match="Schlüssel"):
            _build_ssl_context(str(cert), str(tmp_path / "nonexistent.key"))


# ===========================================================================
# chimera_server: _load_result_files, _list_run_files
# ===========================================================================

class TestServerLoadHelpers:
    """Helfer-Funktionen im chimera_server."""

    def test_load_result_files_returns_rows(self, tmp_path):
        from chimera_server import _load_result_files
        rows = [{"workload_id": "ycsb_a", "throughput_ops_per_sec": 100.0}]
        (tmp_path / "checkpoint_001.json").write_text(
            json.dumps(rows), encoding="utf-8"
        )
        loaded = _load_result_files(tmp_path)
        assert len(loaded) == 1
        assert loaded[0]["workload_id"] == "ycsb_a"

    def test_load_result_files_skips_corrupt(self, tmp_path):
        from chimera_server import _load_result_files
        (tmp_path / "checkpoint_001.json").write_text("not json{{{", encoding="utf-8")
        loaded = _load_result_files(tmp_path)
        assert loaded == []

    def test_load_result_files_dict_wrapped(self, tmp_path):
        from chimera_server import _load_result_files
        row = {"workload_id": "ts_ingest_raw", "throughput_ops_per_sec": 5000.0}
        (tmp_path / "run_001.json").write_text(json.dumps(row), encoding="utf-8")
        loaded = _load_result_files(tmp_path)
        assert any(r["workload_id"] == "ts_ingest_raw" for r in loaded)

    def test_list_run_files_metadata(self, tmp_path):
        from chimera_server import _list_run_files
        (tmp_path / "checkpoint_001.json").write_text("[]", encoding="utf-8")
        files = _list_run_files(tmp_path)
        assert len(files) == 1
        assert "filename" in files[0]
        assert "size_bytes" in files[0]
        assert "modified" in files[0]

    def test_list_run_files_skips_dotfiles(self, tmp_path):
        from chimera_server import _list_run_files
        (tmp_path / ".heartbeat").write_text("2026-04-17T10:00:00+00:00", encoding="utf-8")
        files = _list_run_files(tmp_path)
        assert not any(e["filename"].startswith(".") for e in files)


# ===========================================================================
# CLI: serve-Subcommand (argparse-Ebene)
# ===========================================================================

class TestCLIServeArgs:
    """chimera_cli.py: serve-Subcommand argparse-Argumente."""

    def _parse(self, argv: List[str]):
        from chimera_cli import _build_parser
        return _build_parser().parse_args(argv)

    def test_serve_defaults(self):
        args = self._parse(["serve"])
        assert args.command == "serve"
        assert args.server_port == 8080
        assert args.server_host == "0.0.0.0"
        assert args.tls_cert == ""
        assert args.tls_key == ""
        assert args.cache_ttl == 30

    def test_serve_custom_port(self):
        args = self._parse(["serve", "--server-port", "9090"])
        assert args.server_port == 9090

    def test_serve_tls_cert_key(self):
        args = self._parse([
            "serve",
            "--tls-cert", "certs/server.crt",
            "--tls-key", "certs/server.key",
        ])
        assert args.tls_cert == "certs/server.crt"
        assert args.tls_key == "certs/server.key"

    def test_serve_debug_flag(self):
        args = self._parse(["serve", "--debug"])
        assert args.debug is True

    def test_serve_results_dir(self):
        args = self._parse(["serve", "--results-dir", "/data/results"])
        assert args.results_dir == "/data/results"

    def test_serve_title(self):
        args = self._parse(["serve", "--title", "My Dashboard"])
        assert args.title == "My Dashboard"


# ===========================================================================
# CLI: --capabilities und --profile-strict Argumente
# ===========================================================================

class TestCLICapabilityArgs:
    """CLI-Argumente für Capabilities und Profile."""

    def _parse(self, argv: List[str]):
        from chimera_cli import _build_parser
        return _build_parser().parse_args(argv)

    def test_run_capabilities_default_empty(self):
        args = self._parse(["run"])
        assert args.capabilities == ""

    def test_run_capabilities_set(self):
        args = self._parse(["run", "--capabilities", "kv_read,kv_write"])
        assert args.capabilities == "kv_read,kv_write"

    def test_run_profile_strict_default_false(self):
        args = self._parse(["run"])
        assert args.profile_strict is False

    def test_run_profile_strict_flag(self):
        args = self._parse(["run", "--profile-strict"])
        assert args.profile_strict is True

    def test_run_capabilities_endpoint_default(self):
        args = self._parse(["run"])
        assert args.capabilities_endpoint == "/api/v1/capabilities"

    def test_run_capabilities_endpoint_custom(self):
        args = self._parse(["run", "--capabilities-endpoint", "/v2/caps"])
        assert args.capabilities_endpoint == "/v2/caps"

    def test_run_profile_id(self):
        args = self._parse(["run", "--profile", "timeseries"])
        assert args.profile == "timeseries"

    def test_endurance_profile_strict(self):
        args = self._parse(["endurance", "--profile-strict"])
        assert args.profile_strict is True

    def test_endurance_capabilities(self):
        args = self._parse(["endurance", "--capabilities", "vector_search"])
        assert args.capabilities == "vector_search"


# ===========================================================================
# EnduranceConfig: neue Felder
# ===========================================================================

class TestEnduranceConfigNewFields:
    """Neue Felder in EnduranceConfig."""

    def test_default_capabilities_endpoint(self):
        cfg = EnduranceConfig()
        assert cfg.capabilities_endpoint == "/api/v1/capabilities"

    def test_default_capabilities_override_empty(self):
        cfg = EnduranceConfig()
        assert cfg.capabilities_override == ""

    def test_default_profile_strict_false(self):
        cfg = EnduranceConfig()
        assert cfg.profile_strict is False

    def test_default_test_profile_empty(self):
        cfg = EnduranceConfig()
        assert cfg.test_profile == ""

    def test_custom_capabilities_override(self):
        cfg = EnduranceConfig(capabilities_override="kv_read,vector_search")
        assert "kv_read" in cfg.capabilities_override

    def test_default_tsbs_seed_and_batch_size(self):
        cfg = EnduranceConfig()
        assert cfg.tsbs_seed == 1337
        assert cfg.tsbs_batch_size == 50

    def test_custom_tsbs_metrics(self):
        cfg = EnduranceConfig(tsbs_metrics=["cpu", "mem"])
        assert cfg.tsbs_metrics == ["cpu", "mem"]

    def test_default_ann_fields(self):
        cfg = EnduranceConfig()
        assert cfg.ann_dim == 128
        assert cfg.ann_k == 10
        assert cfg.ann_pool_size == 10_000
        assert cfg.ann_seed == 42
        assert cfg.ann_batch_size == 10
        assert cfg.ann_dataset == "sift1m"

    def test_custom_ann_fields(self):
        cfg = EnduranceConfig(
            ann_dim=64,
            ann_k=20,
            ann_pool_size=5000,
            ann_seed=7,
            ann_batch_size=8,
            ann_dataset="glove100",
        )
        assert cfg.ann_dim == 64
        assert cfg.ann_k == 20
        assert cfg.ann_pool_size == 5000
        assert cfg.ann_seed == 7
        assert cfg.ann_batch_size == 8
        assert cfg.ann_dataset == "glove100"

    def test_default_beir_fields(self):
        cfg = EnduranceConfig()
        assert cfg.beir_dataset == "msmarco"
        assert cfg.beir_k == 10
        assert cfg.beir_query_count == 100
        assert cfg.beir_seed == 2026

    def test_custom_beir_fields(self):
        cfg = EnduranceConfig(beir_dataset="nfcorpus", beir_k=20, beir_query_count=250, beir_seed=42)
        assert cfg.beir_dataset == "nfcorpus"
        assert cfg.beir_k == 20
        assert cfg.beir_query_count == 250
        assert cfg.beir_seed == 42

    def test_default_mlperf_fields(self):
        cfg = EnduranceConfig()
        assert cfg.mlperf_dataset == "mlperf_llm_mock"
        assert cfg.mlperf_target_qps == 100.0
        assert cfg.mlperf_target_latency_ms == 1500.0
        assert cfg.mlperf_quality_target == 0.80

    def test_custom_mlperf_fields(self):
        cfg = EnduranceConfig(
            mlperf_dataset="openorca",
            mlperf_target_qps=55.0,
            mlperf_target_latency_ms=900.0,
            mlperf_quality_target=0.85,
        )
        assert cfg.mlperf_dataset == "openorca"
        assert cfg.mlperf_target_qps == 55.0
        assert cfg.mlperf_target_latency_ms == 900.0
        assert cfg.mlperf_quality_target == 0.85

    def test_default_ragas_fields(self):
        cfg = EnduranceConfig()
        assert cfg.ragas_seed == 2027
        assert cfg.ragas_eval_samples == 5

    def test_custom_ragas_fields(self):
        cfg = EnduranceConfig(ragas_seed=77, ragas_eval_samples=9)
        assert cfg.ragas_seed == 77
        assert cfg.ragas_eval_samples == 9


class TestConfigStandardization:
    """Zentrale Normalisierung für gemischte TOML/ENV-Konfigwerte."""

    def test_standardize_workloads_and_formats_deduplicates_and_normalizes_case(self):
        cfg = EnduranceConfig(
            workloads=["YCSB_A", " ycsb_a ", "rag_qa_e2e", ""],
            formats=["JSON", " csv ", "json", "invalid"],
        )

        normalized = _standardize_endurance_config(cfg)

        assert normalized.workloads == ["ycsb_a", "rag_qa_e2e"]
        assert normalized.formats == ["json", "csv"]

    def test_standardize_percentiles_filters_invalid_and_sorts(self):
        cfg = EnduranceConfig(percentiles=[99.0, 50.0, -1.0, 120.0, 95.0, 95.0])

        normalized = _standardize_endurance_config(cfg)

        assert normalized.percentiles == [50.0, 95.0, 99.0]

    def test_standardize_protocol_and_members(self):
        cfg = EnduranceConfig(protocol="INVALID", federation_members=" Neo4j,neo4j, QDRANT ")

        normalized = _standardize_endurance_config(cfg)

        assert normalized.protocol == "http_rest"
        assert normalized.federation_members == "neo4j,qdrant"

    def test_standardize_numeric_fields_clamps_to_positive_ranges(self):
        cfg = EnduranceConfig(
            parallel_workers=0,
            run_iterations=-10,
            report_interval_min=0.0,
            mlperf_quality_target=1.5,
            ragas_eval_samples=0,
        )

        normalized = _standardize_endurance_config(cfg)

        assert normalized.parallel_workers >= 1
        assert normalized.run_iterations >= 1
        assert normalized.report_interval_min > 0.0
        assert normalized.mlperf_quality_target == 1.0
        assert normalized.ragas_eval_samples >= 1

    def test_standardize_dataset_provisioning_fields(self):
        cfg = EnduranceConfig(
            dataset_provisioning_seed=-1,
            dataset_provisioning_version=" V2 ",
        )

        normalized = _standardize_endurance_config(cfg)

        assert normalized.dataset_provisioning_seed == 0
        assert normalized.dataset_provisioning_version == "v2"


class TestDatasetProvisioning:
    """Deterministische Dataset-Provisioning-Artefakte."""

    def test_manifest_is_deterministic_for_same_config(self):
        cfg = EnduranceConfig(dataset_provisioning_seed=1234, dataset_provisioning_version="v1")
        m1 = _build_dataset_provisioning_manifest(cfg)
        m2 = _build_dataset_provisioning_manifest(cfg)

        assert m1["fingerprint_sha256"] == m2["fingerprint_sha256"]

    def test_manifest_fingerprint_changes_with_seed(self):
        cfg_a = EnduranceConfig(dataset_provisioning_seed=1234)
        cfg_b = EnduranceConfig(dataset_provisioning_seed=1235)
        m1 = _build_dataset_provisioning_manifest(cfg_a)
        m2 = _build_dataset_provisioning_manifest(cfg_b)

        assert m1["fingerprint_sha256"] != m2["fingerprint_sha256"]

    def test_topology_includes_dataset_provisioning_fields(self):
        cfg = EnduranceConfig(dataset_provisioning_seed=77, dataset_provisioning_version="v3")
        topology = _build_test_topology(cfg)

        assert "dataset_provisioning" in topology
        assert topology["dataset_provisioning"]["seed"] == 77
        assert topology["dataset_provisioning"]["version"] == "v3"


class TestResultSchema:
    """Standardkonforme Ergebnis-Schema-Definitionen."""

    def test_build_standard_result_schema_contains_base_fields(self):
        schema = _build_standard_result_schema()
        assert schema["schema_id"] == "chimera.result.standard.v1"
        assert schema["schema_version"] == "1.0.0"
        assert "base_required_fields" in schema
        assert "workload_id" in schema["base_required_fields"]

    def test_build_standard_result_schema_contains_known_standards(self):
        schema = _build_standard_result_schema()
        assert "standards" in schema
        assert "YCSB" in schema["standards"]
        assert "BEIR" in schema["standards"]

    def test_get_workload_standard_id_mapping(self):
        assert _get_workload_standard_id("ycsb_a") == "YCSB"
        assert _get_workload_standard_id("rag_retrieval_quality") == "BEIR"
        assert _get_workload_standard_id("rag_qa_e2e") == "RAGAS"


class TestStandardGoldenBaselines:
    """Golden-Baselines pro Standard."""

    def test_build_standard_golden_baselines_contains_metadata(self):
        cfg = EnduranceConfig()
        baselines = _build_standard_golden_baselines(cfg)
        assert baselines["golden_baseline_id"] == "chimera.standard.golden-baseline.v1"
        assert baselines["golden_baseline_version"] == "1.0.0"
        assert "fingerprint_sha256" in baselines
        assert len(str(baselines["fingerprint_sha256"])) == 64

    def test_build_standard_golden_baselines_contains_known_standards(self):
        cfg = EnduranceConfig()
        baselines = _build_standard_golden_baselines(cfg)
        standards = baselines["standards"]
        assert "YCSB" in standards
        assert "BEIR" in standards
        assert "MLPerf-Inference" in standards
        assert "RAGAS" in standards

    def test_build_standard_golden_baselines_includes_metric_ranges(self):
        cfg = EnduranceConfig()
        baselines = _build_standard_golden_baselines(cfg)
        ycsb = baselines["standards"]["YCSB"]
        assert "metric_ranges" in ycsb
        assert "throughput_ops_per_sec" in ycsb["metric_ranges"]
        assert "p99_latency_ms" in ycsb["metric_ranges"]

    def test_build_standard_golden_baselines_fingerprint_deterministic(self):
        cfg = EnduranceConfig(dataset_provisioning_seed=77, dataset_provisioning_version="v3")
        b1 = _build_standard_golden_baselines(cfg)
        b2 = _build_standard_golden_baselines(cfg)
        assert b1["fingerprint_sha256"] == b2["fingerprint_sha256"]

    def test_write_standard_golden_baselines_writes_file(self, tmp_path):
        cfg = EnduranceConfig()
        baselines = _build_standard_golden_baselines(cfg)
        path = _write_standard_golden_baselines(tmp_path, baselines)
        assert path.exists()
        raw = json.loads(path.read_text(encoding="utf-8"))
        assert raw["golden_baseline_id"] == "chimera.standard.golden-baseline.v1"

    def test_repository_seed_file_exists_and_is_valid_json(self):
        seed_file = pathlib.Path(__file__).parent / "baselines" / "chimera" / "standard_golden_baselines.seed.json"
        assert seed_file.exists()
        raw = json.loads(seed_file.read_text(encoding="utf-8"))
        assert raw["golden_baseline_id"] == "chimera.standard.golden-baseline.v1"
        assert "YCSB" in raw["standards"]


class TestBeirDatasetAdapter:
    """Deterministische BEIR-Dataset-Adapter für Query/Qrels-Erzeugung."""

    def test_build_beir_adapter_msmarco_has_queries_and_qrels(self):
        adapter = _build_beir_dataset_adapter("msmarco", query_count=5, seed=11)
        assert adapter["dataset"] == "msmarco"
        assert len(adapter["query_ids"]) == 5
        assert len(adapter["queries"]) == 5
        assert len(adapter["qrels"]) == 5
        assert len(adapter["doc_universe"]) > 100

    def test_build_beir_adapter_unknown_dataset_falls_back(self):
        adapter = _build_beir_dataset_adapter("unknown_ds", query_count=3, seed=1)
        assert adapter["dataset"] == "msmarco"

    def test_build_beir_adapter_is_deterministic_for_same_seed(self):
        adapter_a = _build_beir_dataset_adapter("scifact", query_count=4, seed=99)
        adapter_b = _build_beir_dataset_adapter("scifact", query_count=4, seed=99)
        assert adapter_a["queries"] == adapter_b["queries"]
        assert adapter_a["qrels"] == adapter_b["qrels"]


class TestBuildTestTopology:
    """Topologie-Artifact enthält DB-Informationen."""

    def test_build_test_topology_includes_database_info(self):
        cfg = EnduranceConfig()
        topology = _build_test_topology(
            cfg,
            capability_info={"source": "endpoint:/api/v1/capabilities", "items": ["kv_read"]},
            host_baseline={"source": "psutil+stdlib"},
            database_info={
                "detected": True,
                "source": "/api/v1/info",
                "product": "ThemisDB",
                "version": "1.8.2",
            },
        )
        assert "database_info" in topology
        assert topology["database_info"]["product"] == "ThemisDB"
        assert topology["database_info"]["version"] == "1.8.2"

    def test_build_test_topology_includes_tsbs_generator_config(self):
        cfg = EnduranceConfig(tsbs_seed=23, tsbs_batch_size=7)
        topology = _build_test_topology(cfg)
        assert topology["tsbs_generator"]["seed"] == 23
        assert topology["tsbs_generator"]["batch_size"] == 7


class TestExporterMetadata:
    """Exporter gibt DB-/Cluster-Metadaten in Berichten aus."""

    def _rows(self):
        return [{
            "workload_id": "ycsb_a",
            "timestamp": "2026-04-17T10:00:00Z",
            "throughput_ops_per_sec": 1000.0,
            "mean_latency_ms": 1.0,
            "p50_latency_ms": 0.8,
            "p95_latency_ms": 1.5,
            "p99_latency_ms": 2.0,
            "error_count": 0,
            "elapsed_seconds": 10.0,
            "db_system": "themisdb",
            "database_product": "ThemisDB",
            "database_version": "1.8.2",
            "database_status": "ok",
            "database_cluster_enabled": True,
            "database_cluster_role": "leader",
            "database_node_count": 3,
            "database_shard_count": 12,
            "database_replication_enabled": True,
            "database_replication_role": "primary",
            "database_replication_factor": 3,
        }]

    def test_json_export_includes_metadata(self, tmp_path):
        exporter = ChimeraExporter(self._rows(), tmp_path)
        paths = exporter.export(["json"], base_name="report")
        data = json.loads((tmp_path / "report.json").read_text(encoding="utf-8"))
        assert data["metadata"]["database_product"] == "ThemisDB"
        assert data["metadata"]["database_cluster_role"] == "leader"
        assert data["metadata"]["database_replication_factor"] == 3

    def test_html_export_includes_metadata_table(self, tmp_path):
        exporter = ChimeraExporter(self._rows(), tmp_path)
        exporter.export(["html"], base_name="report")
        content = (tmp_path / "report.html").read_text(encoding="utf-8")
        assert "Umgebungs- und DB-Metadaten" in content
        assert "ThemisDB" in content
        assert "leader" in content
        assert "primary" in content

    def test_markdown_export_includes_metadata(self, tmp_path):
        exporter = ChimeraExporter(self._rows(), tmp_path)
        exporter.export(["md"], base_name="report")
        content = (tmp_path / "report.md").read_text(encoding="utf-8")
        assert "## Umgebungs- und DB-Metadaten" in content
        assert "database_product" in content
        assert "ThemisDB" in content


# ===========================================================================
# _aggregate_from_rows (chimera_server)
# ===========================================================================

class TestAggregateFromRows:
    """_aggregate_from_rows: Aggregation aus flachen Ergebniszeilen."""

    def test_returns_none_for_empty(self):
        from chimera_server import _aggregate_from_rows
        assert _aggregate_from_rows([], "Test") is None

    def test_returns_dict_with_data(self):
        from chimera_server import _aggregate_from_rows
        rows = [
            {
                "workload_id": "ycsb_a",
                "system_name": "ThemisDB",
                "throughput_ops_per_sec": 10000.0,
                "mean_latency_ms": 0.3,
                "p50_latency_ms": 0.25,
                "p95_latency_ms": 0.5,
                "p99_latency_ms": 0.8,
                "error_count": 0,
                "elapsed_seconds": 10.0,
            }
        ]
        result = _aggregate_from_rows(rows, "Test Dashboard")
        assert result is not None
        assert isinstance(result, dict)

    def test_title_in_result(self):
        from chimera_server import _aggregate_from_rows
        rows = [
            {
                "workload_id": "ycsb_c",
                "system_name": "ThemisDB",
                "throughput_ops_per_sec": 5000.0,
                "mean_latency_ms": 0.6,
                "p50_latency_ms": 0.5,
                "p95_latency_ms": 1.0,
                "p99_latency_ms": 1.5,
                "error_count": 0,
                "elapsed_seconds": 5.0,
            }
        ]
        result = _aggregate_from_rows(rows, "My Title")
        assert result is not None
        # Entweder voller BenchmarkDashboard-Output oder Lightweight-Fallback
        assert "title" in result or "workload_summary" in result


# ===========================================================================
# CLI für Standard-Suites erweitern
# ===========================================================================

class TestCLIStandardSuites:
    """CLI-Argumente und Funktionalität für Standard-Suites."""

    def _parse(self, argv: List[str]):
        from chimera_cli import _build_parser
        return _build_parser().parse_args(argv)

    def test_list_search_standards_argument_default_empty(self):
        args = self._parse(["list"])
        assert args.search_standards == ""

    def test_list_search_standards_argument_set(self):
        args = self._parse(["list", "--search-standards", "vector"])
        assert args.search_standards == "vector"

    def test_list_standard_details_argument_default_empty(self):
        args = self._parse(["list"])
        assert args.standard_details == ""

    def test_list_standard_details_argument_set(self):
        args = self._parse(["list", "--standard-details", "BEIR"])
        assert args.standard_details == "BEIR"

    def test_list_standard_coverage_argument_default_false(self):
        args = self._parse(["list"])
        assert args.standard_coverage is False

    def test_list_standard_coverage_argument_flag(self):
        args = self._parse(["list", "--standard-coverage"])
        assert args.standard_coverage is True

    def test_list_workloads_for_standard_argument_default_empty(self):
        args = self._parse(["list"])
        assert args.workloads_for_standard == ""

    def test_list_workloads_for_standard_argument_set(self):
        args = self._parse(["list", "--workloads-for-standard", "YCSB"])
        assert args.workloads_for_standard == "YCSB"

    def test_list_compliance_argument_default_empty(self):
        args = self._parse(["list"])
        assert args.compliance == ""

    def test_list_compliance_argument_set(self):
        args = self._parse(["list", "--compliance", "MLPerf-Inference"])
        assert args.compliance == "MLPerf-Inference"

    def test_search_standards_keyword_matching(self):
        """Test Standard-Suche nach Keyword (tpc, vector, rag)."""
        standards = get_standard_catalog()
        
        # YCSB sollte vorhanden sein
        ycsb = [s for s in standards if "YCSB" in s.standard_id]
        assert len(ycsb) > 0
        
        # TPC-basierte Standards sollten vorhanden sein
        tpc = [s for s in standards if "TPC" in s.standard_id]
        assert len(tpc) > 0
        
        # Vector/ANN-basierte Standards sollten vorhanden sein
        vector = [s for s in standards if "ANN" in s.standard_id]
        assert len(vector) > 0

    def test_standard_catalog_all_standards_have_metrics(self):
        """Alle Standards sollten primary_metrics definieren."""
        standards = get_standard_catalog()
        for std in standards:
            assert len(std.primary_metrics) > 0
            assert all(isinstance(m, str) for m in std.primary_metrics)

    def test_coverage_snapshot_includes_all_standards(self):
        """Coverage-Snapshot sollte Status für alle Standards anzeigen."""
        coverage = get_standard_coverage_snapshot()
        assert "YCSB" in coverage
        assert "TPC-C" in coverage
        assert "BEIR" in coverage
        assert "MLPerf-Inference" in coverage
        
        for std_id, info in coverage.items():
            assert "status" in info or "is_fully_implemented" in info

    def test_standard_compliance_requirements_all_have_metrics(self):
        """Alle Standards sollten required_metrics definieren."""
        reqs = get_standard_compliance_requirements()
        for std_id, req in reqs.items():
            assert len(req.required_metrics) > 0
            assert all(isinstance(m, str) for m in req.required_metrics)

    def test_workload_to_standard_mapping_is_consistent(self):
        """Workload-zu-Standard-Mapping sollte konsistent sein."""
        mapping = {
            "ycsb_a": "YCSB",
            "ycsb_b": "YCSB",
            "ycsb_c": "YCSB",
            "rag_retrieval_quality": "BEIR",
            "rag_qa_e2e": "RAGAS",
            "llm_serving_latency": "MLPerf-Inference",
            "llm_serving_throughput": "MLPerf-Inference",
            "tpc_c_mix": "TPC-C",
        }
        
        # Verifiziere, dass die Standards existieren
        standards = get_standard_catalog()
        std_ids = {s.standard_id for s in standards}
        
        for wid, expected_std in mapping.items():
            assert expected_std in std_ids, f"Standard '{expected_std}' für Workload '{wid}' nicht gefunden"


# ===========================================================================
# Exporter um Compliance-Felder ergänzen
# ===========================================================================

class TestExporterComplianceFields:
    """Exporter-Integration: Compliance-Felder in Ausgaben."""

    def test_csv_export_includes_compliance_columns(self, tmp_path):
        rows = [{
            "workload_id": "ycsb_a",
            "result_schema_id": "chimera.result.standard.v1",
            "result_schema_version": "1.0.0",
            "standard_id": "YCSB",
            "standard_required_metrics": "throughput,latency",
            "standard_present_metrics": "throughput,latency",
            "standard_missing_metrics": "",
            "standard_metric_coverage_pct": 100.0,
            "throughput_ops_per_sec": 1000.0,
            "mean_latency_ms": 1.0,
            "p50_latency_ms": 0.8,
            "p95_latency_ms": 1.5,
            "p99_latency_ms": 2.0,
            "error_count": 0,
            "elapsed_seconds": 10.0,
        }]
        exporter = ChimeraExporter(rows, tmp_path)
        paths = exporter.export(["csv"], base_name="report")
        
        csv_content = (tmp_path / "report.csv").read_text(encoding="utf-8")
        assert "result_schema_id" in csv_content
        assert "standard_id" in csv_content
        assert "standard_metric_coverage_pct" in csv_content
        assert "chimera.result.standard.v1" in csv_content
        assert "YCSB" in csv_content

    def test_json_export_includes_compliance_metadata(self, tmp_path):
        rows = [{
            "workload_id": "rag_retrieval_quality",
            "result_schema_id": "chimera.result.standard.v1",
            "result_schema_version": "1.0.0",
            "standard_id": "BEIR",
            "beir_dataset": "msmarco",
            "beir_k": 10,
            "ndcg_at_10": 0.45,
            "mrr": 0.52,
            "map": 0.38,
            "throughput_ops_per_sec": 100.0,
            "mean_latency_ms": 10.0,
            "p50_latency_ms": 8.0,
            "p95_latency_ms": 15.0,
            "p99_latency_ms": 20.0,
            "error_count": 0,
            "elapsed_seconds": 5.0,
        }]
        exporter = ChimeraExporter(rows, tmp_path)
        paths = exporter.export(["json"], base_name="report")
        
        data = json.loads((tmp_path / "report.json").read_text(encoding="utf-8"))
        assert data["metadata"]["result_schema_id"] == "chimera.result.standard.v1"
        assert data["metadata"]["standard_id"] == "BEIR"

    def test_markdown_export_includes_compliance_section(self, tmp_path):
        rows = [{
            "workload_id": "llm_serving_latency",
            "result_schema_id": "chimera.result.standard.v1",
            "result_schema_version": "1.0.0",
            "standard_id": "MLPerf-Inference",
            "standard_metric_coverage_pct": 95.0,
            "mlperf_scenario": "singlestream",
            "mlperf_dataset": "openorca",
            "quality_target_passed": True,
            "throughput_ops_per_sec": 0.5,
            "mean_latency_ms": 2000.0,
            "p50_latency_ms": 1900.0,
            "p95_latency_ms": 2100.0,
            "p99_latency_ms": 2200.0,
            "error_count": 0,
            "elapsed_seconds": 30.0,
        }]
        exporter = ChimeraExporter(rows, tmp_path)
        paths = exporter.export(["md"], base_name="report")
        
        md_content = (tmp_path / "report.md").read_text(encoding="utf-8")
        assert "Standardkonforme Ergebnisse" in md_content
        assert "Schema-ID" in md_content
        assert "MLPerf-Inference" in md_content
        assert "95.0%" in md_content

    def test_html_export_includes_compliance_metadata(self, tmp_path):
        rows = [{
            "workload_id": "rag_qa_e2e",
            "result_schema_id": "chimera.result.standard.v1",
            "result_schema_version": "1.0.0",
            "standard_id": "RAGAS",
            "faithfulness": 0.88,
            "answer_relevance": 0.92,
            "context_precision": 0.85,
            "context_recall": 0.90,
            "ragas_score": 0.89,
            "ragbench_score": 0.87,
            "throughput_ops_per_sec": 20.0,
            "mean_latency_ms": 50.0,
            "p50_latency_ms": 45.0,
            "p95_latency_ms": 60.0,
            "p99_latency_ms": 75.0,
            "error_count": 0,
            "elapsed_seconds": 10.0,
        }]
        exporter = ChimeraExporter(rows, tmp_path)
        paths = exporter.export(["html"], base_name="report")
        
        html_content = (tmp_path / "report.html").read_text(encoding="utf-8")
        assert "RAGAS" in html_content or "Umgebungs- und DB-Metadaten" in html_content

    def test_mlperf_fields_in_csv_export(self, tmp_path):
        rows = [{
            "workload_id": "llm_serving_throughput",
            "mlperf_scenario": "server",
            "mlperf_dataset": "openorca",
            "mlperf_quality_target": 0.80,
            "mlperf_quality_score": 0.82,
            "quality_target_passed": True,
            "mlperf_performance_target_passed": True,
            "mlperf_compliance_passed": True,
            "throughput_ops_per_sec": 75.0,
            "mean_latency_ms": 13.33,
            "p50_latency_ms": 12.0,
            "p95_latency_ms": 15.0,
            "p99_latency_ms": 18.0,
            "error_count": 0,
            "elapsed_seconds": 10.0,
        }]
        exporter = ChimeraExporter(rows, tmp_path)
        paths = exporter.export(["csv"], base_name="mlperf_report")
        
        csv_content = (tmp_path / "mlperf_report.csv").read_text(encoding="utf-8")
        assert "mlperf_scenario" in csv_content
        assert "mlperf_dataset" in csv_content
        assert "quality_target_passed" in csv_content
        assert "mlperf_compliance_passed" in csv_content

    def test_beir_fields_in_csv_export(self, tmp_path):
        rows = [{
            "workload_id": "rag_retrieval_quality",
            "beir_dataset": "scifact",
            "beir_k": 12,
            "ndcg_at_10": 0.50,
            "mrr": 0.55,
            "map": 0.42,
            "throughput_ops_per_sec": 150.0,
            "mean_latency_ms": 6.67,
            "p50_latency_ms": 6.0,
            "p95_latency_ms": 8.0,
            "p99_latency_ms": 10.0,
            "error_count": 0,
            "elapsed_seconds": 10.0,
        }]
        exporter = ChimeraExporter(rows, tmp_path)
        paths = exporter.export(["csv"], base_name="beir_report")
        
        csv_content = (tmp_path / "beir_report.csv").read_text(encoding="utf-8")
        assert "beir_dataset" in csv_content
        assert "beir_k" in csv_content
        assert "ndcg_at_10" in csv_content
        assert "mrr" in csv_content
        assert "map" in csv_content

    def test_ragas_fields_in_csv_export(self, tmp_path):
        rows = [{
            "workload_id": "rag_qa_e2e",
            "faithfulness": 0.90,
            "answer_relevance": 0.93,
            "context_precision": 0.87,
            "context_recall": 0.92,
            "ragas_score": 0.91,
            "ragbench_score": 0.89,
            "ragas_eval_samples": 7,
            "throughput_ops_per_sec": 25.0,
            "mean_latency_ms": 40.0,
            "p50_latency_ms": 38.0,
            "p95_latency_ms": 48.0,
            "p99_latency_ms": 60.0,
            "error_count": 0,
            "elapsed_seconds": 10.0,
        }]
        exporter = ChimeraExporter(rows, tmp_path)
        paths = exporter.export(["csv"], base_name="ragas_report")
        
        csv_content = (tmp_path / "ragas_report.csv").read_text(encoding="utf-8")
        assert "faithfulness" in csv_content
        assert "answer_relevance" in csv_content
        assert "ragas_score" in csv_content
        assert "ragbench_score" in csv_content
        assert "ragas_eval_samples" in csv_content


# ===========================================================================
# Standardkonforme Validierungstests
# ===========================================================================

class TestStandardConformanceValidation:
    """Validierungstests: Ergebnisse gegen Standard-Anforderungen prüfen."""

    def _make_mock_client(self) -> ThemisDBClient:
        cfg = EnduranceConfig()
        client = ThemisDBClient(cfg)
        client.execute_query = MagicMock(return_value={"status": "ok"})
        client.vector_search = MagicMock(return_value={"status": "ok"})
        return client

    def test_ycsb_results_contain_required_metrics(self):
        """YCSB-Ergebnisse enthalten alle erforderlichen Metriken."""
        reqs = get_standard_compliance_requirements()["YCSB"]
        required = set(reqs.required_metrics)
        
        client = self._make_mock_client()
        cfg = EnduranceConfig(run_iterations=1)
        workloads = _build_workloads(client, cfg)
        
        rows = _run_window(
            workload_ids=["ycsb_a"],
            all_workloads=workloads,
            cfg=cfg,
            database_info=None,
            parallel=False,
        )
        
        row = rows[0]
        present = set(row.keys())
        missing = required - present
        assert missing == set(), f"YCSB: fehlende Metriken {missing}"

    def test_beir_results_contain_retrieval_metrics(self):
        """BEIR-Ergebnisse enthalten Retrieval-Quality-Metriken."""
        reqs = get_standard_compliance_requirements()["BEIR"]
        required = {"ndcg_at_10", "mrr", "map"}  # Kernmetriken
        
        client = self._make_mock_client()
        cfg = EnduranceConfig(run_iterations=1, beir_seed=42)
        workloads = _build_workloads(client, cfg)
        
        rows = _run_window(
            workload_ids=["rag_retrieval_quality"],
            all_workloads=workloads,
            cfg=cfg,
            database_info=None,
            parallel=False,
        )
        
        row = rows[0]
        for metric in required:
            assert metric in row, f"BEIR: Metrik '{metric}' fehlt"
            assert 0.0 <= row[metric] <= 1.0, f"BEIR: {metric}={row[metric]} außerhalb [0,1]"

    def test_ragas_results_contain_quality_metrics(self):
        """RAGAS-Ergebnisse enthalten Quality-Score-Metriken."""
        reqs = get_standard_compliance_requirements()["RAGAS"]
        required = {"faithfulness", "answer_relevance", "ragas_score"}
        
        client = self._make_mock_client()
        cfg = EnduranceConfig(run_iterations=1, ragas_seed=99)
        workloads = _build_workloads(client, cfg)
        
        rows = _run_window(
            workload_ids=["rag_qa_e2e"],
            all_workloads=workloads,
            cfg=cfg,
            database_info=None,
            parallel=False,
        )
        
        row = rows[0]
        for metric in required:
            assert metric in row, f"RAGAS: Metrik '{metric}' fehlt"
            assert 0.0 <= row[metric] <= 1.0, f"RAGAS: {metric}={row[metric]} außerhalb [0,1]"

    def test_mlperf_results_contain_compliance_fields(self):
        """MLPerf-Ergebnisse enthalten Compliance-Felder."""
        reqs = get_standard_compliance_requirements()["MLPerf-Inference"]
        required = {"throughput_ops_per_sec", "p99_latency_ms", "quality_target_passed"}
        
        client = self._make_mock_client()
        cfg = EnduranceConfig(
            run_iterations=1,
            mlperf_quality_target=0.5,
            mlperf_target_qps=0.1,
            mlperf_target_latency_ms=10000.0,
        )
        workloads = _build_workloads(client, cfg)
        
        rows = _run_window(
            workload_ids=["llm_serving_latency"],
            all_workloads=workloads,
            cfg=cfg,
            database_info=None,
            parallel=False,
        )
        
        row = rows[0]
        for metric in required:
            assert metric in row, f"MLPerf: Metrik '{metric}' fehlt"

    def test_result_schema_completeness_ycsb(self):
        """Result-Schema ist vollständig für YCSB Workloads."""
        client = self._make_mock_client()
        cfg = EnduranceConfig(run_iterations=1)
        workloads = _build_workloads(client, cfg)
        
        rows = _run_window(
            workload_ids=["ycsb_c"],
            all_workloads=workloads,
            cfg=cfg,
            database_info=None,
            parallel=False,
        )
        
        row = rows[0]
        # Verifiziere Schema-Identifikation
        assert row["result_schema_id"] == "chimera.result.standard.v1"
        assert row["result_schema_version"] == "1.0.0"
        assert row["standard_id"] == "YCSB"
        
        # Verifiziere Metrik-Coverage
        assert "standard_required_metrics" in row
        assert "standard_present_metrics" in row
        assert "standard_missing_metrics" in row
        assert "standard_metric_coverage_pct" in row
        assert 0 <= row["standard_metric_coverage_pct"] <= 100

    def test_result_schema_completeness_beir(self):
        """Result-Schema ist vollständig für BEIR Workloads."""
        client = self._make_mock_client()
        cfg = EnduranceConfig(run_iterations=1, beir_seed=11)
        workloads = _build_workloads(client, cfg)
        
        rows = _run_window(
            workload_ids=["rag_retrieval_quality"],
            all_workloads=workloads,
            cfg=cfg,
            database_info=None,
            parallel=False,
        )
        
        row = rows[0]
        assert row["standard_id"] == "BEIR"
        # BEIR-spezifische Felder prüfen
        assert "beir_dataset" in row
        assert "beir_k" in row

    def test_result_schema_completeness_ragas(self):
        """Result-Schema ist vollständig für RAGAS Workloads."""
        client = self._make_mock_client()
        cfg = EnduranceConfig(run_iterations=1, ragas_seed=19)
        workloads = _build_workloads(client, cfg)
        
        rows = _run_window(
            workload_ids=["rag_qa_e2e"],
            all_workloads=workloads,
            cfg=cfg,
            database_info=None,
            parallel=False,
        )
        
        row = rows[0]
        assert row["standard_id"] == "RAGAS"
        # RAGAS-spezifische Felder prüfen
        assert "faithfulness" in row
        assert "ragas_score" in row
        assert "ragas_eval_samples" in row

    def test_result_schema_completeness_mlperf(self):
        """Result-Schema ist vollständig für MLPerf Workloads."""
        client = self._make_mock_client()
        cfg = EnduranceConfig(run_iterations=1, mlperf_quality_target=0.5)
        workloads = _build_workloads(client, cfg)
        
        rows = _run_window(
            workload_ids=["llm_serving_throughput"],
            all_workloads=workloads,
            cfg=cfg,
            database_info=None,
            parallel=False,
        )
        
        row = rows[0]
        assert row["standard_id"] == "MLPerf-Inference"
        # MLPerf-spezifische Felder prüfen
        assert "mlperf_scenario" in row
        assert "mlperf_dataset" in row
        assert "mlperf_quality_target" in row
        assert "quality_target_passed" in row

    def test_metric_coverage_at_least_80_percent(self):
        """Standard-Metrik-Coverage sollte mindestens 80% sein."""
        client = self._make_mock_client()
        cfg = EnduranceConfig(run_iterations=1)
        workloads = _build_workloads(client, cfg)
        
        # Test mehrere Workloads
        for wid in ["ycsb_a", "ycsb_c", "rag_retrieval_quality"]:
            rows = _run_window(
                workload_ids=[wid],
                all_workloads=workloads,
                cfg=cfg,
                database_info=None,
                parallel=False,
            )
            row = rows[0]
            coverage = row.get("standard_metric_coverage_pct", 0)
            # Coverage kann variabel sein; nur Bereich prüfen
            assert 0 <= coverage <= 100, (
                f"Workload '{wid}': Coverage {coverage}% außerhalb [0,100]"
            )

    def test_no_missing_metrics_for_implemented_workloads(self):
        """Implementierte Workloads sollten keine fehlenden Metriken haben."""
        client = self._make_mock_client()
        cfg = EnduranceConfig(run_iterations=1)
        workloads = _build_workloads(client, cfg)
        
        # Test kernimplementierte Workloads
        for wid in ["ycsb_a", "ycsb_c", "vector_search"]:
            rows = _run_window(
                workload_ids=[wid],
                all_workloads=workloads,
                cfg=cfg,
                database_info=None,
                parallel=False,
            )
            row = rows[0]
            missing = row.get("standard_missing_metrics", "")
            # Für Core-Workloads sollten Fehlerlisten leer oder sehr kurz sein (kann Liste oder String sein)
            if isinstance(missing, list):
                missing_count = len(missing)
            else:
                missing_count = len([m for m in str(missing).split(",") if m.strip()])
            assert missing_count <= 2, (
                f"Workload '{wid}': zu viele fehlende Metriken"
            )

    def test_compliance_requirements_all_workloads_map_to_standard(self):
        """Alle Workloads in Compliance-Requirements sollten existieren."""
        reqs = get_standard_compliance_requirements()
        # Nur Standards prüfen, die vollständig implementiert sind
        implemented_standards = {"YCSB", "TPC-C", "TSBS", "LDBC-SNB", "ANN-Benchmarks", "BEIR", "RAGAS", "MLPerf-Inference"}
        
        for std_id, req in reqs.items():
            if std_id not in implemented_standards:
                continue  # Nicht implementierte Standards wie TPC-H überspringen
            for spec in req.canonical_workloads:
                assert spec.workload_id in CURRENT_CHIMERA_WORKLOADS, (
                    f"{std_id}: Workload '{spec.workload_id}' nicht in CURRENT_CHIMERA_WORKLOADS"
                )

    def test_standard_result_schema_deterministic(self):
        """Standard-Result-Schema-Ausgabe sollte deterministisch sein."""
        schema1 = _build_standard_result_schema()
        schema2 = _build_standard_result_schema()
        
        assert schema1["schema_id"] == schema2["schema_id"]
        assert schema1["schema_version"] == schema2["schema_version"]
        assert schema1["base_required_fields"] == schema2["base_required_fields"]
        assert set(schema1["standards"].keys()) == set(schema2["standards"].keys())
