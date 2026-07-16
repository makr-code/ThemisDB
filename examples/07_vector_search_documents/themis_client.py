"""
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            themis_client.py                                   ║
  Version:         0.0.47                                             ║
  Last Modified:   2026-04-15 18:43:49                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     316                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
"""

"""
ThemisDB Client für Vector Search und Dokumentenverwaltung
"""
import requests
from typing import List, Optional, Dict, Any
from models import Document, SearchResult, Collection, EmbeddingGenerator, cosine_similarity


class VectorSearchClient:
    """
    Client für Dokumentensuche mit Vector Embeddings
    """
    
    def __init__(self, host: str = "localhost", port: int = 8080, timeout: int = 10):
        """
        Initialisiert Client
        
        Args:
            host: ThemisDB Host
            port: ThemisDB Port
            timeout: Request Timeout in Sekunden
        """
        self.base_url = f"http://{host}:{port}"
        self.timeout = timeout
        self.embedding_generator = EmbeddingGenerator()
        
        # In-Memory Cache für Demo
        self.documents: Dict[str, Document] = {}
        self.collections: Dict[str, Collection] = {}
    
    # Collection Operations
    
    def create_collection(self, name: str, description: str = "") -> Collection:
        """Erstellt neue Collection"""
        collection = Collection(name=name, description=description)
        self.collections[name] = collection
        return collection
    
    def get_collection(self, name: str) -> Optional[Collection]:
        """Gibt Collection zurück"""
        return self.collections.get(name)
    
    def list_collections(self) -> List[Collection]:
        """Listet alle Collections auf"""
        return list(self.collections.values())
    
    def delete_collection(self, name: str) -> bool:
        """Löscht Collection"""
        if name in self.collections:
            # Lösche auch alle Dokumente in der Collection
            docs_to_delete = [doc_id for doc_id, doc in self.documents.items() 
                            if doc.collection == name]
            for doc_id in docs_to_delete:
                del self.documents[doc_id]
            
            del self.collections[name]
            return True
        return False
    
    # Document Operations
    
    def add_document(self, document: Document, auto_embed: bool = True) -> Document:
        """
        Fügt Dokument hinzu
        
        Args:
            document: Dokument-Objekt
            auto_embed: Automatisch Embedding generieren
            
        Returns:
            Gespeichertes Dokument mit Embedding
        """
        # Generiere Embedding wenn gewünscht und noch nicht vorhanden
        if auto_embed and not document.embedding:
            document.embedding = self.embedding_generator.generate(document.content)
        
        # Speichere Dokument
        self.documents[document.id] = document
        
        # Update Collection Count
        if document.collection in self.collections:
            self.collections[document.collection].document_count = sum(
                1 for doc in self.documents.values() 
                if doc.collection == document.collection
            )
        
        return document
    
    def get_document(self, doc_id: str) -> Optional[Document]:
        """Gibt Dokument zurück"""
        return self.documents.get(doc_id)
    
    def list_documents(self, collection: Optional[str] = None) -> List[Document]:
        """
        Listet Dokumente auf
        
        Args:
            collection: Optional filter by collection
            
        Returns:
            Liste von Dokumenten
        """
        docs = list(self.documents.values())
        if collection:
            docs = [doc for doc in docs if doc.collection == collection]
        return docs
    
    def update_document(self, doc_id: str, **updates) -> Optional[Document]:
        """
        Aktualisiert Dokument
        
        Args:
            doc_id: Dokument ID
            **updates: Felder zum Aktualisieren
            
        Returns:
            Aktualisiertes Dokument oder None
        """
        document = self.documents.get(doc_id)
        if not document:
            return None
        
        # Update fields
        for key, value in updates.items():
            if hasattr(document, key):
                setattr(document, key, value)
        
        # Re-generate embedding if content changed
        if 'content' in updates:
            document.embedding = self.embedding_generator.generate(document.content)
        
        return document
    
    def delete_document(self, doc_id: str) -> bool:
        """Löscht Dokument"""
        if doc_id in self.documents:
            document = self.documents[doc_id]
            del self.documents[doc_id]
            
            # Update Collection Count
            if document.collection in self.collections:
                self.collections[document.collection].document_count = sum(
                    1 for doc in self.documents.values() 
                    if doc.collection == document.collection
                )
            return True
        return False
    
    # Search Operations
    
    def search(self, query: str, collection: Optional[str] = None, 
               top_k: int = 10) -> List[SearchResult]:
        """
        Semantische Suche mit Vector Similarity
        
        Args:
            query: Suchquery
            collection: Optional filter by collection
            top_k: Anzahl der Top-Ergebnisse
            
        Returns:
            Liste von SearchResult (sortiert nach Score)
        """
        # Generiere Query Embedding
        query_embedding = self.embedding_generator.generate(query)
        if not query_embedding:
            print("⚠️ Konnte kein Query-Embedding generieren")
            return []
        
        # Filter Dokumente
        documents = self.list_documents(collection)
        
        # Berechne Similarities
        results = []
        for doc in documents:
            if not doc.embedding:
                continue
            
            score = cosine_similarity(query_embedding, doc.embedding)
            results.append(SearchResult(document=doc, score=score))
        
        # Sortiere nach Score (absteigend)
        results.sort(key=lambda x: x.score, reverse=True)
        
        # Limit to top_k
        results = results[:top_k]
        
        # Setze Ranks
        for i, result in enumerate(results):
            result.rank = i + 1
        
        return results
    
    def search_hybrid(self, query: str, collection: Optional[str] = None,
                     top_k: int = 10, keyword_weight: float = 0.3) -> List[SearchResult]:
        """
        Hybrid Search: Vector + Keyword
        
        Args:
            query: Suchquery
            collection: Optional filter by collection
            top_k: Anzahl der Top-Ergebnisse
            keyword_weight: Gewicht für Keyword-Score (0-1)
            
        Returns:
            Liste von SearchResult (sortiert nach kombiniertem Score)
        """
        # Vector Search
        vector_results = self.search(query, collection, top_k * 2)
        
        # Keyword Search (simple contains check)
        query_lower = query.lower()
        for result in vector_results:
            # Berechne Keyword Score
            content_lower = result.document.content.lower()
            title_lower = result.document.title.lower()
            
            keyword_score = 0.0
            if query_lower in title_lower:
                keyword_score += 0.5
            if query_lower in content_lower:
                keyword_score += 0.3
            
            # Count occurrences
            occurrences = content_lower.count(query_lower)
            keyword_score += min(occurrences * 0.1, 0.2)
            
            # Kombiniere Scores
            vector_score = result.score
            combined_score = (1 - keyword_weight) * vector_score + keyword_weight * keyword_score
            result.score = combined_score
        
        # Re-sort
        vector_results.sort(key=lambda x: x.score, reverse=True)
        
        # Limit to top_k
        results = vector_results[:top_k]
        
        # Update Ranks
        for i, result in enumerate(results):
            result.rank = i + 1
        
        return results
    
    def find_similar(self, doc_id: str, top_k: int = 5) -> List[SearchResult]:
        """
        Findet ähnliche Dokumente zu gegebenem Dokument
        
        Args:
            doc_id: Dokument ID
            top_k: Anzahl der ähnlichen Dokumente
            
        Returns:
            Liste von ähnlichen Dokumenten
        """
        source_doc = self.documents.get(doc_id)
        if not source_doc or not source_doc.embedding:
            return []
        
        # Berechne Similarities
        results = []
        for doc_id_other, doc in self.documents.items():
            if doc_id_other == doc_id:  # Skip self
                continue
            if not doc.embedding:
                continue
            
            score = cosine_similarity(source_doc.embedding, doc.embedding)
            results.append(SearchResult(document=doc, score=score))
        
        # Sortiere und limite
        results.sort(key=lambda x: x.score, reverse=True)
        results = results[:top_k]
        
        # Setze Ranks
        for i, result in enumerate(results):
            result.rank = i + 1
        
        return results
    
    # Statistics
    
    def get_stats(self) -> Dict[str, Any]:
        """Gibt Statistiken zurück"""
        total_docs = len(self.documents)
        docs_with_embeddings = sum(1 for doc in self.documents.values() if doc.embedding)
        
        return {
            'total_documents': total_docs,
            'documents_with_embeddings': docs_with_embeddings,
            'total_collections': len(self.collections),
            'embedding_model': self.embedding_generator.model_name,
            'embedding_dimension': self.embedding_generator.dimension
        }
