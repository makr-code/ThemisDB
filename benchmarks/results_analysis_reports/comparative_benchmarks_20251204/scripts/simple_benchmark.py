"""
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            simple_benchmark.py                                ║
  Version:         0.0.47                                             ║
  Last Modified:   2026-04-15 18:43:44                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     416                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
"""

#!/usr/bin/env python3
"""
ThemisDB Polyglot Benchmark - Simplified Demo
Vergleicht ThemisDB vs Polyglot Persistence
"""

import time
import statistics
from dataclasses import dataclass, field
from typing import List
import httpx
import psycopg2
from pymongo import MongoClient
import json
from rich.console import Console
from rich.table import Table
from rich.progress import Progress, SpinnerColumn, TextColumn

console = Console()

# =============================================================================
# Configuration
# =============================================================================

THEMISDB_URL = "http://localhost:8765"
POSTGRESQL_CONN = "postgresql://benchmark:benchmark123@localhost:5432/benchmark"
MONGODB_CONN = "mongodb://localhost:27017/"

WARMUP_ITERATIONS = 5
BENCHMARK_ITERATIONS = 50

# =============================================================================
# Data Class
# =============================================================================

@dataclass
class BenchmarkResult:
    scenario: str
    database: str
    operation: str
    latencies_ms: List[float] = field(default_factory=list)

    @property
    def mean(self) -> float:
        return statistics.mean(self.latencies_ms) if self.latencies_ms else 0.0

    @property
    def median(self) -> float:
        return statistics.median(self.latencies_ms) if self.latencies_ms else 0.0

    @property
    def p95(self) -> float:
        if not self.latencies_ms:
            return 0.0
        sorted_vals = sorted(self.latencies_ms)
        idx = int(len(sorted_vals) * 0.95)
        return sorted_vals[idx]

    @property
    def p99(self) -> float:
        if not self.latencies_ms:
            return 0.0
        sorted_vals = sorted(self.latencies_ms)
        idx = int(len(sorted_vals) * 0.99)
        return sorted_vals[idx]


# =============================================================================
# Benchmark Scenarios
# =============================================================================

def benchmark_themis_simple_document_insert() -> BenchmarkResult:
    """ThemisDB: Simple Document Insert"""
    result = BenchmarkResult("Document Insert", "ThemisDB", "POST /entities")
    
    client = httpx.Client(base_url=THEMISDB_URL, timeout=30.0)
    
    # Warmup
    for i in range(WARMUP_ITERATIONS):
        payload = {
            "_key": f"warmup_{i}",
            "title": f"Warmup Document {i}",
            "content": "Sample content"
        }
        try:
            response = client.post("/entities", json=payload)
            response.raise_for_status()
        except httpx.HTTPError as e:
            print(f"[Warmup] HTTP error: {e}")
        except Exception as e:
            print(f"[Warmup] Error: {e}")
    
    # Benchmark
    for i in range(BENCHMARK_ITERATIONS):
        payload = {
            "_key": f"doc_{i:04d}",
            "title": f"Document {i}: Research Paper",
            "content": "This is a sample document about AI and machine learning.",
            "author": f"Author {i % 10}",
            "year": 2020 + (i % 5)
        }
        
        start = time.perf_counter()
        try:
            response = client.post("/entities", json=payload)
            response.raise_for_status()
            latency_ms = (time.perf_counter() - start) * 1000
            result.latencies_ms.append(latency_ms)
        except httpx.HTTPError as e:
            print(f"[Benchmark] HTTP error skipped: {e}")
        except Exception as e:
            print(f"[Benchmark] Error skipped: {e}")
    
    client.close()
    return result


