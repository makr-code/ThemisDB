"""
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            simplified_polyglot_benchmark.py                   ║
  Version:         0.0.47                                             ║
  Last Modified:   2026-04-15 18:43:44                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     319                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
"""

#!/usr/bin/env python3
"""
Simplified Polyglot Persistence Benchmark
Compares ThemisDB (local Windows build) vs PostgreSQL + MongoDB

This demonstrates ThemisDB's unified multi-model advantage over polyglot persistence.
"""

import time
import json
import statistics
from typing import Dict, List, Tuple
import httpx
import psycopg2
from pymongo import MongoClient
from rich.console import Console
from rich.table import Table
from rich.progress import Progress

console = Console()

# Configuration
THEMIS_URL = "http://localhost:8765"
POSTGRES_CONN = "postgresql://benchmark:benchmark123@localhost:5432/benchmark"
MONGODB_URI = "mongodb://benchmark:benchmark123@localhost:27017/"

class BenchmarkResults:
    def __init__(self, name: str):
        self.name = name
        self.latencies: List[float] = []
    
    def add(self, latency_ms: float):
        self.latencies.append(latency_ms)
    
    def stats(self) -> Dict:
        if not self.latencies:
            return {}
        return {
            "mean": statistics.mean(self.latencies),
            "median": statistics.median(self.latencies),
            "p95": sorted(self.latencies)[int(len(self.latencies) * 0.95)],
            "p99": sorted(self.latencies)[int(len(self.latencies) * 0.99)],
            "min": min(self.latencies),
            "max": max(self.latencies),
            "count": len(self.latencies)
        }

# ==============================================================================
# Scenario 1: Document + Graph (Relational + Graph DBs vs ThemisDB)
# ==============================================================================

def benchmark_scenario_1_polyglot(iterations: int = 1000) -> BenchmarkResults:
    """PostgreSQL (documents) + simulated graph via JOINs"""
    results = BenchmarkResults("PostgreSQL (Document+Graph)")
    
    console.print("[yellow]Benchmarking PostgreSQL (Polyglot Approach)...[/yellow]")
    
    try:
        conn = psycopg2.connect(POSTGRES_CONN)
        cur = conn.cursor()
        
        # Create tables
        cur.execute("""
            CREATE TABLE IF NOT EXISTS users (
                id SERIAL PRIMARY KEY,
                name VARCHAR(255),
                email VARCHAR(255),
                data JSONB
            )
        """)
        cur.execute("""
            CREATE TABLE IF NOT EXISTS friendships (
                user_id INT REFERENCES users(id),
                friend_id INT REFERENCES users(id),
                since DATE,
                PRIMARY KEY (user_id, friend_id)
            )
        """)
        conn.commit()
        
        # Insert test data
        with Progress() as progress:
            task = progress.add_task("[cyan]Inserting test data...", total=iterations)
            for i in range(iterations):
                start = time.perf_counter()
                cur.execute(
                    "INSERT INTO users (name, email, data) VALUES (%s, %s, %s)",
                    (f"user_{i}", f"user{i}@example.com", json.dumps({"age": 20 + (i % 50)}))
                )
                # Create friendship
                if i > 0:
                    friend_id = max(1, i - (i % 10))
                    cur.execute(
                        "INSERT INTO friendships (user_id, friend_id, since) VALUES (%s, %s, CURRENT_DATE) ON CONFLICT DO NOTHING",
                        (i + 1, friend_id)
                    )
                conn.commit()
                latency = (time.perf_counter() - start) * 1000
                results.add(latency)
                progress.update(task, advance=1)
        
        cur.close()
        conn.close()
        
    except Exception as e:
        console.print(f"[red]PostgreSQL Error: {e}[/red]")
    
    return results

def benchmark_scenario_1_themisdb(iterations: int = 1000) -> BenchmarkResults:
    """ThemisDB unified multi-model (documents + graph)"""
    results = BenchmarkResults("ThemisDB (Unified Multi-Model)")
    
    console.print("[yellow]Benchmarking ThemisDB (Unified Approach)...[/yellow]")
    
    try:
        client = httpx.Client(base_url=THEMIS_URL, timeout=10.0)
        
        # Insert test data (documents with relationships)
        with Progress() as progress:
            task = progress.add_task("[cyan]Inserting test data...", total=iterations)
            for i in range(iterations):
                start = time.perf_counter()
                
                # Create user entity with embedded graph relationships
                user_data = {
                    "_key": f"user_{i}",
                    "name": f"user_{i}",
                    "email": f"user{i}@example.com",
                    "data": {"age": 20 + (i % 50)}
                }
                
                # Add relationship if not first user
                if i > 0:
                    friend_id = max(0, i - (i % 10))
                    user_data["_to"] = f"user_{friend_id}"
                
                response = client.put(f"/entities/users:user_{i}", json=user_data)
                latency = (time.perf_counter() - start) * 1000
                results.add(latency)
                progress.update(task, advance=1)
        
        client.close()
        
    except Exception as e:
        console.print(f"[red]ThemisDB Error: {e}[/red]")
    
    return results

