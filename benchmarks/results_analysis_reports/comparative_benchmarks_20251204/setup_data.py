"""
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            setup_data.py                                      ║
  Version:         0.0.47                                             ║
  Last Modified:   2026-04-15 18:43:44                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     273                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
"""

#!/usr/bin/env python3
"""
Setup Benchmark Testdaten von Hugging Face
und starte Polyglot Benchmark
"""

import os
import sys
import time
from pathlib import Path

# Add benchmark scripts to path
sys.path.insert(0, str(Path(__file__).parent / "scripts"))

# Import datasets library BEFORE adding datasets dir to path
from huggingface_hub import list_datasets
try:
    from datasets import load_dataset
except ImportError:
    # Fallback: nur synthetische Daten generieren
    load_dataset = None

from rich.console import Console
from rich.progress import Progress, SpinnerColumn, TextColumn, DownloadColumn, TransferSpeedColumn, TimeRemainingColumn
import json

console = Console()

# =============================================================================
# Configuration
# =============================================================================

DATASETS_DIR = Path(__file__).parent / "datasets" / "huggingface"
DATASETS_DIR.mkdir(parents=True, exist_ok=True)

DATASETS_TO_LOAD = {
    "wikipedia": {
        "dataset": "wikipedia",
        "config": "20220301.simple",  # Simple English Wikipedia
        "split": "train",
        "limit": 100,  # First 100 articles for testing
        "description": "Wikipedia Simple English (100 articles)"
    },
    "squad": {
        "dataset": "squad",
        "config": None,
        "split": "train",
        "limit": 50,  # Q&A pairs
        "description": "SQuAD - Reading comprehension dataset (50 examples)"
    }
}

# =============================================================================
# Dataset Loading
# =============================================================================

def load_and_save_datasets():
    """Lade Testdaten von Hugging Face herunter"""
    console.print("\n[bold cyan]📥 Loading Hugging Face Datasets...[/bold cyan]\n")
    
    with Progress(
        SpinnerColumn(),
        TextColumn("[progress.description]{task.description}"),
        console=console
    ) as progress:
        for dataset_name, config in DATASETS_TO_LOAD.items():
            task = progress.add_task(
                f"Loading {config['description']}...",
                total=None
            )
            
            try:
                # Load dataset
                if config["config"]:
                    dataset = load_dataset(
                        config["dataset"],
                        config["config"],
                        split=config["split"],
                        streaming=False
                    )
                else:
                    dataset = load_dataset(
                        config["dataset"],
                        split=config["split"],
                        streaming=False
                    )
                
                # Limit samples
                if len(dataset) > config["limit"]:
                    dataset = dataset.select(range(config["limit"]))
                
                # Save to JSON
                output_file = DATASETS_DIR / f"{dataset_name}.json"
                dataset.to_json(str(output_file))
                
                file_size = output_file.stat().st_size / (1024 * 1024)  # MB
                console.print(f"  ✅ {dataset_name}: {len(dataset)} samples ({file_size:.1f} MB)")
                
                progress.update(task, completed=True)
            
            except Exception as e:
                console.print(f"  ⚠️  {dataset_name}: {str(e)}")
                progress.update(task, completed=True)

# =============================================================================
# Generate Synthetic Data for Benchmarks
# =============================================================================

