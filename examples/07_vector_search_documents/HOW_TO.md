> **Aktueller Build-Flow:** `cmake --preset linux-ninja-release && cmake --build --preset linux-ninja-release`

# Dokumenten-Suche - Bedienungsanleitung

## 🚀 Start

```bash
python main.py
```

## 📋 Hauptfunktionen

### Dokumente hochladen

1. "Dokument hinzufügen"
2. Datei auswählen (PDF/TXT/DOCX)
3. Collection wählen
4. System generiert Embedding automatisch
5. Dokument ist durchsuchbar

### Semantische Suche

**Natürliche Sprache**:
1. Suchfrage eingeben (z.B. "Wie funktioniert X?")
2. System findet relevante Dokumente
3. Ergebnisse nach Relevanz sortiert

**Parameter anpassen**:
- **Top-K**: Anzahl Ergebnisse (5-50)
- **Threshold**: Minimale Ähnlichkeit (0.0-1.0)
- **Hybrid**: Vector + Volltext kombinieren

### RAG-Workflow

**Context abrufen**:
1. Frage stellen
2. System findet relevante Abschnitte
3. Context wird bereitgestellt
4. Optional: An LLM senden

**LLM-Integration**:
- ThemisDB Native LLM nutzen
- Oder externes LLM (OpenAI, etc.)
- Context automatisch übergeben

### Collections verwalten

**Collection erstellen**:
- Name und Beschreibung
- Embedding-Modell wählen
- Vektorraum-Dimensionen

**Dokumente organisieren**:
- Nach Thema gruppieren
- Zugriffskontrolle pro Collection
- Separate Suche möglich

## 💡 Tipps

- Verwenden Sie natürliche Fragen
- Kurze Dokumente = bessere Ergebnisse
- Hybrid Search für best of both worlds
- Regelmäßig Re-Index bei vielen Updates

## 🔍 Beispiel-Queries

- "Wie starte ich ThemisDB?"
- "Dokumentation zu Vector Search"
- "Best Practices für Performanz"

---

**Weitere Details**: Siehe VECTOR_SEARCH.md und EMBEDDINGS_GUIDE.md