def benchmark_postgres_simple_document_insert() -> BenchmarkResult:
    """PostgreSQL: Simple Document Insert"""
    result = BenchmarkResult("Document Insert", "PostgreSQL", "INSERT")
    
    conn = psycopg2.connect(POSTGRESQL_CONN)
    cursor = conn.cursor()
    
    # Create table if not exists
    cursor.execute("""
        DROP TABLE IF EXISTS test_documents CASCADE
    """)
    cursor.execute("""
        CREATE TABLE test_documents (
            id SERIAL PRIMARY KEY,
            title TEXT,
            content TEXT
        )
    """)
    conn.commit()
    
    # Warmup
    for i in range(WARMUP_ITERATIONS):
        cursor.execute("""
            INSERT INTO test_documents (title, content)
            VALUES (%s, %s)
        """, (f"Warmup {i}", "Sample"))
    conn.commit()
    
    # Benchmark
    for i in range(BENCHMARK_ITERATIONS):
        start = time.perf_counter()
        cursor.execute("""
            INSERT INTO test_documents (title, content)
            VALUES (%s, %s)
        """, (
            f"Document {i}: Research Paper",
            "This is a sample document about AI and machine learning."
        ))
        conn.commit()
        latency_ms = (time.perf_counter() - start) * 1000
        result.latencies_ms.append(latency_ms)
    
    cursor.close()
    conn.close()
    return result


def benchmark_mongodb_simple_document_insert() -> BenchmarkResult:
    """MongoDB: Simple Document Insert"""
    result = BenchmarkResult("Document Insert", "MongoDB", "insertOne")
    
    client = MongoClient(MONGODB_CONN, serverSelectionTimeoutMS=5000)
    db = client.benchmark
    collection = db.documents
    
    # Clear collection
    collection.delete_many({})
    
    # Warmup
    for i in range(WARMUP_ITERATIONS):
        collection.insert_one({
            "_id": f"warmup_{i}",
            "title": f"Warmup {i}",
            "content": "Sample",
            "author": f"Author {i}",
            "year": 2020
        })
    
    # Benchmark
    for i in range(BENCHMARK_ITERATIONS):
        start = time.perf_counter()
        collection.insert_one({
            "_id": f"doc_{i:04d}",
            "title": f"Document {i}: Research Paper",
            "content": "This is a sample document about AI and machine learning.",
            "author": f"Author {i % 10}",
            "year": 2020 + (i % 5)
        })
        latency_ms = (time.perf_counter() - start) * 1000
        result.latencies_ms.append(latency_ms)
    
    client.close()
    return result


def benchmark_themis_document_query() -> BenchmarkResult:
    """ThemisDB: Document Query"""
    result = BenchmarkResult("Document Query", "ThemisDB", "GET /entities/:key")
    
    client = httpx.Client(base_url=THEMISDB_URL, timeout=30.0)
    
    # Insert test document
    test_doc = {
        "_key": "query_test",
        "title": "Query Test Document",
        "content": "Content for query"
    }
    try:
        response = client.post("/entities", json=test_doc)
        response.raise_for_status()
    except httpx.HTTPError as e:
        print(f"[Setup] HTTP error: {e}")
    except Exception as e:
        print(f"[Setup] Error: {e}")
    
    # Warmup
    for _ in range(WARMUP_ITERATIONS):
        try:
            response = client.get(f"/entities/query_test")
            response.raise_for_status()
        except httpx.HTTPError as e:
            print(f"[Warmup] HTTP error: {e}")
        except Exception as e:
            print(f"[Warmup] Error: {e}")
    
    # Benchmark
    for _ in range(BENCHMARK_ITERATIONS):
        start = time.perf_counter()
        try:
            response = client.get(f"/entities/query_test")
            response.raise_for_status()
            latency_ms = (time.perf_counter() - start) * 1000
            result.latencies_ms.append(latency_ms)
        except httpx.HTTPError as e:
            print(f"[Benchmark] HTTP error skipped: {e}")
        except Exception as e:
            print(f"[Benchmark] Error skipped: {e}")
    
    client.close()
    return result


def benchmark_postgres_document_query() -> BenchmarkResult:
    """PostgreSQL: Document Query"""
    result = BenchmarkResult("Document Query", "PostgreSQL", "SELECT")
    
    conn = psycopg2.connect(POSTGRESQL_CONN)
    cursor = conn.cursor()
    
    # Insert test document
    cursor.execute("""
        INSERT INTO test_documents (title, content)
        VALUES (%s, %s)
    """, ("Query Test", "Content"))
    conn.commit()
    
    # Warmup
    for _ in range(WARMUP_ITERATIONS):
        cursor.execute("SELECT * FROM test_documents WHERE title = %s", ("Query Test",))
        cursor.fetchone()
    
    # Benchmark
    for _ in range(BENCHMARK_ITERATIONS):
        start = time.perf_counter()
        cursor.execute("SELECT * FROM test_documents WHERE title = %s", ("Query Test",))
        cursor.fetchone()
        latency_ms = (time.perf_counter() - start) * 1000
        result.latencies_ms.append(latency_ms)
    
    cursor.close()
    conn.close()
    return result


