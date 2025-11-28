"""
RESPO Vector Store Module

Provides a pluggable interface for different vector store backends.
"""

from respo.vectorstore.base import VectorStoreBase, VectorStoreFactory

__all__ = ["VectorStoreBase", "VectorStoreFactory"]
