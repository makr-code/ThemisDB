# Systematische Review-Template Auswahl

Schnellreferenz zur Auswahl der richtigen Issue-Vorlage für Ihre ThemisDB-Komponenten-Review.

## 🎯 Schnellauswahl

**Frage: Zu welcher Kategorie gehört meine Komponente?**

### 1️⃣ Kerndatenbank-Operationen?
- Speicher, RocksDB, Persistenz
- Transaktionen, ACID, MVCC
- Query Engine, Optimizer, Executor
- Indizes (B-Tree, LSM, HNSW)
- AQL Parser

**→ Verwenden Sie:** `core_database_component_review.md`

---

### 2️⃣ AI/Machine Learning/LLM?
- LLM-Integration (llama.cpp)
- Embeddings, Vektorsuche
- RAG (Retrieval-Augmented Generation)
- Sprachverarbeitung
- Ethik, Bias-Erkennung
- Verantwortungsvolle KI

**→ Verwenden Sie:** `ai_llm_component_review.md`

---

### 3️⃣ Verteilte Systeme?
- Sharding, Partitionierung
- Replikation (Master-Slave, Multi-Master)
- Konsens (Raft, Paxos, Gossip)
- CDC (Change Data Capture)
- Verteilte Transaktionen (2PC, 3PC)
- Shard-übergreifende Operationen

**→ Verwenden Sie:** `distributed_systems_component_review.md`

---

### 4️⃣ Netzwerk/API/Protokolle?
- HTTP/REST APIs
- gRPC
- WebSocket
- MQTT
- PostgreSQL Wire Protocol
- GraphQL
- Protokoll-Implementierungen

**→ Verwenden Sie:** `network_api_component_review.md`

---

### 5️⃣ Etwas Anderes?
- Sicherheit/Authentifizierung
- Cache-Verwaltung
- Analytik
- Content-Verarbeitung
- Observability/Monitoring
- Geo-räumlich
- Zeitreihen
- Graph-spezifische Features
- Jede andere Komponente

**→ Verwenden Sie:** `SYSTEMATIC_COMPONENT_REVIEW_TEMPLATE.md` (Master-Template)

---

## 📋 Komponenten-Pfad → Template-Zuordnung

| Komponenten-Pfad | Template |
|-------------------|----------|
| `src/storage/` | core_database_component_review.md |
| `src/transaction/` | core_database_component_review.md |
| `src/query/` | core_database_component_review.md |
| `src/index/` | core_database_component_review.md |
| `src/aql/` | core_database_component_review.md |
| `src/llm/` | ai_llm_component_review.md |
| `src/embeddings/` | ai_llm_component_review.md |
| `src/rag/` | ai_llm_component_review.md |
| `src/voice/` | ai_llm_component_review.md |
| `src/governance/` | ai_llm_component_review.md |
| `src/ethics/` | ai_llm_component_review.md |
| `src/sharding/` | distributed_systems_component_review.md |
| `src/replication/` | distributed_systems_component_review.md |
| `src/cdc/` | distributed_systems_component_review.md |
| `src/api/` | network_api_component_review.md |
| `src/network/` (Protokoll-Handler) | network_api_component_review.md |
| `src/plugins/` (graphql, etc.) | network_api_component_review.md |
| Alle anderen Pfade | SYSTEMATIC_COMPONENT_REVIEW_TEMPLATE.md |

---

## 🔍 Wann Welches Template Verwenden

### Verwenden Sie spezialisierte Templates wenn:
- ✅ Komponente passt eindeutig in eine der 4 spezialisierten Kategorien
- ✅ Sie domänen-spezifische Review-Abschnitte benötigen
- ✅ Sie detaillierte Checklisten für diese Domäne benötigen

### Verwenden Sie das Master-Template wenn:
- ✅ Komponente passt nicht eindeutig in spezialisierte Kategorien
- ✅ Komponente umfasst mehrere Kategorien
- ✅ Sie Flexibilität zur Anpassung von Abschnitten benötigen
- ✅ Komponente ist neu und hat noch kein spezialisiertes Template

