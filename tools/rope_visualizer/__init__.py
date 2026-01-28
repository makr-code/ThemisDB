"""
RoPE Visualization Tools

This module provides visualization utilities for Rotary Position Embeddings (RoPE).
"""

from .visualizer import RopeVisualizer
from .utils import compute_similarity_matrix, project_embeddings

__version__ = "1.0.0"
__all__ = ['RopeVisualizer', 'compute_similarity_matrix', 'project_embeddings']
