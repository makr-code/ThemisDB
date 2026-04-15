"""
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            cli_usage_demo.py                                  ║
  Version:         0.0.44                                             ║
  Last Modified:   2026-04-15 05:32:45                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     130                                            ║
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
Example: CLI Usage Demo

Demonstrates how to use the rope-visualize CLI tool for different
visualization tasks.
"""

import subprocess
import sys
import numpy as np
from pathlib import Path

def run_command(cmd):
    """Run a shell command and print output"""
    print(f"\n{'='*60}")
    print(f"Running: {' '.join(cmd)}")
    print('='*60)
    result = subprocess.run(cmd, capture_output=True, text=True)
    print(result.stdout)
    if result.stderr:
        print("STDERR:", result.stderr, file=sys.stderr)
    return result.returncode == 0


def main():
    print("RoPE Visualizer CLI Demo")
    print("="*60)
    
    # Paths
    example_dir = Path(__file__).parent
    cli_script = example_dir.parent.parent / 'tools' / 'rope_visualizer' / 'cli.py'
    output_dir = Path('/tmp')
    
    # Generate test data if not exists
    embeddings_file = output_dir / 'test_embeddings.npy'
    if not embeddings_file.exists():
        print("\n📦 Generating test embeddings...")
        np.random.seed(42)
        embeddings = np.random.randn(100, 128).astype(np.float32)
        embeddings = embeddings / np.linalg.norm(embeddings, axis=1, keepdims=True)
        np.save(embeddings_file, embeddings)
        print(f"   Saved to: {embeddings_file}")
    
    config_file = example_dir / 'rope_config.json'
    
    # Example 1: Basic visualization with PCA
    run_command([
        sys.executable, str(cli_script), 'visualize',
        '--embeddings', str(embeddings_file),
        '--config', str(config_file),
        '--method', 'pca',
        '--output', str(output_dir / 'cli_demo_pca.png'),
        '--no-show'
    ])
    
    # Example 2: Heatmap with specific positions
    run_command([
        sys.executable, str(cli_script), 'heatmap',
        '--embeddings', str(embeddings_file),
        '--config', str(config_file),
        '--positions', '0,10,20,30,40,50',
        '--metric', 'cosine',
        '--output', str(output_dir / 'cli_demo_heatmap.png'),
        '--no-show'
    ])
    
    # Example 3: Rotation trail
    run_command([
        sys.executable, str(cli_script), 'trail',
        '--embeddings', str(embeddings_file),
        '--config', str(config_file),
        '--positions', '0-150',
        '--method', 'pca',
        '--output', str(output_dir / 'cli_demo_trail.png'),
        '--no-show'
    ])
    
    # Example 4: Theta distribution
    run_command([
        sys.executable, str(cli_script), 'theta',
        '--config', str(config_file),
        '--output', str(output_dir / 'cli_demo_theta.png'),
        '--no-show'
    ])
    
    # Example 5: Custom title
    run_command([
        sys.executable, str(cli_script), 'visualize',
        '--embeddings', str(embeddings_file),
        '--method', 'tsne',
        '--title', 'Custom Title: t-SNE Projection of RoPE Embeddings',
        '--output', str(output_dir / 'cli_demo_custom_title.png'),
        '--no-show'
    ])
    
    print("\n" + "="*60)
    print("✅ All CLI examples completed!")
    print(f"📁 Output files saved to: {output_dir}")
    print("\nGenerated files:")
    for f in output_dir.glob('cli_demo_*.png'):
        print(f"  - {f.name}")
    print("="*60)


if __name__ == '__main__':
    main()
