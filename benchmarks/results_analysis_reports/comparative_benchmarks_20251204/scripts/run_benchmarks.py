"""
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            run_benchmarks.py                                  ║
  Version:         0.0.47                                             ║
  Last Modified:   2026-04-15 18:43:44                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     620                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
"""

#!/usr/bin/env python3
"""
ThemisDB Comparative Benchmark Runner

Main script for executing comparative benchmarks across multiple database systems.
Uses standardized Hugging Face datasets for fair comparison.

Usage:
    python run_benchmarks.py --all
    python run_benchmarks.py --category crud --databases themisdb,postgresql
    python run_benchmarks.py --dataset-size 100000 --iterations 10000
"""

import os
import sys
import json
import time
from datetime import datetime
from pathlib import Path
from typing import Dict, List, Optional, Type
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
        structlog.stdlib.PositionalArgumentsFormatter(),
        structlog.processors.TimeStamper(fmt="iso"),
        structlog.processors.JSONRenderer()
    ],
    wrapper_class=structlog.stdlib.BoundLogger,
    context_class=dict,
    logger_factory=structlog.stdlib.LoggerFactory(),
    cache_logger_on_first_use=True,
)

logger = structlog.get_logger()
console = Console()


# Database adapter registry
ADAPTERS: Dict[str, Type[BaseDatabaseAdapter]] = {
    "themisdb": ThemisDBAdapter,
    "postgresql": PostgreSQLAdapter,
    "postgresql-pgvector": PostgreSQLAdapter,  # With pgvector config
    "neo4j": Neo4jAdapter,
    "chromadb": ChromaDBAdapter,
    # Additional adapters can be registered here:
    # "mongodb": MongoDBAdapter,
    # "redis": RedisAdapter,
    # "arangodb": ArangoDBAdapter,
    # "milvus": MilvusAdapter,
    # "elasticsearch": ElasticsearchAdapter,
}


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
        "postgresql-pgvector": {
            "host": os.getenv("POSTGRESQL_PGVECTOR_HOST", "localhost"),
            "port": int(os.getenv("POSTGRESQL_PGVECTOR_PORT", "5433")),
            "user": os.getenv("POSTGRESQL_USER", "benchmark"),
            "password": os.getenv("POSTGRESQL_PASSWORD", "benchmark123"),
            "database": os.getenv("POSTGRESQL_DB", "benchmark"),
            "use_pgvector": True,
        },
        "mongodb": {
            "host": os.getenv("MONGODB_HOST", "localhost"),
            "port": int(os.getenv("MONGODB_PORT", "27017")),
            "user": os.getenv("MONGODB_USER", "benchmark"),
            "password": os.getenv("MONGODB_PASSWORD", "benchmark123"),
        },
        "redis": {
            "host": os.getenv("REDIS_HOST", "localhost"),
            "port": int(os.getenv("REDIS_PORT", "6379")),
        },
        "arangodb": {
            "host": os.getenv("ARANGODB_HOST", "localhost"),
            "port": int(os.getenv("ARANGODB_PORT", "8529")),
            "password": os.getenv("ARANGODB_PASSWORD", "benchmark123"),
        },
        "neo4j": {
            "host": os.getenv("NEO4J_HOST", "localhost"),
            "bolt_port": int(os.getenv("NEO4J_BOLT_PORT", "7687")),
            "password": os.getenv("NEO4J_PASSWORD", "benchmark123"),
        },
        "milvus": {
            "host": os.getenv("MILVUS_HOST", "localhost"),
            "port": int(os.getenv("MILVUS_PORT", "19530")),
        },
        "elasticsearch": {
            "host": os.getenv("ELASTICSEARCH_HOST", "localhost"),
            "port": int(os.getenv("ELASTICSEARCH_PORT", "9200")),
        },
        "chromadb": {
            "host": os.getenv("CHROMADB_HOST", "localhost"),
            "port": int(os.getenv("CHROMADB_PORT", "8000")),
        },
    }
    return configs.get(db_name, {})