# ==============================================================================
# Scenario 2: Document + Vector (MongoDB + ChromaDB vs ThemisDB)
# ==============================================================================

def benchmark_scenario_2_polyglot(iterations: int = 500) -> BenchmarkResults:
    """MongoDB (documents) + simulated vector storage"""
    results = BenchmarkResults("MongoDB (Document+Vector)")
    
    console.print("[yellow]Benchmarking MongoDB (Polyglot Approach)...[/yellow]")
    
    try:
        client = MongoClient(MONGODB_URI)
        db = client.benchmark
        collection = db.products
        
        # Insert test data
        with Progress() as progress:
            task = progress.add_task("[cyan]Inserting test data...", total=iterations)
            for i in range(iterations):
                start = time.perf_counter()
                
                # Simulate embedding (384-dim vector)
                embedding = [0.1] * 384
                
                doc = {
                    "product_id": f"prod_{i}",
                    "name": f"Product {i}",
                    "price": 10.0 + (i % 100),
                    "embedding": embedding,  # Stored as array (not optimized for vector search)
                    "category": f"category_{i % 10}"
                }
                
                collection.insert_one(doc)
                latency = (time.perf_counter() - start) * 1000
                results.add(latency)
                progress.update(task, advance=1)
        
        client.close()
        
    except Exception as e:
        console.print(f"[red]MongoDB Error: {e}[/red]")
    
    return results

def benchmark_scenario_2_themisdb(iterations: int = 500) -> BenchmarkResults:
    """ThemisDB unified multi-model (documents + vectors)"""
    results = BenchmarkResults("ThemisDB (Unified Multi-Model)")
    
    console.print("[yellow]Benchmarking ThemisDB (Unified Approach)...[/yellow]")
    
    try:
        client = httpx.Client(base_url=THEMIS_URL, timeout=10.0)
        
        # Create vector index
        console.print("[cyan]Creating vector index...[/cyan]")
        client.post("/index/vector/create", json={
            "table": "products",
            "dimensions": 384,
            "metric": "l2"
        })
        
        # Insert test data
        with Progress() as progress:
            task = progress.add_task("[cyan]Inserting test data...", total=iterations)
            for i in range(iterations):
                start = time.perf_counter()
                
                # Embedding
                embedding = [0.1] * 384
                
                product_data = {
                    "_key": f"prod_{i}",
                    "name": f"Product {i}",
                    "price": 10.0 + (i % 100),
                    "_embedding": embedding,  # Automatically indexed
                    "category": f"category_{i % 10}"
                }
                
                response = client.put(f"/entities/products:prod_{i}", json=product_data)
                latency = (time.perf_counter() - start) * 1000
                results.add(latency)
                progress.update(task, advance=1)
        
        client.close()
        
    except Exception as e:
        console.print(f"[red]ThemisDB Error: {e}[/red]")
    
    return results

# ==============================================================================
# Main Benchmark Runner
# ==============================================================================

def print_comparison_table(polyglot: BenchmarkResults, themis: BenchmarkResults):
    """Print comparison table"""
    poly_stats = polyglot.stats()
    themis_stats = themis.stats()
    
    if not poly_stats or not themis_stats:
        console.print("[red]Insufficient data for comparison[/red]")
        return
    
    table = Table(title=f"Benchmark Results: {polyglot.name} vs {themis.name}")
    
    table.add_column("Metric", style="cyan")
    table.add_column("Polyglot (ms)", style="yellow")
    table.add_column("ThemisDB (ms)", style="green")
    table.add_column("Improvement", style="magenta")
    
    metrics = ["mean", "median", "p95", "p99", "min", "max"]
    for metric in metrics:
        poly_val = poly_stats[metric]
        themis_val = themis_stats[metric]
        improvement = ((poly_val - themis_val) / poly_val) * 100 if poly_val > 0 else 0
        
        table.add_row(
            metric.upper(),
            f"{poly_val:.2f}",
            f"{themis_val:.2f}",
            f"{improvement:+.1f}%" if improvement != 0 else "±0%"
        )
    
    console.print(table)

def main():
    console.print("\n[bold cyan]═══ ThemisDB Polyglot Persistence Benchmark ═══[/bold cyan]\n")
    
    console.print("[bold]Scenario 1: Document + Graph Storage[/bold]")
    console.print("Polyglot: PostgreSQL (documents) + JOIN-based graph")
    console.print("ThemisDB: Unified document + native graph\n")
    
    poly_1 = benchmark_scenario_1_polyglot(iterations=100)
    themis_1 = benchmark_scenario_1_themisdb(iterations=100)
    print_comparison_table(poly_1, themis_1)
    
    console.print("\n[bold]Scenario 2: Document + Vector Storage[/bold]")
    console.print("Polyglot: MongoDB (documents) + array-based vectors")
    console.print("ThemisDB: Unified document + native vector index\n")
    
    poly_2 = benchmark_scenario_2_polyglot(iterations=50)
    themis_2 = benchmark_scenario_2_themisdb(iterations=50)
    print_comparison_table(poly_2, themis_2)
    
    console.print("\n[bold green]✅ Benchmark completed![/bold green]")

if __name__ == "__main__":
    main()
