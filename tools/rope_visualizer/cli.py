"""
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            cli.py                                             ║
  Version:         0.0.45                                             ║
  Last Modified:   2026-04-15 07:24:27                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     302                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 2a1fb04231  2026-03-03  Merge branch 'develop' into copilot/audit-src-module-docu... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
"""

#!/usr/bin/env python3
"""
RoPE Visualization CLI Tool

Command-line interface for visualizing Rotary Position Embeddings.
"""

import argparse
import json
import sys
import numpy as np
from pathlib import Path

# Add parent directory to path to import rope_visualizer
sys.path.insert(0, str(Path(__file__).parent.parent))

from rope_visualizer import RopeVisualizer


def load_embeddings(path: str) -> np.ndarray:
    """Load embeddings from file (supports .npy, .npz, .json)"""
    path_obj = Path(path)
    
    if not path_obj.exists():
        raise FileNotFoundError(f"Embeddings file not found: {path}")
    
    if path_obj.suffix == '.npy':
        return np.load(path)
    elif path_obj.suffix == '.npz':
        data = np.load(path)
        # Try to find embeddings key
        for key in ['embeddings', 'data', 'arr_0']:
            if key in data:
                return data[key]
        raise ValueError(f"Could not find embeddings in .npz file: {path}")
    elif path_obj.suffix == '.json':
        with open(path, 'r') as f:
            data = json.load(f)
        return np.array(data)
    else:
        raise ValueError(f"Unsupported file format: {path_obj.suffix}")


def load_config(path: str) -> dict:
    """Load RoPE configuration from JSON file"""
    with open(path, 'r') as f:
        return json.load(f)


def parse_positions(positions_str: str) -> list:
    """Parse position string (e.g., '0,50,100,200' or '0-500')"""
    if '-' in positions_str:
        # Range format: 0-500
        start, end = map(int, positions_str.split('-'))
        # Generate reasonable number of samples
        num_samples = min(50, end - start + 1)
        return list(np.linspace(start, end, num_samples, dtype=int))
    else:
        # Comma-separated: 0,50,100,200
        return [int(x.strip()) for x in positions_str.split(',')]


def cmd_visualize(args):
    """Visualize embeddings with rotation"""
    embeddings = load_embeddings(args.embeddings)
    
    config = {}
    if args.config:
        config = load_config(args.config)
    
    # Parse positions
    if args.positions:
        positions = parse_positions(args.positions)
        # Ensure we have enough embeddings
        if len(positions) > len(embeddings):
            positions = positions[:len(embeddings)]
    else:
        positions = list(range(len(embeddings)))
    
    # Create visualizer
    viz = RopeVisualizer(config)
    
    # Generate plot
    print(f"Visualizing {len(embeddings)} embeddings with {args.method} projection...")
    
    fig = viz.plot_embeddings(
        embeddings=embeddings,
        positions=positions,
        method=args.method,
        title=args.title,
        save_path=args.output,
        show=not args.no_show
    )
    
    print(f"Visualization saved to: {args.output}")


def cmd_heatmap(args):
    """Generate similarity heatmap"""
    embeddings = load_embeddings(args.embeddings)
    
    config = {}
    if args.config:
        config = load_config(args.config)
    
    # Use first embedding for heatmap
    embedding = embeddings[0]
    
    # Parse positions
    if args.positions:
        positions = parse_positions(args.positions)
    else:
        positions = list(range(0, 100, 10))  # Default: 0, 10, 20, ..., 90
    
    # Create visualizer
    viz = RopeVisualizer(config)
    
    # Generate heatmap
    print(f"Generating similarity heatmap for positions: {positions}")
    
    fig = viz.plot_similarity_matrix(
        embedding=embedding,
        positions=positions,
        metric=args.metric,
        title=args.title,
        save_path=args.output,
        show=not args.no_show
    )
    
    print(f"Heatmap saved to: {args.output}")


def cmd_trail(args):
    """Generate rotation trail visualization"""
    embeddings = load_embeddings(args.embeddings)
    
    config = {}
    if args.config:
        config = load_config(args.config)
    
    # Use first embedding for trail
    embedding = embeddings[0]
    
    # Parse positions
    if args.positions:
        positions = parse_positions(args.positions)
    else:
        positions = list(range(0, 100, 5))  # Default: 0, 5, 10, ..., 95
    
    # Create visualizer
    viz = RopeVisualizer(config)
    
    # Generate trail plot
    print(f"Generating rotation trail for {len(positions)} positions...")
    
    fig = viz.plot_rotation_trail(
        embedding=embedding,
        positions=positions,
        method=args.method,
        title=args.title,
        save_path=args.output,
        show=not args.no_show
    )
    
    print(f"Trail visualization saved to: {args.output}")


