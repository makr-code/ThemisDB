"""
CHIMERA Benchmark Standards and Profile Catalog
===============================================
Katalog etablierter Benchmark-Standards fuer vendor-neutrale Profile.

Ziel:
- Standards transparent machen (YCSB, TPC, TSBS, LDBC, ANN-Benchmarks, BEIR, MLPerf)
- Domains in wiederverwendbare Profile gruppieren
- Sichtbar machen, welche Workloads bereits in CHIMERA verfuegbar sind
"""

from __future__ import annotations

from dataclasses import dataclass, field
from typing import Any, Dict, List, Tuple


@dataclass(frozen=True)
class BenchmarkStandard:
    """Beschreibt einen etablierten Benchmark-Standard."""

    standard_id: str
    name: str
    domain: str
    primary_metrics: List[str] = field(default_factory=list)
    notes: str = ""


@dataclass(frozen=True)
class BenchmarkProfile:
    """Domain-Profil mit zugehoerigen Standards und Workloads."""

    profile_id: str
    description: str
    standards: List[str] = field(default_factory=list)
    required_workloads: List[str] = field(default_factory=list)
    optional_workloads: List[str] = field(default_factory=list)


@dataclass(frozen=True)
class WorkloadComplianceSpec:
    """Standardkonforme Spezifikation eines einzelnen Workload-Profils."""

    workload_id: str
    canonical_name: str
    operation_mix: Dict[str, int] = field(default_factory=dict)
    distribution: str = ""
    required_dataset_shape: Dict[str, int] = field(default_factory=dict)
    notes: str = ""


@dataclass(frozen=True)
class StandardComplianceRequirements:
    """Maschinenlesbare Mindestanforderungen für eine Standard-Integration."""

    standard_id: str
    citation: str
    canonical_workloads: List[WorkloadComplianceSpec] = field(default_factory=list)
    required_metrics: List[str] = field(default_factory=list)
    required_parameters: List[str] = field(default_factory=list)
    measurement_protocol: Dict[str, int] = field(default_factory=dict)
    implementation_notes: List[str] = field(default_factory=list)


# Bereits implementierte Workloads im aktuellen CHIMERA-Runner
CURRENT_CHIMERA_WORKLOADS: List[str] = [
    "ycsb_a",
    "ycsb_b",
    "ycsb_c",
    "ycsb_d",
    "ycsb_e",
    "ycsb_f",
    "vector_search",
    "graph_traversal",
    "ldbc_ic1_friends_of_person",
    "ldbc_ic2_recent_messages",
    "ldbc_ic3_friends_abroad",
    "ldbc_ic6_tag_cocreators",
    "ldbc_ic9_recent_forum_posts",
    "ldbc_ic14_shortest_path",
    "ldbc_is1_person_profile",
    "ldbc_lu1_add_person",
    "ts_ingest_raw",
    "ts_ingest_batch",
    "ts_range_query",
    "ts_lastpoint",
    "ts_downsample",
    "ts_high_cpu",
    "ts_groupby",
    "geo_intersects",
    "geo_contains",
    "geo_range_query",
    "process_discovery_alpha",
    "process_discovery_heuristic",
    "process_conformance",
    "llm_serving_latency",
    "llm_serving_throughput",
    "ml_policy_eval",
    "ethics_eval_pipeline",
    "lora_load_apply",
    "lora_switch_overhead",
    "rag_retrieval_quality",
    "rag_qa_e2e",
    "tpc_c_mix",
    "ann_recall_at_k",
    "ann_range_filter",
    "ann_batch_query",
]


