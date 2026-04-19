> **Aktueller Build-Flow:** `cmake --preset linux-ninja-release && cmake --build --preset linux-ninja-release`

# Recommendation Engine - Anleitung

## 🚀 Schnellstart

```bash
cd examples/19_recommendation_engine
pip install -r requirements.txt
python main.py
```

## 📖 Hauptfunktionen

### Collaborative Filtering
- User-based: Findet ähnliche User
- Item-based: Findet ähnliche Items
- Matrix Factorization für bessere Predictions

### Content-Based Filtering
- Feature Extraction aus Items
- Cosine Similarity für Ähnlichkeit
- User-Profil basiert auf Interaktionen

### Hybrid Approach
- Kombiniert Collaborative + Content-Based
- Gewichtete Empfehlungen
- Bessere Cold-Start Behandlung

### A/B Testing
- Verschiedene Algorithmen testen
- Metriken vergleichen (CTR, Conversion)
- Besten Algorithmus auswählen

---

**Letzte Aktualisierung**: 2025-12-22