def cmd_theta(args):
    """Visualize theta distribution"""
    config = {}
    if args.config:
        config = load_config(args.config)
    
    hidden_dim = args.hidden_dim or config.get('hidden_dim', 128)
    base_theta = args.base_theta or config.get('base_theta', 10000.0)
    
    # Create visualizer
    viz = RopeVisualizer(config)
    
    # Generate theta distribution plot
    print(f"Generating theta distribution (hidden_dim={hidden_dim}, base_theta={base_theta})...")
    
    fig = viz.plot_theta_distribution(
        hidden_dim=hidden_dim,
        base_theta=base_theta,
        title=args.title,
        save_path=args.output,
        show=not args.no_show
    )
    
    print(f"Theta distribution saved to: {args.output}")


def main():
    parser = argparse.ArgumentParser(
        description='RoPE Visualization Tool',
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog="""
Examples:
  # Visualize embeddings with UMAP projection
  rope-visualize visualize --embeddings embeddings.npy --method umap --output plot.png

  # Generate similarity heatmap
  rope-visualize heatmap --embeddings embeddings.npy --positions 0,50,100,200 --output heatmap.png

  # Plot rotation trail
  rope-visualize trail --embeddings embeddings.npy --positions 0-100 --output trail.png

  # Visualize theta distribution
  rope-visualize theta --hidden-dim 128 --base-theta 10000 --output theta.png
        """
    )
    
    subparsers = parser.add_subparsers(dest='command', help='Available commands')
    
    # Visualize command
    parser_viz = subparsers.add_parser('visualize', help='Visualize embeddings with rotation')
    parser_viz.add_argument('--embeddings', required=True, help='Path to embeddings file (.npy, .npz, .json)')
    parser_viz.add_argument('--config', help='Path to RoPE config JSON file')
    parser_viz.add_argument('--output', required=True, help='Output file path')
    parser_viz.add_argument('--method', choices=['pca', 'tsne', 'umap'], default='pca',
                           help='Projection method (default: pca)')
    parser_viz.add_argument('--positions', help='Positions to visualize (e.g., "0,50,100" or "0-500")')
    parser_viz.add_argument('--title', help='Plot title')
    parser_viz.add_argument('--no-show', action='store_true', help='Do not display the plot')
    parser_viz.set_defaults(func=cmd_visualize)
    
    # Heatmap command
    parser_heatmap = subparsers.add_parser('heatmap', help='Generate similarity heatmap')
    parser_heatmap.add_argument('--embeddings', required=True, help='Path to embeddings file')
    parser_heatmap.add_argument('--config', help='Path to RoPE config JSON file')
    parser_heatmap.add_argument('--output', required=True, help='Output file path')
    parser_heatmap.add_argument('--positions', help='Positions to compare (e.g., "0,50,100")')
    parser_heatmap.add_argument('--metric', choices=['cosine', 'euclidean', 'dot'], default='cosine',
                               help='Similarity metric (default: cosine)')
    parser_heatmap.add_argument('--title', help='Plot title')
    parser_heatmap.add_argument('--no-show', action='store_true', help='Do not display the plot')
    parser_heatmap.set_defaults(func=cmd_heatmap)
    
    # Trail command
    parser_trail = subparsers.add_parser('trail', help='Generate rotation trail visualization')
    parser_trail.add_argument('--embeddings', required=True, help='Path to embeddings file')
    parser_trail.add_argument('--config', help='Path to RoPE config JSON file')
    parser_trail.add_argument('--output', required=True, help='Output file path')
    parser_trail.add_argument('--positions', help='Positions for trail (e.g., "0-100")')
    parser_trail.add_argument('--method', choices=['pca', 'tsne', 'umap'], default='pca',
                             help='Projection method (default: pca)')
    parser_trail.add_argument('--title', help='Plot title')
    parser_trail.add_argument('--no-show', action='store_true', help='Do not display the plot')
    parser_trail.set_defaults(func=cmd_trail)
    
    # Theta command
    parser_theta = subparsers.add_parser('theta', help='Visualize theta distribution')
    parser_theta.add_argument('--config', help='Path to RoPE config JSON file')
    parser_theta.add_argument('--output', required=True, help='Output file path')
    parser_theta.add_argument('--hidden-dim', type=int, help='Embedding dimension')
    parser_theta.add_argument('--base-theta', type=float, help='Base theta value')
    parser_theta.add_argument('--title', help='Plot title')
    parser_theta.add_argument('--no-show', action='store_true', help='Do not display the plot')
    parser_theta.set_defaults(func=cmd_theta)
    
    # Parse arguments
    args = parser.parse_args()
    
    if not args.command:
        parser.print_help()
        return 1
    
    try:
        args.func(args)
        return 0
    except Exception as e:
        print(f"Error: {e}", file=sys.stderr)
        import traceback
        traceback.print_exc()
        return 1


if __name__ == '__main__':
    sys.exit(main())