def get_standard_catalog() -> List[BenchmarkStandard]:
    """Liefert den Standard-Katalog fuer Spezialdomaenen."""
    return [
        BenchmarkStandard(
            standard_id="YCSB",
            name="Yahoo! Cloud Serving Benchmark",
            domain="oltp-keyvalue",
            primary_metrics=["throughput_ops_per_sec", "p95_latency_ms", "p99_latency_ms"],
            notes="Workloads A-F fuer Read/Write/Scan/RMW-Mixe.",
        ),
        BenchmarkStandard(
            standard_id="TPC-C",
            name="TPC-C OLTP",
            domain="oltp-transaction",
            primary_metrics=["tpmC", "latency_ms", "abort_rate"],
            notes="5-Transaktions-Mix (NewOrder, Payment, OrderStatus, Delivery, StockLevel).",
        ),
        BenchmarkStandard(
            standard_id="TPC-H",
            name="TPC-H Decision Support",
            domain="olap",
            primary_metrics=["qphh", "query_latency_ms"],
            notes="Ad-hoc Query Suite fuer analytische Lasten.",
        ),
        BenchmarkStandard(
            standard_id="TSBS",
            name="Time Series Benchmark Suite",
            domain="timeseries",
            primary_metrics=["ingest_rate", "query_latency_ms", "scan_latency_ms"],
            notes="DevOps/IoT Use-Cases, deterministische Datengenerierung.",
        ),
        BenchmarkStandard(
            standard_id="LDBC-SNB",
            name="LDBC Social Network Benchmark",
            domain="graph",
            primary_metrics=["throughput_ops_per_sec", "interactive_latency_ms"],
            notes="Interactive + BI Workloads auf gemeinsamem Graph-Dataset.",
        ),
        BenchmarkStandard(
            standard_id="ANN-Benchmarks",
            name="Approximate Nearest Neighbor Benchmarks",
            domain="vector",
            primary_metrics=["recall_at_k", "qps", "latency_ms"],
            notes="Vergleich ANN-Indizes ueber Recall/Speed Frontier.",
        ),
        BenchmarkStandard(
            standard_id="BEIR",
            name="Benchmark for Information Retrieval",
            domain="rag-retrieval",
            primary_metrics=["ndcg_at_10", "mrr", "map"],
            notes="Heterogene IR-Datasets fuer Zero-Shot Retrieval.",
        ),
        BenchmarkStandard(
            standard_id="MLPerf-Inference",
            name="MLPerf Inference",
            domain="ml-llm-serving",
            primary_metrics=["throughput", "latency", "quality_target"],
            notes="Szenario- und Regelwerk fuer reproduzierbare Inferenzmessung.",
        ),
        BenchmarkStandard(
            standard_id="RAGAS",
            name="RAGAS Metrics",
            domain="rag-quality",
            primary_metrics=["faithfulness", "answer_relevance", "context_precision", "context_recall"],
            notes="Qualitaetsmetriken fuer RAG-Pipelines.",
        ),
    ]


