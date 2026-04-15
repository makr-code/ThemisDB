"""
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            advanced_analysis_example.py                       ║
  Version:         0.0.43                                             ║
  Last Modified:   2026-04-15 04:08:39                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     288                                            ║
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
Example: Advanced RoPE Analysis

This example demonstrates advanced usage of the RoPE visualization tools,
including comparison of different configurations and analysis workflows.
"""

import numpy as np
import sys
from pathlib import Path
import matplotlib.pyplot as plt

# Add rope_visualizer to path
sys.path.insert(0, str(Path(__file__).parent.parent.parent / 'tools'))

from rope_visualizer import RopeVisualizer
from rope_visualizer.utils import apply_rope_rotation, compute_similarity_matrix


def compare_theta_values():
    """Compare different base_theta values"""
    print("\n" + "="*60)
    print("Analysis 1: Comparing Different Base Theta Values")
    print("="*60)
    
    hidden_dim = 128
    theta_values = [1000.0, 10000.0, 100000.0]
    
    # Generate a single embedding
    np.random.seed(42)
    embedding = np.random.randn(hidden_dim).astype(np.float32)
    embedding = embedding / np.linalg.norm(embedding)
    
    positions = list(range(0, 200, 10))
    
    # Compare similarity decay for different theta values
    fig, axes = plt.subplots(1, 2, figsize=(15, 5))
    
    # Left plot: Similarity decay
    for theta in theta_values:
        similarities = []
        for pos in positions:
            rotated = apply_rope_rotation(embedding, pos, theta)
            sim = np.dot(embedding, rotated)
            similarities.append(sim)
        
        axes[0].plot(positions, similarities, 'o-', label=f'theta={theta}', alpha=0.7)
    
    axes[0].set_xlabel('Position')
    axes[0].set_ylabel('Cosine Similarity')
    axes[0].set_title('Similarity Decay: Impact of Base Theta')
    axes[0].legend()
    axes[0].grid(True, alpha=0.3)
    
    # Right plot: Theta distributions
    for theta in theta_values:
        num_pairs = hidden_dim // 2
        indices = np.arange(num_pairs)
        theta_vals = theta ** (-2.0 * indices / hidden_dim)
        axes[1].plot(indices, theta_vals, label=f'theta={theta}', alpha=0.7)
    
    axes[1].set_xlabel('Rotation Pair Index')
    axes[1].set_ylabel('Theta Value')
    axes[1].set_yscale('log')
    axes[1].set_title('Theta Distributions')
    axes[1].legend()
    axes[1].grid(True, alpha=0.3)
    
    plt.tight_layout()
    plt.savefig('/tmp/rope_theta_comparison.png', dpi=300)
    print("✅ Saved: /tmp/rope_theta_comparison.png")
    plt.show()


def analyze_position_separation():
    """Analyze how well different positions separate in embedding space"""
    print("\n" + "="*60)
    print("Analysis 2: Position Separation Analysis")
    print("="*60)
    
    hidden_dim = 128
    base_theta = 10000.0
    num_embeddings = 50
    
    # Generate embeddings
    np.random.seed(42)
    embeddings = np.random.randn(num_embeddings, hidden_dim).astype(np.float32)
    embeddings = embeddings / np.linalg.norm(embeddings, axis=1, keepdims=True)
    
    # Test different position ranges
    position_ranges = [
        (0, 10, 1),    # Close positions
        (0, 50, 5),    # Medium separation
        (0, 200, 10),  # Large separation
    ]
    
    fig, axes = plt.subplots(1, 3, figsize=(18, 5))
    
    for idx, (start, end, step) in enumerate(position_ranges):
        positions = list(range(start, end, step))
        
        # Apply rotations
        rotated_embeddings = []
        for emb, pos in zip(embeddings[:len(positions)], positions):
            rotated = apply_rope_rotation(emb, pos, base_theta)
            rotated_embeddings.append(rotated)
        
        rotated_embeddings = np.array(rotated_embeddings)
        
        # Compute similarity matrix
        similarity = compute_similarity_matrix(rotated_embeddings, metric='cosine')
        
        # Plot
        im = axes[idx].imshow(similarity, cmap='coolwarm', vmin=-1, vmax=1)
        axes[idx].set_title(f'Positions {start}-{end} (step {step})')
        axes[idx].set_xlabel('Embedding Index')
        axes[idx].set_ylabel('Embedding Index')
        plt.colorbar(im, ax=axes[idx])
    
    plt.tight_layout()
    plt.savefig('/tmp/rope_position_separation.png', dpi=300)
    print("✅ Saved: /tmp/rope_position_separation.png")
    plt.show()