# =============================================================================
# Reporting
# =============================================================================

def print_results(results: List[BenchmarkResult]):
    """Print Rich table with benchmark results"""
    
    # Group by scenario
    scenarios = {}
    for r in results:
        if r.scenario not in scenarios:
            scenarios[r.scenario] = []
        scenarios[r.scenario].append(r)
    
    for scenario_name, scenario_results in scenarios.items():
        console.print(f"\n[bold cyan]📊 {scenario_name}[/bold cyan]\n")
        
        table = Table(title=scenario_name, show_lines=True)
        table.add_column("Database", style="magenta", no_wrap=True)
        table.add_column("Operation", style="green")
        table.add_column("Mean (ms)", justify="right", style="yellow")
        table.add_column("Median (ms)", justify="right")
        table.add_column("P95 (ms)", justify="right")
        table.add_column("P99 (ms)", justify="right")
        
        for r in scenario_results:
            table.add_row(
                r.database,
                r.operation,
                f"{r.mean:.2f}",
                f"{r.median:.2f}",
                f"{r.p95:.2f}",
                f"{r.p99:.2f}"
            )
        
        console.print(table)
        
        # Calculate improvements
        themis = next((r for r in scenario_results if "ThemisDB" in r.database), None)
        for r in scenario_results:
            if r != themis and themis:
                improvement = ((r.mean - themis.mean) / r.mean) * 100
                console.print(f"  [green]✨ ThemisDB vs {r.database}: {improvement:+.1f}% latency improvement[/green]")


def save_results(results: List[BenchmarkResult]):
    """Save results to JSON"""
    output = []
    for r in results:
        output.append({
            "scenario": r.scenario,
            "database": r.database,
            "operation": r.operation,
            "mean_ms": r.mean,
            "median_ms": r.median,
            "p95_ms": r.p95,
            "p99_ms": r.p99
        })
    
    filename = "benchmark_results_simple.json"
    with open(filename, "w") as f:
        json.dump(output, f, indent=2)
    
    console.print(f"\n✅ Results saved to [bold]{filename}[/bold]")


# =============================================================================
# Main
# =============================================================================

def main():
    console.print("\n[bold cyan]🚀 ThemisDB vs Polyglot Benchmark[/bold cyan]\n")
    console.print(f"Warmup: {WARMUP_ITERATIONS} iterations")
    console.print(f"Benchmark: {BENCHMARK_ITERATIONS} iterations\n")
    
    results = []
    
    with Progress(
        SpinnerColumn(),
        TextColumn("[progress.description]{task.description}"),
        console=console
    ) as progress:
        # Scenario 1: Document Insert
        task1 = progress.add_task("Running: Document Insert (ThemisDB)...", total=None)
        results.append(benchmark_themis_simple_document_insert())
        progress.update(task1, completed=True)
        
        task2 = progress.add_task("Running: Document Insert (PostgreSQL)...", total=None)
        results.append(benchmark_postgres_simple_document_insert())
        progress.update(task2, completed=True)
        
        task3 = progress.add_task("Running: Document Insert (MongoDB)...", total=None)
        results.append(benchmark_mongodb_simple_document_insert())
        progress.update(task3, completed=True)
        
        # Scenario 2: Document Query
        task4 = progress.add_task("Running: Document Query (ThemisDB)...", total=None)
        results.append(benchmark_themis_document_query())
        progress.update(task4, completed=True)
        
        task5 = progress.add_task("Running: Document Query (PostgreSQL)...", total=None)
        results.append(benchmark_postgres_document_query())
        progress.update(task5, completed=True)
    
    # Display results
    print_results(results)
    save_results(results)
    
    console.print("\n[bold green]✅ Benchmark Complete![/bold green]\n")


if __name__ == "__main__":
    main()
