"""
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            utils.py                                           ║
  Version:         0.0.39                                             ║
  Last Modified:   2026-04-13 20:54:49                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     195                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 2a1fb04231  2026-03-03  Merge branch 'develop' into copilot/audit-src-module-docu... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
"""

"""
Utility functions for RoPE visualization
"""

import numpy as np
from typing import List, Optional, Tuple
from sklearn.decomposition import PCA
from sklearn.manifold import TSNE

try:
    import umap
    UMAP_AVAILABLE = True
except ImportError:
    UMAP_AVAILABLE = False


def compute_similarity_matrix(embeddings: np.ndarray, metric: str = 'cosine') -> np.ndarray:
    """
    Compute pairwise similarity matrix for embeddings.
    
    Args:
        embeddings: Array of shape (n_samples, n_features)
        metric: Similarity metric ('cosine', 'euclidean', or 'dot')
    
    Returns:
        Similarity matrix of shape (n_samples, n_samples)
    """
    if metric == 'cosine':
        # Normalize embeddings
        norms = np.linalg.norm(embeddings, axis=1, keepdims=True)
        norms = np.where(norms == 0, 1, norms)  # Avoid division by zero
        normalized = embeddings / norms
        return normalized @ normalized.T
    elif metric == 'euclidean':
        # Compute pairwise Euclidean distances
        dists = np.linalg.norm(embeddings[:, None, :] - embeddings[None, :, :], axis=2)
        # Convert to similarity (inverse distance)
        return 1.0 / (1.0 + dists)
    elif metric == 'dot':
        return embeddings @ embeddings.T
    else:
        raise ValueError(f"Unknown metric: {metric}")


def project_embeddings(
    embeddings: np.ndarray,
    method: str = 'pca',
    n_components: int = 2,
    **kwargs
) -> np.ndarray:
    """
    Project high-dimensional embeddings to lower dimensions.
    
    Args:
        embeddings: Array of shape (n_samples, n_features)
        method: Projection method ('pca', 'tsne', 'umap')
        n_components: Number of dimensions to project to (2 or 3)
        **kwargs: Additional arguments for the projection method
    
    Returns:
        Projected embeddings of shape (n_samples, n_components)
    """
    if embeddings.shape[0] < n_components:
        raise ValueError(f"Need at least {n_components} samples for {n_components}D projection")
    
    if method == 'pca':
        reducer = PCA(n_components=n_components, **kwargs)
        return reducer.fit_transform(embeddings)
    
    elif method == 'tsne':
        # Set reasonable defaults for t-SNE
        tsne_kwargs = {
            'n_components': n_components,
            'perplexity': min(30, embeddings.shape[0] - 1),
            'random_state': kwargs.get('random_state', 42)
        }
        tsne_kwargs.update(kwargs)
        reducer = TSNE(**tsne_kwargs)
        return reducer.fit_transform(embeddings)
    
    elif method == 'umap':
        if not UMAP_AVAILABLE:
            raise ImportError("UMAP is not installed. Install with: pip install umap-learn")
        
        # Set reasonable defaults for UMAP
        umap_kwargs = {
            'n_components': n_components,
            'n_neighbors': min(15, embeddings.shape[0] - 1),
            'min_dist': 0.1,
            'random_state': kwargs.get('random_state', 42)
        }
        umap_kwargs.update(kwargs)
        reducer = umap.UMAP(**umap_kwargs)
        return reducer.fit_transform(embeddings)
    
    else:
        raise ValueError(f"Unknown projection method: {method}")


def rotate_embedding_2d(x: float, y: float, theta: float) -> Tuple[float, float]:
    """
    Rotate a 2D coordinate pair by angle theta.
    
    Args:
        x: X coordinate
        y: Y coordinate
        theta: Rotation angle in radians
    
    Returns:
        Tuple of (x_rotated, y_rotated)
    """
    cos_theta = np.cos(theta)
    sin_theta = np.sin(theta)
    x_rot = x * cos_theta - y * sin_theta
    y_rot = x * sin_theta + y * cos_theta
    return x_rot, y_rot


def compute_rotation_angles(
    position: int,
    hidden_dim: int,
    base_theta: float = 10000.0
) -> np.ndarray:
    """
    Compute rotation angles for RoPE at a given position.
    
    Args:
        position: Position index
        hidden_dim: Embedding dimension
        base_theta: Base frequency for RoPE
    
    Returns:
        Array of rotation angles for each dimension pair
    """
    num_pairs = hidden_dim // 2
    # θ_i = base^(-2i/d) where i ∈ [0, d/2)
    theta_values = base_theta ** (-2.0 * np.arange(num_pairs) / hidden_dim)
    # Angle for position m: m * θ_i
    angles = position * theta_values
    return angles


def apply_rope_rotation(
    embedding: np.ndarray,
    position: int,
    base_theta: float = 10000.0
) -> np.ndarray:
    """
    Apply RoPE rotation to an embedding vector.
    
    Args:
        embedding: Input embedding vector
        position: Position index
        base_theta: Base frequency for RoPE
    
    Returns:
        Rotated embedding vector
    """
    hidden_dim = len(embedding)
    if hidden_dim % 2 != 0:
        raise ValueError("Embedding dimension must be even")
    
    result = embedding.copy()
    angles = compute_rotation_angles(position, hidden_dim, base_theta)
    
    # Apply rotation to each coordinate pair
    for i, angle in enumerate(angles):
        x_idx = 2 * i
        y_idx = 2 * i + 1
        x, y = result[x_idx], result[y_idx]
        result[x_idx], result[y_idx] = rotate_embedding_2d(x, y, angle)
    
    return result