class ComparativeBenchmarkSuite:
    """
    Orchestrates comparative benchmarks across multiple databases.
    """
    
    def __init__(self, 
                 config: BenchmarkConfig,
                 databases: List[str],
                 output_dir: Path):
        self.config = config
        self.databases = databases
        self.output_dir = output_dir
        self.adapters: Dict[str, BaseDatabaseAdapter] = {}
        self.dataset: Optional[BenchmarkDataset] = None
        self.results: Dict[str, List[BenchmarkResult]] = {}
        
        # Create output directory
        self.output_dir.mkdir(parents=True, exist_ok=True)
        
        # Timestamp for this run
        self.run_id = datetime.now().strftime("%Y%m%d_%H%M%S")
    
    def setup_databases(self) -> None:
        """Initialize connections to all specified databases."""
        console.print("\n[bold blue]Setting up database connections...[/bold blue]")
        
        with Progress(
            SpinnerColumn(),
            TextColumn("[progress.description]{task.description}"),
            console=console,
        ) as progress:
            for db_name in self.databases:
                task = progress.add_task(f"Connecting to {db_name}...", total=None)
                
                if db_name not in ADAPTERS:
                    console.print(f"[yellow]Warning: No adapter for {db_name}, skipping[/yellow]")
                    continue
                
                try:
                    adapter_class = ADAPTERS[db_name]
                    config = get_adapter_config(db_name)
                    adapter = adapter_class(config)
                    adapter.connect()
                    self.adapters[db_name] = adapter
                    progress.update(task, description=f"[green]✓ Connected to {db_name}[/green]")
                except Exception as e:
                    progress.update(task, description=f"[red]✗ Failed to connect to {db_name}: {e}[/red]")
                    logger.error(f"Failed to connect to {db_name}", error=str(e))
    
    def load_dataset(self) -> None:
        """Load benchmark dataset from Hugging Face."""
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
    
    def run_crud_benchmarks(self) -> None:
        """Run CRUD operation benchmarks."""
        console.print("\n[bold blue]Running CRUD benchmarks...[/bold blue]")
        
        if not self.dataset:
            raise RuntimeError("Dataset not loaded")
        
        for db_name, adapter in self.adapters.items():
            console.print(f"\n[cyan]Testing {db_name}...[/cyan]")
            runner = BenchmarkRunner(self.config)
            
            # Prepare test data
            test_docs = self.dataset.documents[:1000]  # Use subset for benchmarks
            
            # INSERT benchmark
            insert_idx = [0]
            def insert_op():
                doc = test_docs[insert_idx[0] % len(test_docs)].copy()
                doc["id"] = f"bench_{insert_idx[0]}"
                adapter.insert_one("benchmark_docs", doc)
                insert_idx[0] += 1
            
            runner.run_benchmark(
                name="insert_single",
                category=BenchmarkCategory.CRUD,
                database=db_name,
                func=insert_op,
                iterations=min(self.config.iterations, 1000)
            )
            
            # READ benchmark
            read_idx = [0]
            def read_op():
                doc_id = f"bench_{read_idx[0] % insert_idx[0]}" if insert_idx[0] > 0 else "bench_0"
                adapter.find_by_id("benchmark_docs", doc_id)
                read_idx[0] += 1
            
            runner.run_benchmark(
                name="read_single",
                category=BenchmarkCategory.CRUD,
                database=db_name,
                func=read_op,
                iterations=self.config.iterations
            )
            
            # UPDATE benchmark
            update_idx = [0]
            def update_op():
                doc_id = f"bench_{update_idx[0] % insert_idx[0]}" if insert_idx[0] > 0 else "bench_0"
                adapter.update_one("benchmark_docs", doc_id, {"updated": True})
                update_idx[0] += 1
            
            runner.run_benchmark(
                name="update_single",
                category=BenchmarkCategory.CRUD,
                database=db_name,
                func=update_op,
                iterations=min(self.config.iterations, 1000)
            )
            
            # DELETE benchmark
            delete_idx = [0]
            def delete_op():
                if delete_idx[0] < insert_idx[0]:
                    doc_id = f"bench_{delete_idx[0]}"
                    adapter.delete_one("benchmark_docs", doc_id)
                    delete_idx[0] += 1
            
            runner.run_benchmark(
                name="delete_single",
                category=BenchmarkCategory.CRUD,
                database=db_name,
                func=delete_op,
                iterations=min(self.config.iterations, insert_idx[0]) if insert_idx[0] > 0 else 100
            )
            
            self.results.setdefault(db_name, []).extend(runner.results)
    
    def run_query_benchmarks(self) -> None:
        """Run query operation benchmarks."""
        console.print("\n[bold blue]Running query benchmarks...[/bold blue]")
        
        if not self.dataset:
            raise RuntimeError("Dataset not loaded")
        
        for db_name, adapter in self.adapters.items():
            console.print(f"\n[cyan]Testing {db_name}...[/cyan]")
            runner = BenchmarkRunner(self.config)
            
            # First, insert test data
            categories = list(set(doc["category"] for doc in self.dataset.documents[:1000]))
            for i, doc in enumerate(self.dataset.documents[:1000]):
                doc_copy = doc.copy()
                doc_copy["id"] = f"query_{i}"
                adapter.insert_one("query_docs", doc_copy)
            
            # Create index if supported
            if hasattr(adapter, 'create_index'):
                adapter.create_index("query_docs", "category")
            
            # EQUALITY query benchmark
            query_idx = [0]
            def equality_query():
                cat = categories[query_idx[0] % len(categories)]
                adapter.find_by_field("query_docs", "category", cat)
                query_idx[0] += 1
            
            runner.run_benchmark(
                name="query_equality",
                category=BenchmarkCategory.QUERY,
                database=db_name,
                func=equality_query,
                iterations=self.config.iterations
            )
            
            # COUNT query benchmark
            count_idx = [0]
            def count_query():
                cat = categories[count_idx[0] % len(categories)]
                adapter.count_by_field("query_docs", "category", cat)
                count_idx[0] += 1
            
            runner.run_benchmark(
                name="query_count",
                category=BenchmarkCategory.QUERY,
                database=db_name,
                func=count_query,
                iterations=self.config.iterations
            )
            
            self.results.setdefault(db_name, []).extend(runner.results)
    
    def run_vector_benchmarks(self) -> None:
        """Run vector search benchmarks."""
        console.print("\n[bold blue]Running vector search benchmarks...[/bold blue]")
        
        if not self.dataset or not self.dataset.vectors:
            console.print("[yellow]No vector data available, skipping[/yellow]")
            return
        
        for db_name, adapter in self.adapters.items():
            if not adapter.supports_vector_search():
                console.print(f"[yellow]{db_name} does not support vector search, skipping[/yellow]")
                continue
            
            console.print(f"\n[cyan]Testing {db_name}...[/cyan]")
            runner = BenchmarkRunner(self.config)
            
            # Insert vectors
            for doc_id, vector, metadata in self.dataset.vectors[:1000]:
                adapter.insert_vector("vectors", doc_id, vector, metadata)
            
            # Generate query vectors
            import numpy as np
            query_vectors = []
            for _ in range(100):
                vec = np.random.randn(self.config.vector_dimensions).astype(np.float32)
                normalized_vec = vec / np.linalg.norm(vec)
                query_vectors.append(normalized_vec.tolist())
            
            # k-NN search benchmark
            search_idx = [0]
            def knn_search():
                qv = query_vectors[search_idx[0] % len(query_vectors)]
                adapter.search_vectors("vectors", qv, k=self.config.k_nearest)
                search_idx[0] += 1
            
            runner.run_benchmark(
                name=f"vector_knn_k{self.config.k_nearest}",
                category=BenchmarkCategory.VECTOR,
                database=db_name,
                func=knn_search,
                iterations=min(self.config.iterations, 500)
            )
            
            self.results.setdefault(db_name, []).extend(runner.results)
    
    def run_graph_benchmarks(self) -> None:
        """Run graph operation benchmarks."""
        console.print("\n[bold blue]Running graph benchmarks...[/bold blue]")
        
        if not self.dataset or not self.dataset.graph_nodes:
            console.print("[yellow]No graph data available, skipping[/yellow]")
            return
        
        for db_name, adapter in self.adapters.items():
            if not adapter.supports_graph_operations():
                console.print(f"[yellow]{db_name} does not support graph operations, skipping[/yellow]")
                continue
            
            console.print(f"\n[cyan]Testing {db_name}...[/cyan]")
            runner = BenchmarkRunner(self.config)
            
            # Insert nodes and edges
            for node_id, labels, props in self.dataset.graph_nodes[:1000]:
                adapter.insert_node(node_id, labels, props)
            
            for from_id, to_id, edge_type, props in self.dataset.graph_edges[:5000]:
                # Only insert if both nodes exist
                from_idx = int(from_id.split("_")[1])
                to_idx = int(to_id.split("_")[1])
                if from_idx < 1000 and to_idx < 1000:
                    adapter.insert_edge(from_id, to_id, edge_type, props)
            
            # BFS traversal benchmark
            start_nodes = [f"user_{i:08d}" for i in range(100)]
            traverse_idx = [0]
            
            def bfs_traverse():
                start = start_nodes[traverse_idx[0] % len(start_nodes)]
                adapter.traverse_bfs(start, max_depth=self.config.graph_depth)
                traverse_idx[0] += 1
            
            runner.run_benchmark(
                name=f"graph_bfs_depth{self.config.graph_depth}",
                category=BenchmarkCategory.GRAPH,
                database=db_name,
                func=bfs_traverse,
                iterations=min(self.config.iterations, 200)
            )
            
            self.results.setdefault(db_name, []).extend(runner.results)
    
    def cleanup(self) -> None:
        """Disconnect from all databases."""
        console.print("\n[bold blue]Cleaning up...[/bold blue]")
        
        for db_name, adapter in self.adapters.items():
            try:
                adapter.disconnect()
                console.print(f"[green]✓ Disconnected from {db_name}[/green]")
            except Exception as e:
                console.print(f"[red]✗ Error disconnecting from {db_name}: {e}[/red]")
    
    def save_results(self) -> Path:
        """Save benchmark results to JSON file."""
        output_file = self.output_dir / f"benchmark_results_{self.run_id}.json"
        
        all_results = {
            "run_id": self.run_id,
            "timestamp": datetime.now().isoformat(),
            "config": {
                "dataset_size": self.config.dataset_size,
                "iterations": self.config.iterations,
                "warmup_iterations": self.config.warmup_iterations,
                "vector_dimensions": self.config.vector_dimensions,
                "k_nearest": self.config.k_nearest,
                "graph_depth": self.config.graph_depth,
            },
            "databases": list(self.results.keys()),
            "results": {
                db: [r.to_dict() for r in results]
                for db, results in self.results.items()
            }
        }
        
        with open(output_file, "w") as f:
            json.dump(all_results, f, indent=2)
        
        console.print(f"\n[green]✓ Results saved to {output_file}[/green]")
        return output_file
    
    def print_summary(self) -> None:
        """Print a summary table of results."""
        console.print("\n[bold blue]Benchmark Results Summary[/bold blue]")
        
        # Group results by benchmark name
        benchmarks: Dict[str, Dict[str, BenchmarkResult]] = {}
        
        for db_name, results in self.results.items():
            for result in results:
                if result.name not in benchmarks:
                    benchmarks[result.name] = {}
                benchmarks[result.name][db_name] = result
        
        # Create summary table
        table = Table(title="Performance Comparison (lower is better)")
        table.add_column("Benchmark", style="cyan")
        
        for db_name in self.results.keys():
            table.add_column(db_name, style="green")
        
        for bench_name, db_results in benchmarks.items():
            row = [bench_name]
            for db_name in self.results.keys():
                if db_name in db_results:
                    result = db_results[db_name]
                    row.append(f"{result.mean_ms:.3f}ms\n({result.ops_per_second:.0f} ops/s)")
                else:
                    row.append("N/A")
            table.add_row(*row)
        
        console.print(table)