def get_standard_compliance_requirements() -> Dict[str, StandardComplianceRequirements]:
    """Liefert standardkonforme Mindestanforderungen für CHIMERA-Integrationen.

    Die Spezifikationen werden iterativ erweitert und als maschinenlesbare
    Zielanforderungen fuer Runtime- und Report-Compliance gepflegt.
    """
    return {
        "YCSB": StandardComplianceRequirements(
            standard_id="YCSB",
            citation="Cooper et al., SoCC 2010, DOI: 10.1145/1807128.1807152",
            canonical_workloads=[
                WorkloadComplianceSpec(
                    workload_id="ycsb_a",
                    canonical_name="Workload A",
                    operation_mix={"read": 50, "update": 50, "insert": 0, "scan": 0},
                    distribution="zipfian",
                    required_dataset_shape={"record_count": 1_000_000, "field_count": 10, "field_length": 100},
                    notes="Update-heavy workload.",
                ),
                WorkloadComplianceSpec(
                    workload_id="ycsb_b",
                    canonical_name="Workload B",
                    operation_mix={"read": 95, "update": 5, "insert": 0, "scan": 0},
                    distribution="zipfian",
                    required_dataset_shape={"record_count": 1_000_000, "field_count": 10, "field_length": 100},
                    notes="Read-heavy workload.",
                ),
                WorkloadComplianceSpec(
                    workload_id="ycsb_c",
                    canonical_name="Workload C",
                    operation_mix={"read": 100, "update": 0, "insert": 0, "scan": 0},
                    distribution="zipfian",
                    required_dataset_shape={"record_count": 1_000_000, "field_count": 10, "field_length": 100},
                    notes="Read-only workload.",
                ),
                WorkloadComplianceSpec(
                    workload_id="ycsb_d",
                    canonical_name="Workload D",
                    operation_mix={"read": 95, "update": 0, "insert": 5, "scan": 0},
                    distribution="latest",
                    required_dataset_shape={"record_count": 1_000_000, "field_count": 10, "field_length": 100},
                    notes="Read-latest workload.",
                ),
                WorkloadComplianceSpec(
                    workload_id="ycsb_e",
                    canonical_name="Workload E",
                    operation_mix={"read": 0, "update": 0, "insert": 5, "scan": 95},
                    distribution="zipfian",
                    required_dataset_shape={"record_count": 1_000_000, "field_count": 10, "field_length": 100},
                    notes="Scan-heavy workload.",
                ),
                WorkloadComplianceSpec(
                    workload_id="ycsb_f",
                    canonical_name="Workload F",
                    operation_mix={"read": 50, "read_modify_write": 50, "insert": 0, "scan": 0},
                    distribution="zipfian",
                    required_dataset_shape={"record_count": 1_000_000, "field_count": 10, "field_length": 100},
                    notes="Read-modify-write workload.",
                ),
            ],
            required_metrics=[
                "throughput_ops_per_sec",
                "mean_latency_ms",
                "p95_latency_ms",
                "p99_latency_ms",
            ],
            required_parameters=[
                "record_count",
                "operation_count",
                "field_count",
                "field_length",
                "thread_count",
                "target_throughput",
                "distribution",
            ],
            measurement_protocol={
                "warm_up_seconds": 60,
                "measurement_duration_seconds": 300,
                "num_runs": 5,
            },
            implementation_notes=[
                "Alle Workloads A-F müssen abbildbar sein.",
                "Distributionen zipfian und latest müssen unterstützt werden.",
                "Dataset-Shape muss reproduzierbar und standardnah sein.",
                "CHIMERA darf zusätzliche Metadaten erfassen, aber die YCSB-Kernmetriken nicht ersetzen.",
            ],
        ),
        "TPC-C": StandardComplianceRequirements(
            standard_id="TPC-C",
            citation="TPC Benchmark C, Revision 5.11",
            canonical_workloads=[
                WorkloadComplianceSpec(
                    workload_id="tpc_c_mix",
                    canonical_name="TPC-C Transaction Mix",
                    operation_mix={
                        "new_order": 45,
                        "payment": 43,
                        "order_status": 4,
                        "delivery": 4,
                        "stock_level": 4,
                    },
                    distribution="tpc_c_mix",
                    required_dataset_shape={"warehouses": 10},
                    notes="Standard TPC-C transaction distribution.",
                ),
            ],
            required_metrics=[
                "throughput_ops_per_sec",
                "mean_latency_ms",
                "p95_latency_ms",
                "p99_latency_ms",
                "error_count",
            ],
            required_parameters=[
                "warehouses",
                "thread_count",
                "duration_seconds",
                "ramp_up_seconds",
                "think_time_ms",
            ],
            measurement_protocol={
                "warm_up_seconds": 60,
                "measurement_duration_seconds": 300,
                "num_runs": 5,
            },
            implementation_notes=[
                "Die 5 TPC-C Kerntransaktionen müssen im Standard-Mix enthalten sein.",
                "CHIMERA nutzt aktuell throughput_ops_per_sec als neutrale Primärmetrik; tpmC ist als Zusatzmetrik möglich.",
                "Dataset-Generierung und Warehouse-Skalierung müssen reproduzierbar sein.",
            ],
        ),
        "TSBS": StandardComplianceRequirements(
            standard_id="TSBS",
            citation="Kunjir et al., VLDB 2017; github.com/timescale/tsbs",
            canonical_workloads=[
                WorkloadComplianceSpec(
                    workload_id="ts_ingest_raw",
                    canonical_name="TSBS Write (single-point)",
                    operation_mix={"insert": 100},
                    distribution="sequential_seeded",
                    required_dataset_shape={"entity_count": 200, "metrics_per_entity": 5},
                    notes="Einzel-Datenpunkt-Schreiben nach TSBS DevOps-Schema.",
                ),
                WorkloadComplianceSpec(
                    workload_id="ts_ingest_batch",
                    canonical_name="TSBS Write (batch)",
                    operation_mix={"insert": 100},
                    distribution="sequential_seeded",
                    required_dataset_shape={"entity_count": 200, "batch_size": 50},
                    notes="Batch-Schreiben; Batch-Größe gemäß Konfiguration.",
                ),
                WorkloadComplianceSpec(
                    workload_id="ts_range_query",
                    canonical_name="TSBS simple-range",
                    operation_mix={"range_query": 100},
                    distribution="uniform_time_range",
                    required_dataset_shape={"query_range_ms": 3_600_000},
                    notes="Einfache Bereichsabfrage über einen konfigurierten Zeitbereich.",
                ),
                WorkloadComplianceSpec(
                    workload_id="ts_lastpoint",
                    canonical_name="TSBS last-point",
                    operation_mix={"last_point_query": 100},
                    distribution="latest_per_entity",
                    required_dataset_shape={"entity_count": 200},
                    notes="Letzter bekannter Wert je Entity – typische IoT/Monitoring-Abfrage.",
                ),
                WorkloadComplianceSpec(
                    workload_id="ts_downsample",
                    canonical_name="TSBS simple-downsampling",
                    operation_mix={"aggregate_query": 100},
                    distribution="uniform_time_range",
                    required_dataset_shape={"bucket_ms": 60_000},
                    notes="Aggregation (AVG) über Zeitbuckets – TSBS DevOps-Downsampling.",
                ),
                WorkloadComplianceSpec(
                    workload_id="ts_high_cpu",
                    canonical_name="TSBS high-cpu-all",
                    operation_mix={"threshold_query": 100},
                    distribution="uniform_time_range",
                    required_dataset_shape={"entity_count": 200},
                    notes="Filterabfrage: Entities mit cpu_usage oberhalb eines Schwellwerts.",
                ),
                WorkloadComplianceSpec(
                    workload_id="ts_groupby",
                    canonical_name="TSBS double-groupby",
                    operation_mix={"groupby_query": 100},
                    distribution="uniform_time_range",
                    required_dataset_shape={"bucket_ms": 60_000},
                    notes="Doppeltes Group-By nach Entity und Zeitbucket (MAX/MIN/AVG).",
                ),
            ],
            required_metrics=[
                "throughput_ops_per_sec",
                "mean_latency_ms",
                "p95_latency_ms",
                "p99_latency_ms",
                "ingest_rate_points_per_sec",
            ],
            required_parameters=[
                "entity_count",
                "step_ms",
                "start_time_ms",
                "seed",
                "metrics",
                "query_range_ms",
                "batch_size",
                "bucket_ms",
            ],
            measurement_protocol={
                "warm_up_seconds": 30,
                "measurement_duration_seconds": 300,
                "num_runs": 5,
            },
            implementation_notes=[
                "Alle 7 TSBS-Workload-Klassen (ingest_raw, ingest_batch, range, lastpoint, downsample, high_cpu, groupby) implementiert.",
                "Seed-basierter Generator für reproduzierbare Datenformen und Zeitreihenverläufe.",
                "DevOps-inspiriertes Schema: mehrere Metriken (cpu_usage, memory_usage, disk_io, net_rx, net_tx) pro Entity (Host).",
            ],
        ),
        "TPC-H": StandardComplianceRequirements(
            standard_id="TPC-H",
            citation="TPC Benchmark H, Revision 3.0.0",
            canonical_workloads=[
                WorkloadComplianceSpec(
                    workload_id="tpc_h_sf1",
                    canonical_name="TPC-H Scale Factor 1",
                    operation_mix={
                        "q1": 1,
                        "q2": 1,
                        "q3": 1,
                        "q4": 1,
                        "q5": 1,
                        "q6": 1,
                        "q7": 1,
                        "q8": 1,
                        "q9": 1,
                        "q10": 1,
                        "q11": 1,
                        "q12": 1,
                        "q13": 1,
                        "q14": 1,
                        "q15": 1,
                        "q16": 1,
                        "q17": 1,
                        "q18": 1,
                        "q19": 1,
                        "q20": 1,
                        "q21": 1,
                        "q22": 1,
                    },
                    distribution="uniform_query_set",
                    required_dataset_shape={"scale_factor": 1},
                    notes="Alle 22 TPC-H Queries gehören zum Kanon; Power- und Throughput-Test sind erforderlich.",
                ),
            ],
            required_metrics=[
                "qphh",
                "power_test_seconds",
                "throughput_test_seconds",
                "query_latency_ms",
            ],
            required_parameters=[
                "scale_factor",
                "query_set",
                "stream_count",
                "refresh_function_enabled",
            ],
            measurement_protocol={
                "power_runs": 1,
                "throughput_streams": 2,
                "num_runs": 3,
            },
            implementation_notes=[
                "TPC-H erfordert den vollständigen Query-Satz Q1-Q22 inklusive standardnaher Ausführungsregeln.",
                "Aktueller CHIMERA-Runtime-Runner enthält noch keinen tpc_h_sf1 Workload.",
                "Diese Spezifikation definiert die Zielanforderungen für eine spätere Runtime-Integration.",
            ],
        ),
        "LDBC-SNB": StandardComplianceRequirements(
            standard_id="LDBC-SNB",
            citation="Erling et al., SIGMOD 2015, DOI: 10.1145/2723372.2742786",
            canonical_workloads=[
                WorkloadComplianceSpec(
                    workload_id="ldbc_ic1_friends_of_person",
                    canonical_name="IC-1: Friends of Person",
                    operation_mix={"read": 100},
                    distribution="uniform_person_id",
                    required_dataset_shape={"scale_factor": 1, "person_count": 10_000},
                    notes="Traversal bis Tiefe 3, geordnet nach Distanz und Name.",
                ),
                WorkloadComplianceSpec(
                    workload_id="ldbc_ic2_recent_messages",
                    canonical_name="IC-2: Recent Messages of Friends",
                    operation_mix={"read": 100},
                    distribution="uniform_person_id_with_date",
                    required_dataset_shape={"scale_factor": 1},
                    notes="Top-20 neueste Nachrichten von Direktfreunden.",
                ),
                WorkloadComplianceSpec(
                    workload_id="ldbc_ic3_friends_abroad",
                    canonical_name="IC-3: Friends Abroad",
                    operation_mix={"read": 100},
                    distribution="uniform_person_id_country_date",
                    required_dataset_shape={"scale_factor": 1},
                    notes="2-Hop-Freunde in bestimmten Ländern über Zeitbereich.",
                ),
                WorkloadComplianceSpec(
                    workload_id="ldbc_ic6_tag_cocreators",
                    canonical_name="IC-6: Tag Co-Creators",
                    operation_mix={"read": 100},
                    distribution="uniform_person_id_tag",
                    required_dataset_shape={"scale_factor": 1},
                    notes="Tags auf Nachrichten im Freundesnetzwerk (2-Hop).",
                ),
                WorkloadComplianceSpec(
                    workload_id="ldbc_ic9_recent_forum_posts",
                    canonical_name="IC-9: Recent Forum Posts",
                    operation_mix={"read": 100},
                    distribution="uniform_person_id_with_date",
                    required_dataset_shape={"scale_factor": 1},
                    notes="Neueste Posts im 2-Hop-Freundesnetzwerk vor Stichtag.",
                ),
                WorkloadComplianceSpec(
                    workload_id="ldbc_ic14_shortest_path",
                    canonical_name="IC-14: Shortest Path",
                    operation_mix={"read": 100},
                    distribution="uniform_person_pair",
                    required_dataset_shape={"scale_factor": 1},
                    notes="Gewichteter kürzester Pfad zwischen zwei Personen.",
                ),
                WorkloadComplianceSpec(
                    workload_id="ldbc_is1_person_profile",
                    canonical_name="IS-1: Person Profile",
                    operation_mix={"read": 100},
                    distribution="uniform_person_id",
                    required_dataset_shape={"scale_factor": 1},
                    notes="Punktabfrage eines einzelnen Person-Knotens.",
                ),
                WorkloadComplianceSpec(
                    workload_id="ldbc_lu1_add_person",
                    canonical_name="LU-1: Add Person",
                    operation_mix={"insert": 100},
                    distribution="sequential_new_id",
                    required_dataset_shape={"scale_factor": 1},
                    notes="Einfügen eines neuen Person-Knotens (Update-Klasse).",
                ),
            ],
            required_metrics=[
                "throughput_ops_per_sec",
                "mean_latency_ms",
                "p95_latency_ms",
                "p99_latency_ms",
            ],
            required_parameters=[
                "scale_factor",
                "person_count",
                "max_date_offset_days",
            ],
            measurement_protocol={
                "warm_up_seconds": 60,
                "measurement_duration_seconds": 300,
                "num_runs": 5,
            },
            implementation_notes=[
                "IC/IS/LU-Workloads nutzen LDBC-SNB Interactive Spezifikation v1.",
                "Graph-Queries sind gegen generisches Cypher-API (graph_traversal + graph_pattern_match) ausgeführt.",
                "Dataset-Modell: Person, Message, Forum, Tag, Place – Scale Factor 1 (ca. 10k Personen).",
            ],
        ),
        "ANN-Benchmarks": StandardComplianceRequirements(
            standard_id="ANN-Benchmarks",
            citation="Aumueller et al., VLDB 2020, DOI: 10.14778/3415478.3415562; github.com/erikbern/ann-benchmarks",
            canonical_workloads=[
                WorkloadComplianceSpec(
                    workload_id="ann_recall_at_k",
                    canonical_name="Recall@k (SIFT1M baseline)",
                    operation_mix={"read": 100},
                    distribution="uniform_random_query_vector",
                    required_dataset_shape={"dim": 128, "k": 10, "pool_size": 10_000, "dataset": "sift1m"},
                    notes="Misst recall@k gegen Brute-Force Ground Truth; Kernel: L2 Distance.",
                ),
                WorkloadComplianceSpec(
                    workload_id="ann_range_filter",
                    canonical_name="Filtered-ANN (Metadaten-Filter)",
                    operation_mix={"read": 100},
                    distribution="uniform_random_query_vector_with_category_filter",
                    required_dataset_shape={"dim": 128, "k": 10, "categories": 10, "dataset": "sift1m"},
                    notes="ANN-Abfrage mit zusaetzlichem Metadaten-Praedikat (Filter).",
                ),
                WorkloadComplianceSpec(
                    workload_id="ann_batch_query",
                    canonical_name="Batch-Query QPS Frontier",
                    operation_mix={"read": 100},
                    distribution="uniform_random_query_batch",
                    required_dataset_shape={"dim": 128, "k": 10, "batch_size": 10, "dataset": "sift1m"},
                    notes="Misst QPS ueber Batches; Grundlage fuer QPS-vs-Recall Frontier.",
                ),
            ],
            required_metrics=["recall_at_k", "qps", "mean_latency_ms", "p99_latency_ms"],
            required_parameters=["dim", "k", "pool_size", "seed", "batch_size", "dataset"],
            measurement_protocol={
                "warm_up_seconds": 30,
                "measurement_duration_seconds": 300,
                "num_runs": 5,
            },
            implementation_notes=[
                "ann_recall_at_k berechnet Recall@k in-process via Brute-Force (L2) gegen geseedeten Vektorpool.",
                "ann_batch_query ermoeglicht QPS-vs-Recall Frontier Messung ueber Parameter-Sweep.",
                "Unterstuetzte Datasets: sift1m (d=128), gist1m (d=960), glove100 (d=100).",
                "Runtime-Mapping: TEST_ANN_DATASET steuert die effektive Vektordimension pro Workload.",
                "Der seeded RNG (ann_seed) garantiert reproduzierbare Query-Vektoren.",
            ],
        ),
        "BEIR": StandardComplianceRequirements(
            standard_id="BEIR",
            citation="Thakur et al., NeurIPS Datasets and Benchmarks 2021, arXiv:2104.08663",
            canonical_workloads=[
                WorkloadComplianceSpec(
                    workload_id="rag_retrieval_quality",
                    canonical_name="BEIR Retrieval Evaluation",
                    operation_mix={"read": 100},
                    distribution="uniform_query_set",
                    required_dataset_shape={"dataset": "msmarco", "k": 10, "query_count": 100},
                    notes="Berechnet ndcg@10, mrr und map auf Retrieval-Rankings gegen Relevanzlabels.",
                ),
            ],
            required_metrics=[
                "ndcg_at_10",
                "mrr",
                "map",
                "throughput_ops_per_sec",
                "mean_latency_ms",
                "p95_latency_ms",
                "p99_latency_ms",
            ],
            required_parameters=[
                "dataset",
                "k",
                "query_count",
                "seed",
            ],
            measurement_protocol={
                "warm_up_seconds": 30,
                "measurement_duration_seconds": 300,
                "num_runs": 5,
            },
            implementation_notes=[
                "rag_retrieval_quality erzeugt BEIR-kompatible Retriever-Metriken (ndcg@10, mrr, map).",
                "Dataset-Steuerung erfolgt ueber TEST_BEIR_DATASET (derzeit: msmarco, nfcorpus, scifact).",
                "Deterministische Query-/Relevanz-Sampling-Logik via beir_seed fuer reproduzierbare Runs.",
            ],
        ),
        "MLPerf-Inference": StandardComplianceRequirements(
            standard_id="MLPerf-Inference",
            citation="MLCommons Inference Benchmark, Closed Division Rules (aktuelle Revision)",
            canonical_workloads=[
                WorkloadComplianceSpec(
                    workload_id="llm_serving_latency",
                    canonical_name="MLPerf SingleStream (Latency)",
                    operation_mix={"inference": 100},
                    distribution="single_stream_poisson_arrivals",
                    required_dataset_shape={"scenario": "singlestream", "samples_per_query": 1},
                    notes="Latenzdominantes Single-Request-Szenario mit Quality-Target.",
                ),
                WorkloadComplianceSpec(
                    workload_id="llm_serving_throughput",
                    canonical_name="MLPerf Server/Offline (Throughput)",
                    operation_mix={"inference": 100},
                    distribution="server_qps_target_or_offline_batch",
                    required_dataset_shape={"scenario": "server", "samples_per_query": 1},
                    notes="Durchsatzszenario mit Laststeuerung ueber Ziel-QPS/Batches und Quality-Target.",
                ),
            ],
            required_metrics=[
                "throughput_ops_per_sec",
                "mean_latency_ms",
                "p95_latency_ms",
                "p99_latency_ms",
                "quality_target_passed",
            ],
            required_parameters=[
                "scenario",
                "dataset",
                "batch_size",
                "target_qps",
                "target_latency_ms",
                "seed",
                "quality_target",
            ],
            measurement_protocol={
                "warm_up_seconds": 60,
                "measurement_duration_seconds": 600,
                "num_runs": 5,
            },
            implementation_notes=[
                "CHIMERA mappt MLPerf-Szenarien derzeit auf llm_serving_latency (SingleStream) und llm_serving_throughput (Server/Offline-inspiriert).",
                "Eine gueltige MLPerf-Runklassifikation erfordert Performance-Metriken plus erfuelltes Quality-Target.",
                "Parameter fuer Ziel-QPS und Ziel-Latenz werden explizit als Compliance-Eingaben gefuehrt.",
            ],
        ),
        "RAGAS": StandardComplianceRequirements(
            standard_id="RAGAS",
            citation="Es et al., RAGAS: Automated Evaluation of Retrieval Augmented Generation, arXiv:2309.15217",
            canonical_workloads=[
                WorkloadComplianceSpec(
                    workload_id="rag_qa_e2e",
                    canonical_name="RAG End-to-End QA Evaluation",
                    operation_mix={"qa": 100},
                    distribution="uniform_query_set",
                    required_dataset_shape={"query_count": 100, "top_k_contexts": 5},
                    notes="Bewertet End-to-End RAG-Antworten ueber Groundedness-, Relevance- und Context-Metriken.",
                ),
            ],
            required_metrics=[
                "faithfulness",
                "answer_relevance",
                "context_precision",
                "context_recall",
                "ragas_score",
                "ragbench_score",
                "throughput_ops_per_sec",
                "mean_latency_ms",
                "p95_latency_ms",
                "p99_latency_ms",
            ],
            required_parameters=[
                "query_count",
                "top_k_contexts",
                "eval_samples",
                "seed",
            ],
            measurement_protocol={
                "warm_up_seconds": 30,
                "measurement_duration_seconds": 300,
                "num_runs": 5,
            },
            implementation_notes=[
                "rag_qa_e2e liefert robuste RAGAS-Kernmetriken (faithfulness, answer_relevance, context_precision, context_recall).",
                "Zusatzmetrik ragbench_score dient als kompakter Robustheitsindikator fuer Antwort- und Kontextnutzung.",
                "Metrikpfad ist seed-basiert deterministisch, damit Regressionstests reproduzierbar bleiben.",
            ],
        ),
    }