---

## 🚀 Ein Review-Issue Erstellen

1. **Template Wählen** (diese Anleitung verwenden)
2. **Zu GitHub Issues** → Neues Issue
3. **Template Auswählen** aus der Liste
4. **Header Ausfüllen:**
   - Komponenten-Name
   - Komponenten-Pfad
   - Review-Periode
   - Reviewer
5. **Alle Abschnitte Systematisch Ausfüllen**
6. **Aktionspunkte Hinzufügen** mit Verantwortlichen und Terminen
7. **Sign-Offs Einholen** von relevanten Teams
8. **Nächstes Review Planen**

---

## 📚 Weitere Hilfe Benötigt?

- **Lesen Sie:** `SYSTEMATIC_REVIEW_GUIDE.md` - Umfassender Leitfaden (English)
- **Lesen Sie:** `TEMPLATES_README.md` - Vollständige Template-Dokumentation
- **Fragen Sie:** Technical Lead oder Architecture Team

---

## 🎯 Was Wird In Jedem Template Überprüft?

### Alle Templates Enthalten:
- ✅ **Best Practices** - Code-Qualität, Design-Patterns, moderne Standards
- ✅ **Stand der Technik** - Forschungsarbeiten, Wettbewerbsanalyse, Trends
- ✅ **Dokumentation** - Code-Docs, User-Docs, Developer-Docs, Lücken
- ✅ **Roadmap** - Aktueller Stand, technische Schulden, kurz-/mittel-/langfristige Pläne
- ✅ **Sicherheit & Compliance** - Threat-Modeling, Schwachstellen, BSI C5, ISO 27001, DSGVO, NIS2
- ✅ **Performance** - Aktuelle Metriken, Engpässe, Optimierungsmöglichkeiten
- ✅ **Testing & Qualität** - Test-Coverage, Test-Typen, Test-Lücken
- ✅ **Abhängigkeiten** - Externe/interne Dependencies, Integration
- ✅ **Metriken & KPIs** - Code-Metriken, Qualitätsmetriken, Betriebsmetriken

### Spezialisierte Templates Fügen Hinzu:
- **Core Database:** ACID-Properties, Multi-Model, RocksDB-Integration, Transaktionsisolation
- **AI/LLM:** Modell-Integration, Vektor-Suche, RAG-Pipeline, LLM-Sicherheit (OWASP Top 10 für LLMs)
- **Distributed Systems:** Sharding-Strategien, Replikation, Konsens, CAP-Theorem, Fehlertoleranz
- **Network/API:** Protokoll-Implementierung, API-Design, OWASP API Security Top 10, Rate Limiting

---

## ⏱️ Review-Häufigkeit - Empfehlung

- **Vierteljährliche Reviews** für Kern-Komponenten (Storage, Transaction, Query, Security)
- **Halbjährliche Reviews** für stabile Komponenten
- **Nach größeren Änderungen** (neue Features, Refactoring, Sicherheitsvorfälle)
- **Pre-Release Reviews** für Komponenten mit signifikanten Änderungen

---

## 📊 Erwartete Ergebnisse Eines Systematischen Reviews

Nach einem vollständigen Review sollten Sie haben:
- ✅ Umfassendes Verständnis des Komponenten-Status
- ✅ Dokumentierte Lücken in Best Practices, Forschung, Dokumentation
- ✅ Klare Roadmap mit priorisierten Aktionspunkten
- ✅ Keine kritischen Sicherheitslücken (oder Sanierungsplan)
- ✅ Compliance-Lücken identifiziert und verfolgt
- ✅ Performance-Engpässe dokumentiert
- ✅ Test-Lücken identifiziert
- ✅ Sign-Offs von allen relevanten Teams

---

**Version:** 1.0.0  
**Erstellt:** 2026-02-01  
**Schnellreferenz von:** ThemisDB Core Team
