> **Build:** `cmake --preset linux-ninja-release && cmake --build --preset linux-ninja-release`

# RoPE Visualization Tools

Visualization utilities for Rotary Position Embeddings (RoPE) in ThemisDB.

## Overview

This toolkit provides comprehensive visualization capabilities for understanding how Rotary Position Embeddings affect embedding spaces. It includes:

- **2D/3D Projections**: Visualize high-dimensional embeddings in 2D/3D space (PCA, t-SNE, UMAP)
- **Rotation Trails**: See how a single embedding moves through space as position changes
- **Similarity Heatmaps**: Compare rotated versions at different positions
- **Theta Distribution**: Understand the frequency spectrum of rotation pairs
- **CLI Tool**: Command-line interface for quick visualizations
- **Python API**: Programmatic access for notebooks and scripts

## Installation

### Install Dependencies

```bash
# Required dependencies
pip install numpy matplotlib seaborn scikit-learn

# Optional: For UMAP projections
pip install umap-learn

# Or install all at once
pip install -r tools/rope_visualizer/requirements.txt
```

## Quick Start

### Python API

```python
import numpy as np
from rope_visualizer import RopeVisualizer

# Generate or load embeddings
embeddings = np.random.randn(100, 128)  # 100 embeddings, 128 dimensions
positions = list(range(100))

# Initialize visualizer
config = {
    'hidden_dim': 128,
    'base_theta': 10000.0
}
viz = RopeVisualizer(config)

# Plot embeddings with rotation
viz.plot_embeddings(
    embeddings=embeddings,
    positions=positions,
    method='pca',
    save_path='embeddings.png'
)

# Plot rotation trail for a single embedding
viz.plot_rotation_trail(
    embedding=embeddings[0],
    positions=list(range(0, 200, 5)),
    save_path='trail.png'
)

# Generate similarity heatmap
viz.plot_similarity_matrix(
    embedding=embeddings[0],
    positions=list(range(0, 100, 10)),
    save_path='heatmap.png'
)
```

### Command-Line Interface

```bash
# Make CLI executable
chmod +x tools/rope_visualizer/cli.py

# Visualize embeddings
python tools/rope_visualizer/cli.py visualize \
  --embeddings embeddings.npy \
  --method pca \
  --output visualization.png

# Generate similarity heatmap
python tools/rope_visualizer/cli.py heatmap \
  --embeddings embeddings.npy \
  --positions 0,50,100,200 \
  --output heatmap.png

# Plot rotation trail
python tools/rope_visualizer/cli.py trail \
  --embeddings embeddings.npy \
  --positions 0-500 \
  --output trail.png

# Visualize theta distribution
python tools/rope_visualizer/cli.py theta \
  --hidden-dim 128 \
  --base-theta 10000 \
  --output theta.png
```

## Examples

See `examples/rope_visualization/` for complete examples:

```bash
# Run basic visualization example
python examples/rope_visualization/basic_visualization_example.py
```

This will generate several visualizations in `/tmp/`:
- `rope_embeddings_2d.png` - 2D embedding space
- `rope_rotation_trail.png` - Rotation trail
- `rope_similarity_heatmap.png` - Similarity heatmap
- `rope_theta_distribution.png` - Theta distribution
- `rope_embeddings_3d.png` - 3D embedding space

## API Reference

### RopeVisualizer Class

```python
RopeVisualizer(config: Optional[Dict] = None)
```

Main visualization class for RoPE.

**Configuration:**
- `hidden_dim` (int): Embedding dimension (default: 128)
- `base_theta` (float): Base frequency for RoPE (default: 10000.0)
- `figsize` (tuple): Default figure size (default: (10, 8))

**Methods:**

#### plot_embeddings
```python
plot_embeddings(
    embeddings: np.ndarray,
    positions: Optional[List[int]] = None,
    method: str = 'pca',
    color_by: str = 'position',
    title: Optional[str] = None,
    save_path: Optional[str] = None,
    show: bool = True
) -> plt.Figure
```

Plot embeddings in 2D space with optional rotation.

**Parameters:**
- `embeddings`: Array of shape (n_samples, hidden_dim)
- `positions`: Optional list of positions for rotation
- `method`: Projection method ('pca', 'tsne', 'umap')
- `color_by`: Color by 'position' or 'index'
- `title`: Plot title
- `save_path`: Optional path to save the figure
- `show`: Whether to display the plot