@click.command()
@click.option("--all", "run_all", is_flag=True, help="Run all benchmark categories")
@click.option("--category", type=click.Choice(["crud", "query", "vector", "graph", "fulltext"]), 
              multiple=True, help="Specific categories to run")
@click.option("--databases", default="themisdb", 
              help="Comma-separated list of databases to test")
@click.option("--dataset-size", default=10000, type=int,
              help="Number of documents in test dataset")
@click.option("--iterations", default=1000, type=int,
              help="Number of iterations per benchmark")
@click.option("--warmup", default=100, type=int,
              help="Number of warmup iterations")
@click.option("--output", default="results", type=click.Path(),
              help="Output directory for results")
@click.option("--vector-dim", default=384, type=int,
              help="Vector dimensions for embeddings")
@click.option("--k-nearest", default=10, type=int,
              help="K value for k-NN searches")
@click.option("--graph-depth", default=3, type=int,
              help="Maximum depth for graph traversals")
def main(run_all: bool, category: tuple, databases: str, dataset_size: int,
         iterations: int, warmup: int, output: str, vector_dim: int,
         k_nearest: int, graph_depth: int):
    """
    ThemisDB Comparative Benchmark Runner
    
    Runs standardized benchmarks against multiple database systems
    using Hugging Face datasets for fair comparison.
    """
    console.print("[bold blue]ThemisDB Comparative Benchmark Suite[/bold blue]")
    console.print("=" * 50)
    
    # Parse databases
    db_list = [db.strip().lower() for db in databases.split(",")]
    
    # Determine categories to run
    categories_to_run = set()
    if run_all or not category:
        categories_to_run = {"crud", "query", "vector", "graph"}
    else:
        categories_to_run = set(category)
    
    console.print(f"\nDatabases: {', '.join(db_list)}")
    console.print(f"Categories: {', '.join(categories_to_run)}")
    console.print(f"Dataset size: {dataset_size}")
    console.print(f"Iterations: {iterations}")
    
    # Create configuration
    config = BenchmarkConfig(
        warmup_iterations=warmup,
        iterations=iterations,
        dataset_size=dataset_size,
        vector_dimensions=vector_dim,
        k_nearest=k_nearest,
        graph_depth=graph_depth,
    )
    
    # Create and run benchmark suite
    output_dir = Path(output)
    suite = ComparativeBenchmarkSuite(config, db_list, output_dir)
    
    try:
        # Setup
        suite.setup_databases()
        suite.load_dataset()
        
        # Run benchmarks
        if "crud" in categories_to_run:
            suite.run_crud_benchmarks()
        
        if "query" in categories_to_run:
            suite.run_query_benchmarks()
        
        if "vector" in categories_to_run:
            suite.run_vector_benchmarks()
        
        if "graph" in categories_to_run:
            suite.run_graph_benchmarks()
        
        # Save and display results
        suite.save_results()
        suite.print_summary()
        
    except KeyboardInterrupt:
        console.print("\n[yellow]Benchmark interrupted by user[/yellow]")
    except Exception as e:
        console.print(f"\n[red]Error: {e}[/red]")
        logger.exception("Benchmark failed")
        raise
    finally:
        suite.cleanup()
    
    console.print("\n[bold green]Benchmark complete![/bold green]")


if __name__ == "__main__":
    main()
