"""
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            setup_datasets.py                                  ║
  Version:         0.0.47                                             ║
  Last Modified:   2026-04-15 18:43:44                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     169                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
"""

#!/usr/bin/env python3
"""
ThemisDB Comparative Benchmark - Dataset Setup

Downloads and prepares benchmark datasets from Hugging Face.
Run this before executing benchmarks to ensure data is available.

Usage:
    python setup_datasets.py --dataset wikipedia-simple --size 10000
    python setup_datasets.py --all --size 50000
"""

import sys
from pathlib import Path
import click
from rich.console import Console
from rich.progress import Progress, SpinnerColumn, TextColumn

sys.path.insert(0, str(Path(__file__).parent.parent))

from datasets.huggingface_loader import HuggingFaceDatasetLoader, BenchmarkDataset

console = Console()


@click.command()
@click.option("--dataset", 
              type=click.Choice(["wikipedia-simple", "synthetic", "all"]),
              default="wikipedia-simple",
              help="Dataset to download/generate")
@click.option("--size", default=10000, type=int,
              help="Number of documents to load")
@click.option("--vector-dim", default=384, type=int,
              help="Vector embedding dimensions")
@click.option("--output-dir", default="datasets/cache",
              type=click.Path(),
              help="Directory to cache datasets")
@click.option("--include-vectors/--no-vectors", default=True,
              help="Generate vector embeddings")
@click.option("--include-graph/--no-graph", default=True,
              help="Generate graph data")
@click.option("--seed", default=42, type=int,
              help="Random seed for reproducibility")
def main(dataset: str, size: int, vector_dim: int, output_dir: str,
         include_vectors: bool, include_graph: bool, seed: int):
    """
    Download and prepare benchmark datasets.
    
    This script downloads datasets from Hugging Face and prepares them
    for use in comparative benchmarks.
    """
    console.print("[bold blue]ThemisDB Benchmark Dataset Setup[/bold blue]")
    console.print("=" * 50)
    
    console.print(f"\nConfiguration:")
    console.print(f"  Dataset: {dataset}")
    console.print(f"  Size: {size:,} documents")
    console.print(f"  Vector dimensions: {vector_dim}")
    console.print(f"  Include vectors: {include_vectors}")
    console.print(f"  Include graph: {include_graph}")
    console.print(f"  Random seed: {seed}")
    
    # Create output directory
    cache_dir = Path(output_dir)
    cache_dir.mkdir(parents=True, exist_ok=True)
    
    # Initialize loader
    loader = HuggingFaceDatasetLoader(
        dataset_size=size,
        vector_dimensions=vector_dim,
        random_seed=seed
    )
    
    with Progress(
        SpinnerColumn(),
        TextColumn("[progress.description]{task.description}"),
        console=console,
    ) as progress:
        
        # Load documents
        if dataset in ("wikipedia-simple", "all"):
            task = progress.add_task("Loading Wikipedia dataset...", total=None)
            try:
                documents = loader.load_wikipedia_dataset()
                progress.update(task, 
                               description=f"[green]✓ Loaded {len(documents)} Wikipedia documents[/green]")
            except Exception as e:
                progress.update(task,
                               description=f"[yellow]Wikipedia unavailable, using synthetic: {e}[/yellow]")
                documents = loader._generate_synthetic_documents()
        else:
            task = progress.add_task("Generating synthetic documents...", total=None)
            documents = loader._generate_synthetic_documents()
            progress.update(task,
                           description=f"[green]✓ Generated {len(documents)} synthetic documents[/green]")
        
        # Generate vectors if requested
        vectors = None
        if include_vectors:
            task = progress.add_task("Generating vector embeddings...", total=None)
            vectors = loader.generate_vectors(documents)
            progress.update(task,
                           description=f"[green]✓ Generated {len(vectors)} vectors[/green]")
        
        # Generate graph if requested
        graph_nodes = None
        graph_edges = None
        if include_graph:
            task = progress.add_task("Generating graph data...", total=None)
            graph_nodes, graph_edges = loader.generate_graph_data()
            progress.update(task,
                           description=f"[green]✓ Generated {len(graph_nodes)} nodes, {len(graph_edges)} edges[/green]")
    
    # Create dataset object
    dataset_obj = BenchmarkDataset(
        documents=documents,
        vectors=vectors,
        graph_nodes=graph_nodes,
        graph_edges=graph_edges
    )
    
    # Save dataset info
    import json
    info = {
        "document_count": dataset_obj.document_count,
        "vector_count": dataset_obj.vector_count,
        "node_count": dataset_obj.node_count,
        "edge_count": dataset_obj.edge_count,
        "vector_dimensions": vector_dim,
        "seed": seed,
    }
    
    info_file = cache_dir / "dataset_info.json"
    with open(info_file, "w") as f:
        json.dump(info, f, indent=2)
    
    console.print(f"\n[green]✓ Dataset setup complete![/green]")
    console.print(f"\nDataset Summary:")
    console.print(f"  Documents: {dataset_obj.document_count:,}")
    console.print(f"  Vectors: {dataset_obj.vector_count:,}")
    console.print(f"  Graph nodes: {dataset_obj.node_count:,}")
    console.print(f"  Graph edges: {dataset_obj.edge_count:,}")
    console.print(f"\nDataset info saved to: {info_file}")


if __name__ == "__main__":
    main()