def generate_benchmark_data():
    """Generiere synthetische Testdaten für Benchmarks"""
    console.print("\n[bold cyan]🔧 Generating Benchmark Test Data...[/bold cyan]\n")
    
    import random
    import hashlib
    
    # Generate synthetic documents
    documents = []
    for i in range(100):
        doc = {
            "_id": f"doc{i:04d}",
            "title": f"Document {i}: Research Paper on AI",
            "content": f"This is a sample document about AI and machine learning. " * (i % 5 + 1),
            "author": f"Author {i % 20}",
            "category": random.choice(["AI", "ML", "NLP", "Vision", "Robotics"]),
            "citations": random.randint(0, 500),
            "year": random.randint(2015, 2025),
            "keywords": ["AI", "machine learning", "neural networks", "deep learning"]
        }
        documents.append(doc)
    
    # Save documents
    docs_file = DATASETS_DIR / "benchmark_documents.json"
    with open(docs_file, "w") as f:
        json.dump(documents, f, indent=2)
    console.print(f"  ✅ Generated {len(documents)} benchmark documents")
    
    # Generate synthetic vectors (384-dim, normalized)
    import numpy as np
    vectors = []
    np.random.seed(42)
    for i in range(100):
        vec = np.random.normal(0, 1, 384).astype(np.float32)
        vec = vec / np.linalg.norm(vec)  # Normalize
        vectors.append({
            "doc_id": f"doc{i:04d}",
            "embedding": vec.tolist()
        })
    
    # Save vectors
    vecs_file = DATASETS_DIR / "benchmark_embeddings.json"
    with open(vecs_file, "w") as f:
        json.dump(vectors, f, indent=2)
    console.print(f"  ✅ Generated {len(vectors)} embedding vectors (384-dim)")
    
    # Generate graph relationships
    relationships = []
    for i in range(100):
        # Each doc cites 2-5 other docs
        num_citations = random.randint(2, 5)
        cited_docs = random.sample(range(100), num_citations)
        for cited in cited_docs:
            relationships.append({
                "source": f"doc{i:04d}",
                "target": f"doc{cited:04d}",
                "relation": "cites",
                "weight": random.uniform(0.5, 1.0)
            })
    
    # Save relationships
    rels_file = DATASETS_DIR / "benchmark_relationships.json"
    with open(rels_file, "w") as f:
        json.dump(relationships, f, indent=2)
    console.print(f"  ✅ Generated {len(relationships)} graph relationships")
    
    return documents, vectors, relationships

# =============================================================================
# Verify Databases
# =============================================================================

def verify_databases():
    """Prüfe ob alle Datenbanken erreichbar sind"""
    console.print("\n[bold cyan]🔍 Verifying Database Connectivity...[/bold cyan]\n")
    
    import httpx
    import psycopg2
    from pymongo import MongoClient
    
    checks = {
        "PostgreSQL": ("postgresql://benchmark:benchmark123@localhost:5432/benchmark", "psycopg2"),
        "MongoDB": ("mongodb://benchmark:benchmark123@localhost:27017/", "pymongo"),
        "ThemisDB": ("http://localhost:8765/health", "httpx"),
        "Neo4j": ("http://localhost:7474", "httpx"),
        "ClickHouse": ("http://localhost:8123", "httpx"),
        "Qdrant": ("http://localhost:6333", "httpx"),
        "Weaviate": ("http://localhost:8080", "httpx"),
    }
    
    results = {}
    for db_name, (conn_str, driver) in checks.items():
        try:
            if driver == "psycopg2":
                conn = psycopg2.connect(conn_str)
                conn.close()
                results[db_name] = "✅"
            elif driver == "pymongo":
                client = MongoClient(conn_str, serverSelectionTimeoutMS=2000)
                client.server_info()
                client.close()
                results[db_name] = "✅"
            elif driver == "httpx":
                response = httpx.get(conn_str, timeout=2)
                results[db_name] = "✅" if response.status_code < 500 else "⚠️"
        except Exception as e:
            results[db_name] = f"❌ {type(e).__name__}"
    
    for db, status in results.items():
        console.print(f"  {status} {db}")
    
    # Check if ThemisDB server is running
    try:
        httpx.get("http://localhost:8765/health", timeout=2)
        console.print("  ✅ ThemisDB Server: Running")
    except:
        console.print("  ⚠️  ThemisDB Server: Not running yet")

# =============================================================================
# Main
# =============================================================================

def main():
    console.print("\n[bold cyan]🚀 ThemisDB Polyglot Benchmark - Data Setup[/bold cyan]\n")
    
    # Step 1: Load Hugging Face Datasets
    # load_and_save_datasets()  # Skip HF download for now (can be slow)
    
    # Step 2: Generate synthetic benchmark data
    docs, vecs, rels = generate_benchmark_data()
    
    # Step 3: Verify database connectivity
    verify_databases()
    
    console.print("\n[bold green]✅ Data Setup Complete![/bold green]\n")
    console.print("[bold]Next Steps:[/bold]")
    console.print("  1. Starte ThemisDB Server: build-msvc\\Release\\themis_server.exe")
    console.print("  2. Warte auf Server-Start (Port 8765)")
    console.print("  3. Starte Benchmarks: python scripts/extended_polyglot_benchmark.py")
    console.print()

if __name__ == "__main__":
    main()
