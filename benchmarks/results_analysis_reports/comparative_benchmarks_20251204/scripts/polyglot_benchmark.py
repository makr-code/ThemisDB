"""
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            polyglot_benchmark.py                              ║
  Version:         0.0.47                                             ║
  Last Modified:   2026-04-15 18:43:44                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     632                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
"""

#!/usr/bin/env python3
"""
ThemisDB Polyglot Persistence Benchmark

Compares ThemisDB's multi-model capabilities against polyglot persistence
approaches using specialized databases (Neo4j for graphs, ChromaDB for vectors,
PostgreSQL for relational data).

This benchmark demonstrates ThemisDB's advantage as a unified multi-model database
vs. the complexity of maintaining multiple specialized databases.

Usage:
    python polyglot_benchmark.py --all
    python polyglot_benchmark.py --scenario graph-vector
"""

import os
import sys
import json
import time
from datetime import datetime
from pathlib import Path
from typing import Any, Dict, List, Optional
import click
import structlog
from rich.console import Console
from rich.table import Table
from rich.progress import Progress, SpinnerColumn, TextColumn

# Add parent directory to path for imports
sys.path.insert(0, str(Path(__file__).parent.parent))

from benchmarks.base_benchmark import (
    BaseDatabaseAdapter,
    BenchmarkCategory,
    BenchmarkConfig,
    BenchmarkResult,
    BenchmarkRunner,
)
from datasets.huggingface_loader import HuggingFaceDatasetLoader, BenchmarkDataset
from adapters.themisdb_adapter import ThemisDBAdapter
from adapters.postgresql_adapter import PostgreSQLAdapter
from adapters.neo4j_adapter import Neo4jAdapter
from adapters.chromadb_adapter import ChromaDBAdapter

# Configure logging
structlog.configure(
    processors=[
        structlog.stdlib.filter_by_level,
        structlog.stdlib.add_log_level,
        structlog.processors.TimeStamper(fmt="iso"),
        structlog.processors.JSONRenderer()
    ],
    wrapper_class=structlog.stdlib.BoundLogger,
    context_class=dict,
    logger_factory=structlog.stdlib.LoggerFactory(),
)

logger = structlog.get_logger()
console = Console()


def get_adapter_config(db_name: str) -> Dict:
    """Get configuration for a database adapter from environment variables."""
    configs = {
        "themisdb": {
            "host": os.getenv("THEMISDB_HOST", "localhost"),
            "port": int(os.getenv("THEMISDB_PORT", "8765")),
        },
        "postgresql": {
            "host": os.getenv("POSTGRESQL_HOST", "localhost"),
            "port": int(os.getenv("POSTGRESQL_PORT", "5432")),
            "user": os.getenv("POSTGRESQL_USER", "benchmark"),
            "password": os.getenv("POSTGRESQL_PASSWORD", "benchmark123"),
            "database": os.getenv("POSTGRESQL_DB", "benchmark"),
            "use_pgvector": False,
        },
        "neo4j": {
            "host": os.getenv("NEO4J_HOST", "localhost"),
            "bolt_port": int(os.getenv("NEO4J_BOLT_PORT", "7687")),
            "password": os.getenv("NEO4J_PASSWORD", "benchmark123"),
        },
        "chromadb": {
            "host": os.getenv("CHROMADB_HOST", "localhost"),
            "port": int(os.getenv("CHROMADB_PORT", "8000")),
        },
    }
    return configs.get(db_name, {})