#### plot_rotation_trail
```python
plot_rotation_trail(
    embedding: np.ndarray,
    positions: List[int],
    method: str = 'pca',
    title: Optional[str] = None,
    save_path: Optional[str] = None,
    show: bool = True
) -> plt.Figure
```

Plot the trail of a single embedding as it rotates through positions.

#### plot_similarity_matrix
```python
plot_similarity_matrix(
    embedding: np.ndarray,
    positions: List[int],
    metric: str = 'cosine',
    title: Optional[str] = None,
    save_path: Optional[str] = None,
    show: bool = True
) -> plt.Figure
```

Plot similarity heatmap between rotated versions of an embedding.

**Parameters:**
- `metric`: Similarity metric ('cosine', 'euclidean', 'dot')

#### plot_3d_embeddings
```python
plot_3d_embeddings(
    embeddings: np.ndarray,
    positions: Optional[List[int]] = None,
    method: str = 'pca',
    color_by: str = 'position',
    title: Optional[str] = None,
    save_path: Optional[str] = None,
    show: bool = True
) -> plt.Figure
```

Plot embeddings in 3D space.

#### plot_theta_distribution
```python
plot_theta_distribution(
    hidden_dim: Optional[int] = None,
    base_theta: Optional[float] = None,
    title: Optional[str] = None,
    save_path: Optional[str] = None,
    show: bool = True
) -> plt.Figure
```

Plot the distribution of theta values across rotation pairs.

### Utility Functions

#### compute_similarity_matrix
```python
compute_similarity_matrix(
    embeddings: np.ndarray,
    metric: str = 'cosine'
) -> np.ndarray
```

Compute pairwise similarity matrix for embeddings.

#### project_embeddings
```python
project_embeddings(
    embeddings: np.ndarray,
    method: str = 'pca',
    n_components: int = 2,
    **kwargs
) -> np.ndarray
```

Project high-dimensional embeddings to lower dimensions.

#### apply_rope_rotation
```python
apply_rope_rotation(
    embedding: np.ndarray,
    position: int,
    base_theta: float = 10000.0
) -> np.ndarray
```

Apply RoPE rotation to an embedding vector.

## File Formats

### Embeddings

The tool supports multiple file formats for embeddings:

- **`.npy`**: NumPy binary format
  ```python
  np.save('embeddings.npy', embeddings)
  ```

- **`.npz`**: Compressed NumPy format
  ```python
  np.savez('embeddings.npz', embeddings=embeddings)
  ```

- **`.json`**: JSON array format
  ```python
  import json
  with open('embeddings.json', 'w') as f:
      json.dump(embeddings.tolist(), f)
  ```

### Configuration

Configuration files should be in JSON format:

```json
{
  "hidden_dim": 128,
  "base_theta": 10000.0,
  "num_rotation_pairs": 64
}
```

## Use Cases

### 1. Research & Development
- Understand how different theta values affect clustering
- Compare rotation patterns across domains
- Validate rotation implementation correctness

### 2. Debugging & Optimization
- Diagnose why certain positions don't separate well
- Find optimal base_theta for specific datasets
- Identify outliers in rotated space

### 3. Education & Documentation
- Interactive tutorials for RoPE mechanics
- Visual explanations in documentation
- Conference presentations and demos

## Performance Tips

- Use PCA for quick initial exploration (fastest)
- Use UMAP for better separation (requires umap-learn package)
- Use t-SNE for publication-quality plots (slowest)
- Limit number of samples for 3D plots (<1000 for smooth interaction)
- Pre-normalize embeddings for more stable projections

## Troubleshooting

### ImportError: No module named 'umap'

Install UMAP: `pip install umap-learn`

Or use PCA/t-SNE instead: `--method pca`

### ValueError: Need at least N samples

The projection method requires at least `n_components` samples. Either:
- Increase number of embeddings
- Use fewer components (2D instead of 3D)
- Use a different projection method

## Related Documentation

- [RoPE Implementation](../../include/index/rotary_embeddings.h)
- [Learnable RoPE](../../include/index/learnable_rope.h)
- [LoRA RoPE Integration](../../include/index/lora_rope.h)

## License

This tool is part of ThemisDB and follows the same MIT license.
