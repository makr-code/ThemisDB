> **Build:** `cmake --preset linux-ninja-release && cmake --build --preset linux-ninja-release`

# Dokumenten-Suche - Vector Search & RAG

![Status](https://img.shields.io/badge/status-planned-yellow)
![Difficulty](https://img.shields.io/badge/difficulty-medium-orange)
![Duration](https://img.shields.io/badge/duration-40--50%20min-blue)

## 📝 Übersicht

Semantische Dokumentensuche mit Embeddings und RAG (Retrieval Augmented Generation). Demonstriert Vector Search Features.

## ✨ Features

- ✅ **Dokument-Upload** - PDF, TXT, DOCX
- ✅ **Automatische Embeddings** - sentence-transformers
- ✅ **Semantische Suche** - Ähnlichkeit statt Keywords
- ✅ **RAG-Workflow** - Context für LLMs
- ✅ **Ranking** - Relevanz-Scoring
- ✅ **Hybrid Search** - Vector + Volltext
- ✅ **Collection Management** - Dokumente organisieren

## 📊 Datenmodell

### Dokument
```python
{
    "id": "doc_uuid",
    "title": "Dokumenttitel",
    "content": "Volltext...",
    "embedding": [0.123, -0.456, ...],  # 384D Vector
    "metadata": {
        "author": "Max Mustermann",
        "created": "2025-12-22",
        "type": "pdf",
        "pages": 15
    },
    "collection": "technical_docs"
}
```

## 🔧 Verwendung

```bash
cd examples/07_vector_search_documents
pip install -r requirements.txt

# Modell wird automatisch heruntergeladen
python main.py
```

Siehe [HOW_TO.md](HOW_TO.md), [VECTOR_SEARCH.md](VECTOR_SEARCH.md), [EMBEDDINGS_GUIDE.md](EMBEDDINGS_GUIDE.md).

## 📚 Was Sie lernen

- **Vector Model** - Hochdimensionale Vektoren
- **HNSW/FAISS** - Effiziente Ähnlichkeitssuche
- **Embeddings** - Text zu Vektoren
- **RAG Pattern** - Context Retrieval für LLMs
- **Hybrid Search** - Kombination mehrerer Ansätze

## 🎯 Use Cases

1. **Dokumenten-Bibliothek durchsuchen**
2. **Ähnliche Dokumente finden**
3. **Fragen beantworten (Q&A)**
4. **Context für LLMs bereitstellen**

---

**Status**: Geplant