def analyze_dimension_impact():
    """Analyze how embedding dimension affects rotation behavior"""
    print("\n" + "="*60)
    print("Analysis 3: Embedding Dimension Impact")
    print("="*60)
    
    dimensions = [64, 128, 256, 512]
    base_theta = 10000.0
    test_position = 100
    
    fig, axes = plt.subplots(2, 2, figsize=(15, 12))
    axes = axes.flatten()
    
    for idx, dim in enumerate(dimensions):
        # Generate embedding
        np.random.seed(42)
        embedding = np.random.randn(dim).astype(np.float32)
        embedding = embedding / np.linalg.norm(embedding)
        
        # Analyze similarity decay
        positions = list(range(0, 500, 10))
        similarities = []
        
        for pos in positions:
            rotated = apply_rope_rotation(embedding, pos, base_theta)
            sim = np.dot(embedding, rotated)
            similarities.append(sim)
        
        axes[idx].plot(positions, similarities, 'o-', markersize=3)
        axes[idx].set_xlabel('Position')
        axes[idx].set_ylabel('Cosine Similarity')
        axes[idx].set_title(f'Dimension: {dim}')
        axes[idx].grid(True, alpha=0.3)
        
        # Add statistics
        half_similarity_pos = None
        for i, sim in enumerate(similarities):
            if sim < 0.5:
                half_similarity_pos = positions[i]
                break
        
        if half_similarity_pos:
            axes[idx].axhline(y=0.5, color='r', linestyle='--', alpha=0.5)
            axes[idx].axvline(x=half_similarity_pos, color='r', linestyle='--', alpha=0.5)
            axes[idx].text(half_similarity_pos + 20, 0.6, 
                          f'50% at pos {half_similarity_pos}',
                          fontsize=10)
    
    plt.tight_layout()
    plt.savefig('/tmp/rope_dimension_impact.png', dpi=300)
    print("✅ Saved: /tmp/rope_dimension_impact.png")
    plt.show()


def create_comprehensive_report():
    """Generate a comprehensive visualization report"""
    print("\n" + "="*60)
    print("Analysis 4: Comprehensive Visualization Report")
    print("="*60)
    
    hidden_dim = 128
    base_theta = 10000.0
    
    # Generate embeddings
    np.random.seed(42)
    num_embeddings = 100
    embeddings = np.random.randn(num_embeddings, hidden_dim).astype(np.float32)
    embeddings = embeddings / np.linalg.norm(embeddings, axis=1, keepdims=True)
    
    # Create visualizer
    config = {
        'hidden_dim': hidden_dim,
        'base_theta': base_theta,
        'figsize': (10, 8)
    }
    viz = RopeVisualizer(config)
    
    # Create a 2x3 grid of visualizations
    fig = plt.figure(figsize=(20, 12))
    
    # 1. 2D PCA projection
    positions = list(range(0, 100, 2))
    viz.plot_embeddings(
        embeddings=embeddings[:50],
        positions=positions[:50],
        method='pca',
        color_by='position',
        title='2D Embedding Space (PCA)',
        save_path=None,
        show=False
    )
    
    # Generate individual plots for comprehensive report
    plots = [
        ('2D PCA', lambda: viz.plot_embeddings(
            embeddings[:40], list(range(0, 80, 2)), 'pca', show=False)),
        ('Rotation Trail', lambda: viz.plot_rotation_trail(
            embeddings[0], list(range(0, 200, 5)), show=False)),
        ('Similarity Heatmap', lambda: viz.plot_similarity_matrix(
            embeddings[0], list(range(0, 100, 10)), show=False)),
        ('Theta Distribution', lambda: viz.plot_theta_distribution(show=False)),
    ]
    
    for name, plot_func in plots:
        print(f"   Generating {name}...")
        plot_func()
    
    print("✅ Comprehensive report generated")


def main():
    """Run all analyses"""
    print("Advanced RoPE Visualization Analysis")
    print("="*60)
    
    try:
        compare_theta_values()
        analyze_position_separation()
        analyze_dimension_impact()
        create_comprehensive_report()
        
        print("\n" + "="*60)
        print("✅ All analyses complete!")
        print("\nGenerated files:")
        print("  - /tmp/rope_theta_comparison.png")
        print("  - /tmp/rope_position_separation.png")
        print("  - /tmp/rope_dimension_impact.png")
        print("="*60)
        
    except Exception as e:
        print(f"\n❌ Error during analysis: {e}")
        import traceback
        traceback.print_exc()
        return 1
    
    return 0


if __name__ == '__main__':
    sys.exit(main())
