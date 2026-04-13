"""
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            basic_visualization_example.py                     ║
  Version:         0.0.38                                             ║
  Last Modified:   2026-04-13 04:12:56                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     143                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 2a1fb04231  2026-03-03  Merge branch 'develop' into copilot/audit-src-module-docu... ║
    • a629043ab2  2026-02-22  Audit: document gaps found - benchmarks and stale annotat... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
"""

"""
Example: Basic RoPE Visualization

This example demonstrates how to use the RoPE visualization tools
to understand rotary position embeddings.
"""

import numpy as np
import sys
from pathlib import Path

# Add rope_visualizer to path
sys.path.insert(0, str(Path(__file__).parent.parent.parent / 'tools'))

from rope_visualizer import RopeVisualizer


def main():
    print("RoPE Visualization Example\n" + "="*50)
    
    # Configuration
    hidden_dim = 128
    base_theta = 10000.0
    num_embeddings = 100
    
    # Generate random embeddings
    np.random.seed(42)
    embeddings = np.random.randn(num_embeddings, hidden_dim).astype(np.float32)
    
    # Normalize embeddings
    embeddings = embeddings / np.linalg.norm(embeddings, axis=1, keepdims=True)
    
    # Create positions
    positions = list(range(0, num_embeddings, 2))  # Every other position
    
    # Initialize visualizer
    config = {
        'hidden_dim': hidden_dim,
        'base_theta': base_theta,
        'figsize': (12, 8)
    }
    
    viz = RopeVisualizer(config)
    
    # Example 1: Visualize embeddings with PCA projection
    print("\n1. Creating 2D embedding visualization with PCA...")
    viz.plot_embeddings(
        embeddings=embeddings[:50],  # Use first 50 for clarity
        positions=positions[:50],
        method='pca',
        color_by='position',
        title='RoPE Embeddings in 2D (PCA Projection)',
        save_path='/tmp/rope_embeddings_2d.png'
    )
    print("   Saved: /tmp/rope_embeddings_2d.png")
    
    # Example 2: Plot rotation trail for a single embedding
    print("\n2. Creating rotation trail visualization...")
    single_embedding = embeddings[0]
    trail_positions = list(range(0, 200, 5))  # Positions 0 to 195 in steps of 5
    
    viz.plot_rotation_trail(
        embedding=single_embedding,
        positions=trail_positions,
        method='pca',
        title='Rotation Trail: How One Embedding Moves in Space',
        save_path='/tmp/rope_rotation_trail.png'
    )
    print("   Saved: /tmp/rope_rotation_trail.png")
    
    # Example 3: Generate similarity heatmap
    print("\n3. Creating similarity heatmap...")
    heatmap_positions = list(range(0, 100, 10))  # Positions 0, 10, 20, ..., 90
    
    viz.plot_similarity_matrix(
        embedding=single_embedding,
        positions=heatmap_positions,
        metric='cosine',
        title='Cosine Similarity Across Positions',
        save_path='/tmp/rope_similarity_heatmap.png'
    )
    print("   Saved: /tmp/rope_similarity_heatmap.png")
    
    # Example 4: Visualize theta distribution
    print("\n4. Creating theta distribution plot...")
    viz.plot_theta_distribution(
        hidden_dim=hidden_dim,
        base_theta=base_theta,
        title='Theta Values Across Rotation Pairs',
        save_path='/tmp/rope_theta_distribution.png'
    )
    print("   Saved: /tmp/rope_theta_distribution.png")
    
    # Example 5: 3D visualization
    print("\n5. Creating 3D embedding visualization...")
    viz.plot_3d_embeddings(
        embeddings=embeddings[:30],  # Use 30 embeddings for clarity
        positions=list(range(0, 30)),
        method='pca',
        color_by='position',
        title='RoPE Embeddings in 3D (PCA Projection)',
        save_path='/tmp/rope_embeddings_3d.png'
    )
    print("   Saved: /tmp/rope_embeddings_3d.png")
    
    print("\n" + "="*50)
    print("All visualizations complete!")
    print("Check /tmp directory for output files.")
    
    # Optionally save embeddings for CLI testing
    np.save('/tmp/test_embeddings.npy', embeddings)
    print("\nTest embeddings saved to: /tmp/test_embeddings.npy")
    print("You can now test the CLI tool with:")
    print("  python tools/rope_visualizer/cli.py visualize \\")
    print("    --embeddings /tmp/test_embeddings.npy \\")
    print("    --method pca --output /tmp/cli_test.png")


if __name__ == '__main__':
    main()
