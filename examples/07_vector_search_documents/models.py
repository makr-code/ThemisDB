"""
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            models.py                                          ║
  Version:         0.0.41                                             ║
  Last Modified:   2026-04-14 11:22:14                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     211                                            ║
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
Datenmodelle für Dokumenten-Suche mit Vector Embeddings
"""
from dataclasses import dataclass, field, asdict
from datetime import datetime
from typing import List, Optional, Dict, Any
import hashlib


@dataclass
class Document:
    """
    Dokument mit Inhalt und Embedding für semantische Suche
    """
    id: str
    title: str
    content: str
    embedding: Optional[List[float]] = None
    metadata: Dict[str, Any] = field(default_factory=dict)
    collection: str = "default"
    created_at: str = field(default_factory=lambda: datetime.now().isoformat())
    
    def to_dict(self) -> dict:
        """Konvertierung zu Dictionary"""
        return asdict(self)
    
    @staticmethod
    def from_dict(data: dict) -> 'Document':
        """Erstellt Document aus Dictionary"""
        return Document(**data)
    
    def get_preview(self, max_length: int = 200) -> str:
        """Gibt eine Vorschau des Inhalts zurück"""
        if len(self.content) <= max_length:
            return self.content
        return self.content[:max_length] + "..."
    
    @staticmethod
    def create_id(title: str, content: str) -> str:
        """Erstellt eindeutige ID basierend auf Titel und Inhalt"""
        combined = f"{title}:{content[:100]}"
        return hashlib.md5(combined.encode()).hexdigest()


@dataclass
class SearchResult:
    """
    Suchergebnis mit Relevanz-Score
    """
    document: Document
    score: float  # Similarity score (0-1)
    rank: int = 0
    
    def to_dict(self) -> dict:
        """Konvertierung zu Dictionary"""
        return {
            'document': self.document.to_dict(),
            'score': self.score,
            'rank': self.rank
        }


@dataclass
class Collection:
    """
    Dokumenten-Sammlung für Organisation
    """
    name: str
    description: str = ""
    document_count: int = 0
    created_at: str = field(default_factory=lambda: datetime.now().isoformat())
    
    def to_dict(self) -> dict:
        """Konvertierung zu Dictionary"""
        return asdict(self)
    
    @staticmethod
    def from_dict(data: dict) -> 'Collection':
        """Erstellt Collection aus Dictionary"""
        return Collection(**data)


class EmbeddingGenerator:
    """
    Generiert Embeddings für Texte mit sentence-transformers
    """
    
    def __init__(self, model_name: str = "all-MiniLM-L6-v2"):
        """
        Initialisiert Generator mit Model
        
        Args:
            model_name: HuggingFace Model Name (default: all-MiniLM-L6-v2, 384 dim)
        """
        self.model_name = model_name
        self.model = None
        self._load_model()
    
    def _load_model(self):
        """Lädt das sentence-transformer Modell"""
        try:
            from sentence_transformers import SentenceTransformer
            self.model = SentenceTransformer(self.model_name)
            print(f"✅ Embedding-Modell geladen: {self.model_name}")
        except ImportError:
            print("⚠️ sentence-transformers nicht installiert. Embeddings deaktiviert.")
            print("   Install: pip install sentence-transformers")
        except Exception as e:
            print(f"⚠️ Fehler beim Laden des Modells: {e}")
    
    def generate(self, text: str) -> Optional[List[float]]:
        """
        Generiert Embedding für Text
        
        Args:
            text: Input Text
            
        Returns:
            Embedding Vector oder None bei Fehler
        """
        if not self.model:
            return None
        
        try:
            embedding = self.model.encode(text)
            return embedding.tolist()
        except Exception as e:
            print(f"⚠️ Fehler bei Embedding-Generierung: {e}")
            return None
    
    def generate_batch(self, texts: List[str]) -> List[Optional[List[float]]]:
        """
        Generiert Embeddings für mehrere Texte
        
        Args:
            texts: Liste von Texten
            
        Returns:
            Liste von Embedding Vectoren
        """
        if not self.model:
            return [None] * len(texts)
        
        try:
            embeddings = self.model.encode(texts)
            return [emb.tolist() for emb in embeddings]
        except Exception as e:
            print(f"⚠️ Fehler bei Batch-Embedding-Generierung: {e}")
            return [None] * len(texts)
    
    @property
    def dimension(self) -> int:
        """Gibt die Dimensionalität der Embeddings zurück"""
        if not self.model:
            return 0
        # all-MiniLM-L6-v2 hat 384 Dimensionen
        return 384


def cosine_similarity(vec1: List[float], vec2: List[float]) -> float:
    """
    Berechnet Cosine Similarity zwischen zwei Vektoren
    
    Args:
        vec1: Erster Vektor
        vec2: Zweiter Vektor
        
    Returns:
        Similarity Score (0-1, höher = ähnlicher)
    """
    if not vec1 or not vec2 or len(vec1) != len(vec2):
        return 0.0
    
    # Dot product
    dot_product = sum(a * b for a, b in zip(vec1, vec2))
    
    # Magnitudes
    magnitude1 = sum(a * a for a in vec1) ** 0.5
    magnitude2 = sum(b * b for b in vec2) ** 0.5
    
    if magnitude1 == 0 or magnitude2 == 0:
        return 0.0
    
    # Cosine similarity
    similarity = dot_product / (magnitude1 * magnitude2)
    
    # Normalize to 0-1 range (cosine can be -1 to 1)
    return (similarity + 1) / 2