def get_standard_coverage_snapshot() -> Dict[str, Dict[str, Any]]:
    """Liefert einen aktuellen Coverage-Snapshot über Standards zu Runtime-Workloads."""
    requirements = get_standard_compliance_requirements()
    current = set(CURRENT_CHIMERA_WORKLOADS)

    snapshot: Dict[str, Dict[str, Any]] = {}
    for standard_id, spec in requirements.items():
        required = [w.workload_id for w in spec.canonical_workloads]
        implemented = [wid for wid in required if wid in current]
        missing = [wid for wid in required if wid not in current]
        snapshot[standard_id] = {
            "required_workloads": required,
            "implemented_workloads": implemented,
            "missing_workloads": missing,
            "is_fully_implemented": len(missing) == 0,
        }

    return snapshot


def get_profile_catalog() -> Dict[str, BenchmarkProfile]:
    """Liefert vorkonfigurierte CHIMERA-Profile fuer Spezialdomaenen."""
    return {
        "core": BenchmarkProfile(
            profile_id="core",
            description="Basisprofil (OLTP + Vector + Graph)",
            standards=["YCSB", "ANN-Benchmarks", "LDBC-SNB"],
            required_workloads=["ycsb_a", "ycsb_b", "ycsb_c", "ycsb_f", "vector_search", "graph_traversal"],
            optional_workloads=[],
        ),
        "timeseries": BenchmarkProfile(
            profile_id="timeseries",
            description="Timeseries-Ingestion und Query-Profile",
            standards=["TSBS"],
            required_workloads=["ts_ingest_raw", "ts_ingest_batch", "ts_range_query", "ts_lastpoint", "ts_downsample"],
            optional_workloads=["ts_high_cpu", "ts_groupby"],
        ),
        "geo": BenchmarkProfile(
            profile_id="geo",
            description="Geospatial Index- und Query-Profile",
            standards=["LDBC-SNB"],
            required_workloads=["geo_intersects", "geo_contains", "geo_range_query"],
            optional_workloads=["geo_spatial_join"],
        ),
        "ldbc": BenchmarkProfile(
            profile_id="ldbc",
            description="LDBC Social Network Benchmark – Interactive Complex und Short Queries",
            standards=["LDBC-SNB"],
            required_workloads=[
                "ldbc_ic1_friends_of_person",
                "ldbc_ic2_recent_messages",
                "ldbc_ic6_tag_cocreators",
                "ldbc_ic14_shortest_path",
                "ldbc_is1_person_profile",
                "ldbc_lu1_add_person",
            ],
            optional_workloads=[
                "ldbc_ic3_friends_abroad",
                "ldbc_ic9_recent_forum_posts",
            ],
        ),
        "process": BenchmarkProfile(
            profile_id="process",
            description="Process-Mining und Conformance-Profile",
            standards=["TPC-H"],
            required_workloads=["process_discovery_alpha", "process_discovery_heuristic", "process_conformance"],
            optional_workloads=["process_variant_clustering"],
        ),
        "llm": BenchmarkProfile(
            profile_id="llm",
            description="LLM-Serving Performance-Profile",
            standards=["MLPerf-Inference"],
            required_workloads=["llm_serving_latency", "llm_serving_throughput"],
            optional_workloads=["llm_batch_serving", "llm_cache_hit"],
        ),
        "ml": BenchmarkProfile(
            profile_id="ml",
            description="ML/Ethics Pipeline-Profile",
            standards=["MLPerf-Inference", "RAGAS"],
            required_workloads=["ml_policy_eval", "ethics_eval_pipeline"],
            optional_workloads=["ml_model_switch"],
        ),
        "lora": BenchmarkProfile(
            profile_id="lora",
            description="LoRA Adaptation und Switching-Profile",
            standards=["MLPerf-Inference"],
            required_workloads=["lora_load_apply", "lora_switch_overhead"],
            optional_workloads=["lora_train_step", "lora_multi_adapter_batch"],
        ),
        "rag": BenchmarkProfile(
            profile_id="rag",
            description="Retrieval + Reranking + Antwortqualitaet",
            standards=["BEIR", "RAGAS"],
            required_workloads=["rag_retrieval_quality", "rag_qa_e2e"],
            optional_workloads=["rag_rerank_latency"],
        ),
        "ann": BenchmarkProfile(
            profile_id="ann",
            description="ANN-Benchmarks \u2013 Approximate Nearest Neighbor Recall/QPS Frontier",
            standards=["ANN-Benchmarks"],
            required_workloads=[
                "ann_recall_at_k",
                "ann_batch_query",
            ],
            optional_workloads=["ann_range_filter"],
        ),
        "full": BenchmarkProfile(
            profile_id="full",
            description="Kombiniertes Profil fuer Verbund + AI/ML/LoRA",
            standards=["YCSB", "TPC-C", "TSBS", "ANN-Benchmarks", "LDBC-SNB", "BEIR", "MLPerf-Inference", "RAGAS"],
            required_workloads=[
                "ycsb_a", "ycsb_b", "ycsb_c", "ycsb_f", "vector_search", "graph_traversal",
                "ts_ingest_raw", "ts_range_query", "geo_intersects", "process_discovery_alpha",
                "llm_serving_latency", "ml_policy_eval", "lora_load_apply", "rag_retrieval_quality",
                "ldbc_ic1_friends_of_person", "ldbc_is1_person_profile",
                "ann_recall_at_k",
            ],
            optional_workloads=["tpc_c_mix", "geo_spatial_join", "process_conformance", "rag_qa_e2e"],
        ),
    }


def resolve_profile_workloads(profile_id: str) -> Tuple[List[str], List[str], List[str]]:
    """Gibt (required, available_now, missing_now) fuer ein Profil zurueck."""
    profiles = get_profile_catalog()
    profile = profiles.get(profile_id)
    if profile is None:
        raise KeyError(f"Unknown profile_id: {profile_id}")

    required = list(profile.required_workloads)
    available_now = [w for w in required if w in CURRENT_CHIMERA_WORKLOADS]
    missing_now = [w for w in required if w not in CURRENT_CHIMERA_WORKLOADS]
    return required, available_now, missing_now
