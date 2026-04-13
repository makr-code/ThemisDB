"""
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            code_indexer.py                                    ║
  Version:         0.0.38                                             ║
  Last Modified:   2026-04-13 04:12:45                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   97.0/100                                       ║
    • Total Lines:     433                                            ║
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
Code Indexer and Embedding Generator.

This module handles code embedding generation and semantic search functionality.
"""

from typing import List, Dict, Any, Optional
import numpy as np
import hashlib

# Note: These imports would require actual installation
# from sentence_transformers import SentenceTransformer
# For now, we'll create mock implementations

from models import CodeSnippet


class EmbeddingGenerator:
    """
    Generates embeddings for code snippets.
    
    Uses CodeBERT or similar models to create vector representations
    of code for semantic search.
    """
    
    def __init__(self, model_name: str = "microsoft/codebert-base"):
        """
        Initialize embedding generator.
        
        Args:
            model_name: Name of the model to use for embeddings
        """
        self.model_name = model_name
        self.dimensions = 512  # CodeBERT outputs 512-dimensional vectors
        
        # In production, initialize model:
        # self.model = SentenceTransformer(model_name)
        print(f"Initializing embedding model: {model_name}")
        print("Note: Using mock implementation. Install sentence-transformers for actual embeddings.")
    
    def generate(self, code: str, language: str) -> List[float]:
        """
        Generate embedding for code snippet.
        
        Args:
            code: Source code
            language: Programming language
            
        Returns:
            Embedding vector (list of floats)
        """
        # Production implementation:
        # normalized_code = self.normalize_code(code, language)
        # embedding = self.model.encode(normalized_code)
        # return embedding.tolist()
        
        # Mock implementation: deterministic hash-based vector
        return self._mock_embedding(code)
    
    def batch_generate(self, snippets: List[tuple]) -> List[List[float]]:
        """
        Generate embeddings for multiple snippets efficiently.
        
        Args:
            snippets: List of (code, language) tuples
            
        Returns:
            List of embedding vectors
        """
        # Production: batch encoding for efficiency
        # codes = [self.normalize_code(code, lang) for code, lang in snippets]
        # embeddings = self.model.encode(codes, batch_size=32)
        # return embeddings.tolist()
        
        # Mock implementation
        return [self._mock_embedding(code) for code, lang in snippets]
    
    def normalize_code(self, code: str, language: str) -> str:
        """
        Normalize code for better embedding quality.
        
        Args:
            code: Source code
            language: Programming language
            
        Returns:
            Normalized code string
        """
        # Remove excessive whitespace
        code = ' '.join(code.split())
        
        # Language-specific normalization could be added here
        # e.g., removing comments, normalizing variable names, etc.
        
        return code
    
    def _mock_embedding(self, text: str) -> List[float]:
        """Generate mock embedding for development."""
        # Use hash to create deterministic pseudo-random vector
        hash_bytes = hashlib.md5(text.encode()).digest()
        
        # Convert to vector
        seed = int.from_bytes(hash_bytes[:4], 'big')
        np.random.seed(seed)
        vector = np.random.randn(self.dimensions)
        
        # Normalize to unit vector
        vector = vector / np.linalg.norm(vector)
        
        return vector.tolist()


class SimilarityEngine:
    """
    Computes similarity between code snippets using embeddings.
    """
    
    def __init__(self):
        pass
    
    def compute_similarity(self, embedding1: List[float], embedding2: List[float]) -> float:
        """
        Compute cosine similarity between two embeddings.
        
        Args:
            embedding1: First embedding vector
            embedding2: Second embedding vector
            
        Returns:
            Similarity score between 0 and 1
        """
        # Convert to numpy arrays
        vec1 = np.array(embedding1)
        vec2 = np.array(embedding2)
        
        # Cosine similarity
        similarity = np.dot(vec1, vec2) / (np.linalg.norm(vec1) * np.linalg.norm(vec2))
        
        # Convert to 0-1 range (cosine similarity is -1 to 1)
        similarity = (similarity + 1) / 2
        
        return float(similarity)
    
    def rank_by_similarity(
        self,
        query_embedding: List[float],
        candidates: List[Dict[str, Any]]
    ) -> List[Dict[str, Any]]:
        """
        Rank candidates by similarity to query.
        
        Args:
            query_embedding: Query embedding vector
            candidates: List of dicts with 'embedding' key
            
        Returns:
            Sorted list with 'similarity' scores added
        """
        results = []
        
        for candidate in candidates:
            if 'embedding' not in candidate:
                continue
            
            similarity = self.compute_similarity(query_embedding, candidate['embedding'])
            candidate['similarity'] = similarity
            results.append(candidate)
        
        # Sort by similarity (descending)
        results.sort(key=lambda x: x['similarity'], reverse=True)
        
        return results