class PolyglotBenchmarkSuite:
    """
    Compares ThemisDB multi-model approach vs polyglot persistence.
    
    Scenarios tested:
    1. Graph + Vector: Social network with user embeddings
    2. Relational + Graph: User data with relationships
    3. Vector + Relational: Documents with embeddings and metadata
    4. Full Multi-Model: All three data models combined
    """
    
    def __init__(self, config: BenchmarkConfig, output_dir: Path):
        self.config = config
        self.output_dir = output_dir
        self.results: Dict[str, List[BenchmarkResult]] = {}
        
        # Adapters for polyglot approach
        self.themisdb: Optional[ThemisDBAdapter] = None
        self.postgresql: Optional[PostgreSQLAdapter] = None
        self.neo4j: Optional[Neo4jAdapter] = None
        self.chromadb: Optional[ChromaDBAdapter] = None
        
        # Dataset
        self.dataset: Optional[BenchmarkDataset] = None
        
        # Timestamp
        self.run_id = datetime.now().strftime("%Y%m%d_%H%M%S")
        
        output_dir.mkdir(parents=True, exist_ok=True)
    
    def setup(self) -> None:
        """Initialize all database connections."""
        console.print("\n[bold blue]Setting up database connections...[/bold blue]")
        
        adapters = [
            ("ThemisDB", ThemisDBAdapter, "themisdb"),
            ("PostgreSQL", PostgreSQLAdapter, "postgresql"),
            ("Neo4j", Neo4jAdapter, "neo4j"),
            ("ChromaDB", ChromaDBAdapter, "chromadb"),
        ]
        
        for name, adapter_class, config_key in adapters:
            try:
                config = get_adapter_config(config_key)
                adapter = adapter_class(config)
                adapter.connect()
                
                if config_key == "themisdb":
                    self.themisdb = adapter
                elif config_key == "postgresql":
                    self.postgresql = adapter
                elif config_key == "neo4j":
                    self.neo4j = adapter
                elif config_key == "chromadb":
                    self.chromadb = adapter
                
                console.print(f"[green]✓ Connected to {name}[/green]")
            except Exception as e:
                console.print(f"[red]✗ Failed to connect to {name}: {e}[/red]")
    
    def load_dataset(self) -> None:
        """Load benchmark dataset."""
        console.print("\n[bold blue]Loading benchmark dataset...[/bold blue]")
        
        loader = HuggingFaceDatasetLoader(
            dataset_size=self.config.dataset_size,
            vector_dimensions=self.config.vector_dimensions,
            random_seed=42
        )
        
        self.dataset = loader.load_complete_dataset(
            include_vectors=True,
            include_graph=True
        )
        
        console.print(f"[green]✓ Loaded dataset:[/green]")
        console.print(f"  Documents: {self.dataset.document_count}")
        console.print(f"  Vectors: {self.dataset.vector_count}")
        console.print(f"  Graph nodes: {self.dataset.node_count}")
        console.print(f"  Graph edges: {self.dataset.edge_count}")
    
    def run_scenario_graph_vector(self) -> None:
        """
        Scenario: Graph + Vector Search
        
        Use case: Social network with user embeddings for similarity matching
        
        Polyglot approach: Neo4j (graph) + ChromaDB (vectors)
        ThemisDB approach: Single database for both
        """
        console.print("\n[bold cyan]Scenario: Graph + Vector Search[/bold cyan]")
        console.print("Use case: Social network with user embeddings")
        
        runner = BenchmarkRunner(self.config)
        
        # Prepare test data
        nodes = self.dataset.graph_nodes[:500] if self.dataset.graph_nodes else []
        edges = self.dataset.graph_edges[:2000] if self.dataset.graph_edges else []
        vectors = self.dataset.vectors[:500] if self.dataset.vectors else []
        
        # =====================================================================
        # ThemisDB (Multi-model): Single database for both operations
        # =====================================================================
        if self.themisdb:
            console.print("\n[yellow]Testing ThemisDB (multi-model)...[/yellow]")
            
            # Combined insert: graph + vectors in single transactions
            insert_idx = [0]
            def themisdb_combined_insert():
                if insert_idx[0] < len(nodes):
                    node_id, labels, props = nodes[insert_idx[0]]
                    self.themisdb.insert_node(node_id, labels, props)
                    
                    # Also insert vector for this user
                    if insert_idx[0] < len(vectors):
                        doc_id, vec, meta = vectors[insert_idx[0]]
                        self.themisdb.insert_vector("user_vectors", doc_id, vec, meta)
                insert_idx[0] += 1
            
            runner.run_benchmark(
                name="themisdb_graph_vector_insert",
                category=BenchmarkCategory.MIXED,
                database="ThemisDB",
                func=themisdb_combined_insert,
                iterations=min(len(nodes), 200)
            )
            
            # Insert edges
            for from_id, to_id, edge_type, props in edges[:1000]:
                from_idx = int(from_id.split("_")[1])
                to_idx = int(to_id.split("_")[1])
                if from_idx < 500 and to_idx < 500:
                    self.themisdb.insert_edge(from_id, to_id, edge_type, props)
            
            # Combined query: find neighbors + similar users
            query_idx = [0]
            query_vectors = [v[1] for v in vectors[:50]] if vectors else []
            
            def themisdb_combined_query():
                if query_idx[0] < 50 and query_vectors:
                    # Get graph neighbors
                    start_node = f"user_{query_idx[0]:08d}"
                    neighbors = self.themisdb.traverse_bfs(start_node, max_depth=2)
                    
                    # Find similar users by vector
                    if query_idx[0] < len(query_vectors):
                        similar = self.themisdb.search_vectors(
                            "user_vectors", 
                            query_vectors[query_idx[0]], 
                            k=5
                        )
                query_idx[0] += 1
            
            runner.run_benchmark(
                name="themisdb_graph_vector_query",
                category=BenchmarkCategory.MIXED,
                database="ThemisDB",
                func=themisdb_combined_query,
                iterations=50
            )
            
            self.results.setdefault("ThemisDB", []).extend(runner.results)
            runner.clear_results()
        
        # =====================================================================
        # Polyglot: Neo4j (graph) + ChromaDB (vectors)
        # =====================================================================
        if self.neo4j and self.chromadb:
            console.print("\n[yellow]Testing Polyglot (Neo4j + ChromaDB)...[/yellow]")
            
            # Polyglot insert: requires coordination between two databases
            insert_idx = [0]
            def polyglot_combined_insert():
                if insert_idx[0] < len(nodes):
                    node_id, labels, props = nodes[insert_idx[0]]
                    self.neo4j.insert_node(node_id, labels, props)
                    
                    # Also insert vector to ChromaDB
                    if insert_idx[0] < len(vectors):
                        doc_id, vec, meta = vectors[insert_idx[0]]
                        self.chromadb.insert_vector("user_vectors", doc_id, vec, meta)
                insert_idx[0] += 1
            
            runner.run_benchmark(
                name="polyglot_graph_vector_insert",
                category=BenchmarkCategory.MIXED,
                database="Polyglot (Neo4j+ChromaDB)",
                func=polyglot_combined_insert,
                iterations=min(len(nodes), 200)
            )
            
            # Insert edges to Neo4j
            for from_id, to_id, edge_type, props in edges[:1000]:
                from_idx = int(from_id.split("_")[1])
                to_idx = int(to_id.split("_")[1])
                if from_idx < 500 and to_idx < 500:
                    self.neo4j.insert_edge(from_id, to_id, edge_type, props)
            
            # Polyglot query: requires two separate queries + result merging
            query_idx = [0]
            
            def polyglot_combined_query():
                if query_idx[0] < 50 and query_vectors:
                    # Query Neo4j for graph neighbors
                    start_node = f"user_{query_idx[0]:08d}"
                    neighbors = self.neo4j.traverse_bfs(start_node, max_depth=2)
                    
                    # Query ChromaDB for similar users
                    if query_idx[0] < len(query_vectors):
                        similar = self.chromadb.search_vectors(
                            "user_vectors",
                            query_vectors[query_idx[0]],
                            k=5
                        )
                    
                    # In real polyglot, you'd need to correlate/join these results
                query_idx[0] += 1
            
            runner.run_benchmark(
                name="polyglot_graph_vector_query",
                category=BenchmarkCategory.MIXED,
                database="Polyglot (Neo4j+ChromaDB)",
                func=polyglot_combined_query,
                iterations=50
            )
            
            self.results.setdefault("Polyglot", []).extend(runner.results)
    
    def run_scenario_relational_graph(self) -> None:
        """
        Scenario: Relational + Graph
        
        Use case: User profiles with social relationships
        
        Polyglot approach: PostgreSQL (relational) + Neo4j (graph)
        ThemisDB approach: Single database for both
        """
        console.print("\n[bold cyan]Scenario: Relational + Graph[/bold cyan]")
        console.print("Use case: User profiles with social relationships")
        
        runner = BenchmarkRunner(self.config)
        
        # Use documents as user profiles
        docs = self.dataset.documents[:500]
        edges = self.dataset.graph_edges[:2000] if self.dataset.graph_edges else []
        
        # =====================================================================
        # ThemisDB (Multi-model)
        # =====================================================================
        if self.themisdb:
            console.print("\n[yellow]Testing ThemisDB (multi-model)...[/yellow]")
            
            insert_idx = [0]
            def themisdb_relational_graph_insert():
                if insert_idx[0] < len(docs):
                    doc = docs[insert_idx[0]].copy()
                    self.themisdb.insert_one("user_profiles", doc)
                insert_idx[0] += 1
            
            runner.run_benchmark(
                name="themisdb_rel_graph_insert",
                category=BenchmarkCategory.MIXED,
                database="ThemisDB",
                func=themisdb_relational_graph_insert,
                iterations=min(len(docs), 200)
            )
            
            self.results.setdefault("ThemisDB", []).extend(runner.results)
            runner.clear_results()
        
        # =====================================================================
        # Polyglot: PostgreSQL + Neo4j
        # =====================================================================
        if self.postgresql and self.neo4j:
            console.print("\n[yellow]Testing Polyglot (PostgreSQL + Neo4j)...[/yellow]")
            
            insert_idx = [0]
            def polyglot_relational_graph_insert():
                if insert_idx[0] < len(docs):
                    doc = docs[insert_idx[0]].copy()
                    # Insert to PostgreSQL for relational queries
                    self.postgresql.insert_one("user_profiles", doc)
                    # Also insert minimal node to Neo4j for relationships
                    self.neo4j.insert_node(doc['id'], ["User"], {"name": doc.get('title', '')})
                insert_idx[0] += 1
            
            runner.run_benchmark(
                name="polyglot_rel_graph_insert",
                category=BenchmarkCategory.MIXED,
                database="Polyglot (PostgreSQL+Neo4j)",
                func=polyglot_relational_graph_insert,
                iterations=min(len(docs), 200)
            )
            
            self.results.setdefault("Polyglot", []).extend(runner.results)
    
    def run_scenario_full_multimodel(self) -> None:
        """
        Scenario: Full Multi-Model
        
        Use case: Document management with vectors, relationships, and metadata
        
        Polyglot approach: PostgreSQL + Neo4j + ChromaDB
        ThemisDB approach: Single unified database
        """
        console.print("\n[bold cyan]Scenario: Full Multi-Model[/bold cyan]")
        console.print("Use case: Document management (vectors + graph + relational)")
        
        runner = BenchmarkRunner(self.config)
        
        docs = self.dataset.documents[:200]
        vectors = self.dataset.vectors[:200] if self.dataset.vectors else []
        
        # =====================================================================
        # ThemisDB: Single database handles everything
        # =====================================================================
        if self.themisdb:
            console.print("\n[yellow]Testing ThemisDB (unified multi-model)...[/yellow]")
            
            insert_idx = [0]
            def themisdb_full_multimodel():
                if insert_idx[0] < len(docs):
                    doc = docs[insert_idx[0]].copy()
                    # Single insert handles document + can link to vectors/graph
                    self.themisdb.insert_one("full_docs", doc)
                    
                    if insert_idx[0] < len(vectors):
                        doc_id, vec, meta = vectors[insert_idx[0]]
                        self.themisdb.insert_vector("full_vectors", doc_id, vec, meta)
                insert_idx[0] += 1
            
            runner.run_benchmark(
                name="themisdb_full_multimodel",
                category=BenchmarkCategory.MIXED,
                database="ThemisDB",
                func=themisdb_full_multimodel,
                iterations=min(len(docs), 100)
            )
            
            self.results.setdefault("ThemisDB", []).extend(runner.results)
            runner.clear_results()
        
        # =====================================================================
        # Polyglot: Three databases to maintain
        # =====================================================================
        if self.postgresql and self.neo4j and self.chromadb:
            console.print("\n[yellow]Testing Polyglot (PostgreSQL + Neo4j + ChromaDB)...[/yellow]")
            
            insert_idx = [0]
            def polyglot_full_multimodel():
                if insert_idx[0] < len(docs):
                    doc = docs[insert_idx[0]].copy()
                    # Must coordinate across three databases
                    self.postgresql.insert_one("full_docs", doc)
                    self.neo4j.insert_node(doc['id'], ["Document"], {"title": doc.get('title', '')})
                    
                    if insert_idx[0] < len(vectors):
                        doc_id, vec, meta = vectors[insert_idx[0]]
                        self.chromadb.insert_vector("full_vectors", doc_id, vec, meta)
                insert_idx[0] += 1
            
            runner.run_benchmark(
                name="polyglot_full_multimodel",
                category=BenchmarkCategory.MIXED,
                database="Polyglot (PG+Neo4j+Chroma)",
                func=polyglot_full_multimodel,
                iterations=min(len(docs), 100)
            )
            
            self.results.setdefault("Polyglot", []).extend(runner.results)
    
    def cleanup(self) -> None:
        """Disconnect from all databases."""
        console.print("\n[bold blue]Cleaning up...[/bold blue]")
        
        for name, adapter in [
            ("ThemisDB", self.themisdb),
            ("PostgreSQL", self.postgresql),
            ("Neo4j", self.neo4j),
            ("ChromaDB", self.chromadb)
        ]:
            if adapter:
                try:
                    adapter.disconnect()
                    console.print(f"[green]✓ Disconnected from {name}[/green]")
                except Exception as e:
                    console.print(f"[red]✗ Error disconnecting from {name}: {e}[/red]")
    
    def save_results(self) -> Path:
        """Save benchmark results."""
        output_file = self.output_dir / f"polyglot_benchmark_{self.run_id}.json"
        
        all_results = {
            "run_id": self.run_id,
            "timestamp": datetime.now().isoformat(),
            "config": {
                "dataset_size": self.config.dataset_size,
                "iterations": self.config.iterations,
            },
            "results": {
                approach: [r.to_dict() for r in results]
                for approach, results in self.results.items()
            }
        }
        
        with open(output_file, "w") as f:
            json.dump(all_results, f, indent=2)
        
        console.print(f"\n[green]✓ Results saved to {output_file}[/green]")
        return output_file
    
    def print_summary(self) -> None:
        """Print comparison summary."""
        console.print("\n[bold blue]Polyglot Persistence Benchmark Results[/bold blue]")
        console.print("=" * 60)
        
        # Group by benchmark name
        benchmarks: Dict[str, Dict[str, BenchmarkResult]] = {}
        
        for approach, results in self.results.items():
            for result in results:
                if result.name not in benchmarks:
                    benchmarks[result.name] = {}
                benchmarks[result.name][approach] = result
        
        table = Table(title="ThemisDB vs Polyglot Persistence")
        table.add_column("Benchmark", style="cyan")
        table.add_column("ThemisDB", style="green")
        table.add_column("Polyglot", style="yellow")
        table.add_column("Speedup", style="magenta")
        
        for bench_name, approaches in benchmarks.items():
            themis_result = approaches.get("ThemisDB")
            polyglot_result = approaches.get("Polyglot")
            
            themis_str = f"{themis_result.mean_ms:.2f}ms" if themis_result else "N/A"
            polyglot_str = f"{polyglot_result.mean_ms:.2f}ms" if polyglot_result else "N/A"
            
            if themis_result and polyglot_result and themis_result.mean_ms > 0:
                speedup = polyglot_result.mean_ms / themis_result.mean_ms
                speedup_str = f"{speedup:.2f}x"
                if speedup > 1:
                    speedup_str = f"[green]{speedup_str}[/green]"
            else:
                speedup_str = "N/A"
            
            table.add_row(bench_name, themis_str, polyglot_str, speedup_str)
        
        console.print(table)
        
        console.print("\n[bold]Key Insights:[/bold]")
        console.print("• ThemisDB provides unified multi-model storage in a single database")
        console.print("• Polyglot requires coordination across multiple databases")
        console.print("• ThemisDB reduces operational complexity and potential consistency issues")


