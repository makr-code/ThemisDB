"""
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            visualizer.py                                      ║
  Version:         0.0.39                                             ║
  Last Modified:   2026-04-13 20:54:49                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     412                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 2a1fb04231  2026-03-03  Merge branch 'develop' into copilot/audit-src-module-docu... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
"""

"""
RoPE Visualizer Class

Provides high-level API for visualizing Rotary Position Embeddings.
"""

import numpy as np
import matplotlib.pyplot as plt
import seaborn as sns
from typing import List, Optional, Union, Tuple, Dict
from .utils import (
    compute_similarity_matrix,
    project_embeddings,
    apply_rope_rotation
)


class RopeVisualizer:
    """
    Main visualization class for Rotary Position Embeddings (RoPE).
    
    Provides methods to visualize embedding space transformations,
    similarity matrices, rotation trails, and more.
    """
    
    def __init__(self, config: Optional[Dict] = None):
        """
        Initialize the RoPE visualizer.
        
        Args:
            config: Optional configuration dictionary with keys:
                - hidden_dim: Embedding dimension (default: 128)
                - base_theta: Base frequency for RoPE (default: 10000.0)
                - figsize: Default figure size (default: (10, 8))
        """
        self.config = config or {}
        self.hidden_dim = self.config.get('hidden_dim', 128)
        self.base_theta = self.config.get('base_theta', 10000.0)
        self.figsize = self.config.get('figsize', (10, 8))
    
    def plot_embeddings(
        self,
        embeddings: np.ndarray,
        positions: Optional[List[int]] = None,
        method: str = 'pca',
        color_by: str = 'position',
        title: Optional[str] = None,
        save_path: Optional[str] = None,
        show: bool = True
    ) -> plt.Figure:
        """
        Plot embeddings in 2D space with optional rotation.
        
        Args:
            embeddings: Array of shape (n_samples, hidden_dim)
            positions: Optional list of positions for rotation
            method: Projection method ('pca', 'tsne', 'umap')
            color_by: Color by 'position' or 'index'
            title: Plot title
            save_path: Optional path to save the figure
            show: Whether to display the plot
        
        Returns:
            Matplotlib figure object
        """
        if positions is not None:
            # Apply rotation to embeddings
            rotated = []
            for emb, pos in zip(embeddings, positions):
                rotated.append(apply_rope_rotation(emb, pos, self.base_theta))
            embeddings_to_plot = np.array(rotated)
        else:
            embeddings_to_plot = embeddings
            positions = list(range(len(embeddings)))
        
        # Project to 2D
        projected = project_embeddings(embeddings_to_plot, method=method, n_components=2)
        
        # Create plot
        fig, ax = plt.subplots(figsize=self.figsize)
        
        # Color by position or index
        if color_by == 'position':
            colors = positions
            cmap = 'viridis'
        else:
            colors = range(len(embeddings))
            cmap = 'tab10'
        
        scatter = ax.scatter(
            projected[:, 0],
            projected[:, 1],
            c=colors,
            cmap=cmap,
            alpha=0.7,
            s=50
        )
        
        ax.set_xlabel(f'{method.upper()} Component 1')
        ax.set_ylabel(f'{method.upper()} Component 2')
        
        if title is None:
            title = f'Embedding Space ({method.upper()} projection)'
        ax.set_title(title)
        
        plt.colorbar(scatter, ax=ax, label=color_by.capitalize())
        plt.tight_layout()
        
        if save_path:
            plt.savefig(save_path, dpi=300, bbox_inches='tight')
        
        if show:
            plt.show()
        
        return fig
    
    def plot_rotation_trail(
        self,
        embedding: np.ndarray,
        positions: List[int],
        method: str = 'pca',
        title: Optional[str] = None,
        save_path: Optional[str] = None,
        show: bool = True
    ) -> plt.Figure:
        """
        Plot the trail of a single embedding as it rotates through positions.
        
        Args:
            embedding: Single embedding vector of shape (hidden_dim,)
            positions: List of positions to visualize
            method: Projection method ('pca', 'tsne', 'umap')
            title: Plot title
            save_path: Optional path to save the figure
            show: Whether to display the plot
        
        Returns:
            Matplotlib figure object
        """
        # Generate rotated embeddings
        rotated_embeddings = []
        for pos in positions:
            rotated = apply_rope_rotation(embedding, pos, self.base_theta)
            rotated_embeddings.append(rotated)
        
        rotated_embeddings = np.array(rotated_embeddings)
        
        # Project to 2D
        projected = project_embeddings(rotated_embeddings, method=method, n_components=2)
        
        # Create plot
        fig, ax = plt.subplots(figsize=self.figsize)
        
        # Plot trail with color gradient
        scatter = ax.scatter(
            projected[:, 0],
            projected[:, 1],
            c=positions,
            cmap='viridis',
            alpha=0.7,
            s=80,
            edgecolors='black',
            linewidth=0.5
        )
        
        # Draw connecting line
        ax.plot(projected[:, 0], projected[:, 1], 'k--', alpha=0.3, linewidth=1)
        
        # Mark start and end
        ax.scatter(projected[0, 0], projected[0, 1], 
                  marker='o', s=200, c='green', edgecolors='black', 
                  linewidth=2, label='Start', zorder=5)
        ax.scatter(projected[-1, 0], projected[-1, 1], 
                  marker='s', s=200, c='red', edgecolors='black', 
                  linewidth=2, label='End', zorder=5)
        
        ax.set_xlabel(f'{method.upper()} Component 1')
        ax.set_ylabel(f'{method.upper()} Component 2')
        
        if title is None:
            title = f'Rotation Trail (positions {positions[0]} to {positions[-1]})'
        ax.set_title(title)
        
        plt.colorbar(scatter, ax=ax, label='Position')
        ax.legend()
        plt.tight_layout()
        
        if save_path:
            plt.savefig(save_path, dpi=300, bbox_inches='tight')
        
        if show:
            plt.show()
        
        return fig
    
    def plot_similarity_matrix(
        self,
        embedding: np.ndarray,
        positions: List[int],
        metric: str = 'cosine',
        title: Optional[str] = None,
        save_path: Optional[str] = None,
        show: bool = True
    ) -> plt.Figure:
        """
        Plot similarity heatmap between rotated versions of an embedding.
        
        Args:
            embedding: Single embedding vector of shape (hidden_dim,)
            positions: List of positions to compare
            metric: Similarity metric ('cosine', 'euclidean', 'dot')
            title: Plot title
            save_path: Optional path to save the figure
            show: Whether to display the plot
        
        Returns:
            Matplotlib figure object
        """
        # Generate rotated embeddings
        rotated_embeddings = []
        for pos in positions:
            rotated = apply_rope_rotation(embedding, pos, self.base_theta)
            rotated_embeddings.append(rotated)
        
        rotated_embeddings = np.array(rotated_embeddings)
        
        # Compute similarity matrix
        similarity_matrix = compute_similarity_matrix(rotated_embeddings, metric=metric)
        
        # Create plot
        fig, ax = plt.subplots(figsize=self.figsize)
        
        # Plot heatmap
        sns.heatmap(
            similarity_matrix,
            xticklabels=positions,
            yticklabels=positions,
            cmap='coolwarm',
            center=0 if metric == 'cosine' else None,
            vmin=-1 if metric == 'cosine' else None,
            vmax=1,
            annot=len(positions) <= 10,  # Annotate if not too many positions
            fmt='.2f',
            ax=ax
        )
        
        ax.set_xlabel('Position')
        ax.set_ylabel('Position')
        
        if title is None:
            title = f'Position-wise Similarity Heatmap ({metric})'
        ax.set_title(title)
        
        plt.tight_layout()
        
        if save_path:
            plt.savefig(save_path, dpi=300, bbox_inches='tight')
        
        if show:
            plt.show()
        
        return fig
    
    def plot_3d_embeddings(
        self,
        embeddings: np.ndarray,
        positions: Optional[List[int]] = None,
        method: str = 'pca',
        color_by: str = 'position',
        title: Optional[str] = None,
        save_path: Optional[str] = None,
        show: bool = True
    ) -> plt.Figure:
        """
        Plot embeddings in 3D space.
        
        Args:
            embeddings: Array of shape (n_samples, hidden_dim)
            positions: Optional list of positions for rotation
            method: Projection method ('pca', 'tsne', 'umap')
            color_by: Color by 'position' or 'index'
            title: Plot title
            save_path: Optional path to save the figure
            show: Whether to display the plot
        
        Returns:
            Matplotlib figure object
        """
        if positions is not None:
            # Apply rotation to embeddings
            rotated = []
            for emb, pos in zip(embeddings, positions):
                rotated.append(apply_rope_rotation(emb, pos, self.base_theta))
            embeddings_to_plot = np.array(rotated)
        else:
            embeddings_to_plot = embeddings
            positions = list(range(len(embeddings)))
        
        # Project to 3D
        projected = project_embeddings(embeddings_to_plot, method=method, n_components=3)
        
        # Create 3D plot
        fig = plt.figure(figsize=self.figsize)
        ax = fig.add_subplot(111, projection='3d')
        
        # Color by position or index
        if color_by == 'position':
            colors = positions
        else:
            colors = range(len(embeddings))
        
        scatter = ax.scatter(
            projected[:, 0],
            projected[:, 1],
            projected[:, 2],
            c=colors,
            cmap='viridis',
            alpha=0.7,
            s=50
        )
        
        ax.set_xlabel(f'{method.upper()} Component 1')
        ax.set_ylabel(f'{method.upper()} Component 2')
        ax.set_zlabel(f'{method.upper()} Component 3')
        
        if title is None:
            title = f'3D Embedding Space ({method.upper()} projection)'
        ax.set_title(title)
        
        plt.colorbar(scatter, ax=ax, label=color_by.capitalize(), shrink=0.5)
        plt.tight_layout()
        
        if save_path:
            plt.savefig(save_path, dpi=300, bbox_inches='tight')
        
        if show:
            plt.show()
        
        return fig
    
    def plot_theta_distribution(
        self,
        hidden_dim: Optional[int] = None,
        base_theta: Optional[float] = None,
        title: Optional[str] = None,
        save_path: Optional[str] = None,
        show: bool = True
    ) -> plt.Figure:
        """
        Plot the distribution of theta values across rotation pairs.
        
        Args:
            hidden_dim: Embedding dimension (uses config default if None)
            base_theta: Base frequency (uses config default if None)
            title: Plot title
            save_path: Optional path to save the figure
            show: Whether to display the plot
        
        Returns:
            Matplotlib figure object
        """
        hidden_dim = hidden_dim or self.hidden_dim
        base_theta = base_theta or self.base_theta
        
        num_pairs = hidden_dim // 2
        indices = np.arange(num_pairs)
        theta_values = base_theta ** (-2.0 * indices / hidden_dim)
        
        # Create plot
        fig, ax = plt.subplots(figsize=self.figsize)
        
        ax.plot(indices, theta_values, 'o-', markersize=4, linewidth=1.5)
        ax.set_xlabel('Rotation Pair Index')
        ax.set_ylabel('Theta Value')
        ax.set_yscale('log')
        ax.grid(True, alpha=0.3)
        
        if title is None:
            title = f'Theta Distribution (hidden_dim={hidden_dim}, base_theta={base_theta})'
        ax.set_title(title)
        
        plt.tight_layout()
        
        if save_path:
            plt.savefig(save_path, dpi=300, bbox_inches='tight')
        
        if show:
            plt.show()
        
        return fig