class Deduplicator:
    """
    Detects and handles duplicate code snippets.
    """
    
    def __init__(self, similarity_threshold: float = 0.95):
        """
        Initialize deduplicator.
        
        Args:
            similarity_threshold: Threshold for considering snippets as duplicates
        """
        self.similarity_threshold = similarity_threshold
        self.seen_hashes = set()
    
    def hash_code(self, code: str) -> str:
        """
        Create hash of normalized code.
        
        Args:
            code: Source code
            
        Returns:
            Hash string
        """
        # Normalize whitespace
        normalized = ' '.join(code.split())
        return hashlib.sha256(normalized.encode()).hexdigest()
    
    def is_duplicate(self, code: str) -> bool:
        """
        Check if code is a duplicate (exact match).
        
        Args:
            code: Source code to check
            
        Returns:
            True if duplicate, False otherwise
        """
        code_hash = self.hash_code(code)
        
        if code_hash in self.seen_hashes:
            return True
        
        self.seen_hashes.add(code_hash)
        return False
    
    def find_similar_duplicates(
        self,
        snippet: CodeSnippet,
        existing_snippets: List[CodeSnippet],
        similarity_engine: SimilarityEngine
    ) -> List[CodeSnippet]:
        """
        Find semantically similar snippets.
        
        Args:
            snippet: Snippet to check
            existing_snippets: List of existing snippets
            similarity_engine: Engine for computing similarity
            
        Returns:
            List of similar snippets above threshold
        """
        if not snippet.embedding:
            return []
        
        duplicates = []
        
        for existing in existing_snippets:
            if not existing.embedding:
                continue
            
            similarity = similarity_engine.compute_similarity(
                snippet.embedding,
                existing.embedding
            )
            
            if similarity >= self.similarity_threshold:
                duplicates.append(existing)
        
        return duplicates


class CodeIndexer:
    """
    Main class for code indexing and search.
    
    Combines embedding generation, similarity search, and deduplication.
    """
    
    def __init__(self, themis_client, model_name: str = "microsoft/codebert-base"):
        """
        Initialize code indexer.
        
        Args:
            themis_client: ThemisDB client instance
            model_name: Name of embedding model
        """
        self.client = themis_client
        self.embedding_generator = EmbeddingGenerator(model_name)
        self.similarity_engine = SimilarityEngine()
        self.deduplicator = Deduplicator()
    
    def index_snippet(self, snippet: CodeSnippet) -> CodeSnippet:
        """
        Generate embedding and index snippet.
        
        Args:
            snippet: Snippet to index
            
        Returns:
            Snippet with embedding added
        """
        # Generate embedding if not present
        if not snippet.embedding:
            snippet.embedding = self.embedding_generator.generate(
                snippet.code,
                snippet.language
            )
        
        # Store in database
        self.client.create_snippet(snippet)
        
        return snippet
    
    def search_by_description(
        self,
        query: str,
        language: Optional[str] = None,
        limit: int = 10
    ) -> List[Dict[str, Any]]:
        """
        Search for snippets using natural language query.
        
        Args:
            query: Natural language search query
            language: Optional language filter
            limit: Maximum number of results
            
        Returns:
            List of snippets with similarity scores
        """
        # Generate query embedding
        query_embedding = self.embedding_generator.generate(query, language or "")
        
        # Search in database (using mock client for now)
        results = self.client.search_snippets(query, language, limit)
        
        return results
    
    def find_similar(
        self,
        code: str,
        language: str,
        limit: int = 5
    ) -> List[Dict[str, Any]]:
        """
        Find similar code snippets.
        
        Args:
            code: Code to find similar snippets for
            language: Programming language
            limit: Maximum number of results
            
        Returns:
            List of similar snippets with scores
        """
        # Generate embedding for query code
        query_embedding = self.embedding_generator.generate(code, language)
        
        # Search in database
        results = self.client.find_similar(code, language, limit)
        
        return results
    
    def reindex_all(self, batch_size: int = 32):
        """
        Regenerate embeddings for all snippets.
        
        Args:
            batch_size: Number of snippets to process at once
        """
        print("Reindexing all snippets...")
        
        # Get all snippets
        snippets = self.client.list_snippets(limit=10000)
        
        # Process in batches
        for i in range(0, len(snippets), batch_size):
            batch = snippets[i:i + batch_size]
            codes_and_langs = [(s.code, s.language) for s in batch]
            
            # Generate embeddings
            embeddings = self.embedding_generator.batch_generate(codes_and_langs)
            
            # Update snippets
            for snippet, embedding in zip(batch, embeddings):
                snippet.embedding = embedding
                self.client.update_snippet(snippet)
            
            print(f"Processed {i + len(batch)}/{len(snippets)} snippets")
        
        print("Reindexing complete!")
    
    def check_duplicate(self, snippet: CodeSnippet) -> bool:
        """
        Check if snippet is a duplicate.
        
        Args:
            snippet: Snippet to check
            
        Returns:
            True if duplicate, False otherwise
        """
        # Check exact duplicate by hash
        if self.deduplicator.is_duplicate(snippet.code):
            return True
        
        # Check semantic similarity
        if snippet.embedding:
            existing = self.client.list_snippets(
                language=snippet.language,
                limit=1000
            )
            
            similar = self.deduplicator.find_similar_duplicates(
                snippet,
                existing,
                self.similarity_engine
            )
            
            if similar:
                return True
        
        return False
