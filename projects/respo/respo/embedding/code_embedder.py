"""
RESPO Code Embedder

Generates embeddings for code using specialized models.
"""

from typing import Optional

import structlog
import torch
from sentence_transformers import SentenceTransformer

logger = structlog.get_logger(__name__)


class CodeEmbedder:
    """
    Code embedding service using sentence-transformers.

    Supports code-specific models like CodeBERT, StarEncoder, etc.
    """

    def __init__(
        self,
        model_name: str = "microsoft/codebert-base",
        device: Optional[str] = None,
        batch_size: int = 32,
    ) -> None:
        """
        Initialize code embedder.

        Args:
            model_name: Embedding model name
            device: Device (cuda, cpu, mps) - auto-detected if None
            batch_size: Batch size for encoding
        """
        self.model_name = model_name
        self.batch_size = batch_size

        # Auto-detect device
        if device is None:
            if torch.cuda.is_available():
                device = "cuda"
            elif torch.backends.mps.is_available():
                device = "mps"
            else:
                device = "cpu"

        self.device = device

        logger.info(
            "Loading embedding model",
            model=model_name,
            device=device,
        )

        self._model = SentenceTransformer(model_name, device=device)
        self._embedding_dim = self._model.get_sentence_embedding_dimension()

        logger.info(
            "Embedding model loaded",
            embedding_dim=self._embedding_dim,
        )

    @property
    def embedding_dim(self) -> int:
        """Get embedding dimension."""
        return self._embedding_dim

    def embed(self, texts: list[str]) -> list[list[float]]:
        """
        Generate embeddings for texts.

        Args:
            texts: List of texts to embed

        Returns:
            List of embedding vectors
        """
        if not texts:
            return []

        logger.debug("Generating embeddings", count=len(texts))

        embeddings = self._model.encode(
            texts,
            batch_size=self.batch_size,
            show_progress_bar=False,
            convert_to_numpy=True,
        )

        return embeddings.tolist()

    def embed_single(self, text: str) -> list[float]:
        """
        Generate embedding for a single text.

        Args:
            text: Text to embed

        Returns:
            Embedding vector
        """
        embeddings = self.embed([text])
        return embeddings[0] if embeddings else []

    def embed_code(
        self,
        code: str,
        language: Optional[str] = None,
        include_context: bool = True,
    ) -> list[float]:
        """
        Generate embedding for code with optional language context.

        Args:
            code: Source code to embed
            language: Programming language (optional)
            include_context: Whether to include language in embedding

        Returns:
            Embedding vector
        """
        if include_context and language:
            text = f"[{language}] {code}"
        else:
            text = code

        return self.embed_single(text)

    def similarity(self, embedding1: list[float], embedding2: list[float]) -> float:
        """
        Compute cosine similarity between two embeddings.

        Args:
            embedding1: First embedding
            embedding2: Second embedding

        Returns:
            Similarity score (0-1)
        """
        import numpy as np

        vec1 = np.array(embedding1)
        vec2 = np.array(embedding2)

        dot_product = np.dot(vec1, vec2)
        norm1 = np.linalg.norm(vec1)
        norm2 = np.linalg.norm(vec2)

        if norm1 == 0 or norm2 == 0:
            return 0.0

        return float(dot_product / (norm1 * norm2))