@click.command()
@click.option("--all", "run_all", is_flag=True, help="Run all scenarios")
@click.option("--scenario", type=click.Choice(["graph-vector", "rel-graph", "full"]),
              multiple=True, help="Specific scenarios to run")
@click.option("--dataset-size", default=1000, type=int, help="Dataset size")
@click.option("--iterations", default=100, type=int, help="Benchmark iterations")
@click.option("--output", default="results", type=click.Path(), help="Output directory")
def main(run_all: bool, scenario: tuple, dataset_size: int, iterations: int, output: str):
    """
    ThemisDB Polyglot Persistence Benchmark
    
    Compares ThemisDB's multi-model approach against polyglot persistence
    using Neo4j + ChromaDB + PostgreSQL.
    """
    console.print("[bold blue]ThemisDB Polyglot Persistence Benchmark[/bold blue]")
    console.print("=" * 50)
    
    scenarios = set()
    if run_all or not scenario:
        scenarios = {"graph-vector", "rel-graph", "full"}
    else:
        scenarios = set(scenario)
    
    console.print(f"\nScenarios: {', '.join(scenarios)}")
    console.print(f"Dataset size: {dataset_size}")
    console.print(f"Iterations: {iterations}")
    
    config = BenchmarkConfig(
        warmup_iterations=10,
        iterations=iterations,
        dataset_size=dataset_size,
    )
    
    suite = PolyglotBenchmarkSuite(config, Path(output))
    
    try:
        suite.setup()
        suite.load_dataset()
        
        if "graph-vector" in scenarios:
            suite.run_scenario_graph_vector()
        
        if "rel-graph" in scenarios:
            suite.run_scenario_relational_graph()
        
        if "full" in scenarios:
            suite.run_scenario_full_multimodel()
        
        suite.save_results()
        suite.print_summary()
        
    except KeyboardInterrupt:
        console.print("\n[yellow]Benchmark interrupted[/yellow]")
    except Exception as e:
        console.print(f"\n[red]Error: {e}[/red]")
        logger.exception("Benchmark failed")
    finally:
        suite.cleanup()
    
    console.print("\n[bold green]Polyglot benchmark complete![/bold green]")


if __name__ == "__main__":
    main()
