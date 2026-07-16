<!--
  META-INFORMATIONEN
  WordPress-Slug:    /developer
  Meta-Title:        ThemisDB für Entwickler – In 30 Minuten zum ersten PoC
  Meta-Description:  ThemisDB für Entwickler: PostgreSQL Wire Protocol, lokales LLM, Embeddings und Multi-Model-ACID in einem System. Kein Vendor Lock-in, offene APIs, PoC in 30 Minuten.
  OG-Title:          ThemisDB Developer – Multi-Model + Native KI, ohne Vendor Lock-in
  OG-Description:    PostgreSQL Wire Protocol, llama.cpp lokal, Vector Search, Graph und Full-Text in einer DB. Open APIs, keine API-Kosten, PoC in 30 Minuten. Jetzt mit Solution Engineer sprechen.
  Quelle:            docs/de/THEMISDB_MONETARY_VALUATION_ANALYSIS.confidential.md (v3.0, 08.03.2026)
-->

<!-- Gutenberg: Cover Block (Fullwidth Hero) -->
# ThemisDB für Entwickler: Multi-Model + lokale KI – in 30 Minuten zum PoC

**PostgreSQL Wire Protocol, eingebettetes LLM, Embeddings, Graph und Full-Text Search –
in einer einzigen Datenbank. Kein Vendor Lock-in, keine API-Kosten, kein Cloud-Zwang.**

<!-- Gutenberg: Buttons Block -->
[PoC mit Solution Engineer planen](/kontakt?intent=poc) | [Sales kontaktieren](/kontakt)

> Antwort innerhalb von 24 Stunden (Werktage). Wir begleiten Sie vom ersten Query bis zum produktiven Betrieb.

---

<!-- Gutenberg: Columns Block (3 Key Claims) -->
## Warum Entwickler ThemisDB wählen

| PostgreSQL-kompatibel | Kein API-Vendor Lock-in | 12 Features, 1 System |
|---|---|---|
| **Bestehende Tools & Treiber funktionieren sofort** | **LLM, Embeddings, STT/TTS lokal – keine API-Kosten** | **Multi-Model ACID ohne externe Microservices** |

---

<!-- Gutenberg: Heading Block -->
## Was ThemisDB für Entwickler einzigartig macht

### 1. PostgreSQL Wire Protocol – Ihre Tools funktionieren bereits

ThemisDB implementiert das PostgreSQL Wire Protocol vollständig. Das bedeutet:

- **psql**, **pgAdmin**, **DBeaver**, **DataGrip** – direkt nutzbar
- **SQLAlchemy**, **JDBC**, **ODBC**, **Prisma**, **Drizzle** – keine Treiberanpassungen
- **BI-Tools** (Metabase, Tableau, Grafana) – sofort kompatibel
- Bestehende PostgreSQL-Applikationen laufen ohne Codeänderungen

### 2. Native KI – keine Cloud-API, keine Latenz, keine Kosten

Statt teure Cloud-APIs zu nutzen, laufen alle KI-Funktionen direkt in der Datenbank:

- **LLM (llama.cpp):** Text-Generierung, RAG, semantische Suche – lokal
- **Embeddings:** Automatisch generiert und gecacht (bis 1,55 Mrd. items/sec mit Embedding Cache)
- **STT/TTS (Whisper.cpp + Piper):** Spracherkennung und -synthese direkt in der DB
- **Bildanalyse (ONNX/OpenCV):** Computer Vision ohne externe API
- **Vector Search:** 380 k items/sec, 99,5 % Recall@10; mit Cache 1.550× Speedup

### 3. Multi-Model ACID – ein System statt 5+ Microservices

Ein ThemisDB-Cluster ersetzt:

| Was Sie heute brauchen | Was ThemisDB bietet |
|------------------------|---------------------|
| PostgreSQL (relational) | ✅ Nativ |
| MongoDB (document) | ✅ Nativ |
| Elasticsearch (full-text) | ✅ Nativ |
| Milvus/Pinecone (vector) | ✅ Nativ + Embedding Cache |
| Neo4j (graph) | ✅ Nativ |
| InfluxDB (time-series) | ✅ Nativ |
| MQTT-Broker (IoT) | ✅ Nativ |
| LLM-Microservice | ✅ Nativ eingebettet |

Alle Modelle, eine Transaktion – ACID über alle Datenmodelle hinweg.

---

<!-- Gutenberg: Heading Block -->
## Technische Highlights

<!-- Gutenberg: Table Block -->
| Feature | Detail |
|---------|--------|
| **Wire Protocol** | PostgreSQL vollständig kompatibel |
| **LLM** | llama.cpp (GGUF/GGML), lokal, keine API |
| **Embeddings** | Auto-generiert + Embedding Cache (1,55 Mrd./sec) |
| **Vector Search** | 380 k items/sec, 99,5 % Recall@10, HNSW/IVF |
| **Full-Text Search** | BM25/TF-IDF nativ, ohne Elasticsearch |
| **Graph** | Property Graph, AQL-Abfragesprache |
| **Time-Series** | Nativ, Kompression, Window Functions |
| **MQTT Broker** | Native IoT-Integration, kein separater Broker |
| **HTTP/2 Server Push** | CDC mit ~0 ms Latenz für Echtzeit-Subscriptions |
| **Content Processing** | PDF, Office-Dokumente, Archive direkt verarbeiten |
| **Air-Gap** | Vollständig offline betreibbar |
| **Monitoring** | OTLP/Prometheus nativ |

---

<!-- Gutenberg: Heading Block -->
## Quickstart: 30 Minuten bis zur ersten Abfrage

```
# 1. ThemisDB starten (Docker oder Binary)
docker run -p 5432:5432 themisdb/themisdb:latest

# 2. Mit psql verbinden (Standard-PostgreSQL-Client)
psql -h localhost -p 5432 -U themis -d themisdb

# 3. Multi-Model: Relational + Vektor in einer Query
CREATE TABLE documents (id SERIAL, content TEXT, embedding VECTOR(1536));
INSERT INTO documents (content) VALUES ('ThemisDB vereint alles.');
SELECT id, content FROM documents
  WHERE embedding <-> query_embedding('ThemisDB lokal') < 0.3;
```

> Vollständige Dokumentation, Quickstart-Guide und Beispiele sind im Repository verfügbar.

---

<!-- Gutenberg: Heading Block -->
## Typische Entwicklungsszenarien

- **RAG-Anwendungen:** Dokumente einlesen (PDF/Office), automatisch embedden und lokal per LLM
  abfragen – alles in einem System ohne externe APIs
- **Semantische Suche:** Full-Text + Vector Search hybrid für präzise Suchergebnisse
- **KI-Chatbots (offline):** STT → LLM → TTS lokal, Air-Gap-fähig
- **Echtzeit-Dashboards:** HTTP/2 Server Push für CDC ohne Polling
- **IoT-Plattformen:** MQTT-Broker native + Time-Series + Alerting in einer DB
- **Graph-Analysen:** Property Graph mit AQL, kombiniert mit relationalen Daten (ACID)
- **Multimodales Search:** Text, Bilder und Sprachaufnahmen gemeinsam indizieren und abfragen

---

<!-- Gutenberg: Heading Block -->
## Kein Vendor Lock-in – Offene Standards durchgängig

| Standard | ThemisDB |
|----------|---------|
| **PostgreSQL Wire Protocol** | ✅ Vollständig |
| **OpenAPI / REST** | ✅ Nativ |
| **OTLP (OpenTelemetry)** | ✅ Nativ |
| **Prometheus Metrics** | ✅ Nativ |
| **MQTT 3.1/5.0** | ✅ Nativ |
| **ONNX (AI-Modelle)** | ✅ Nativ |
| **GGUF/GGML (LLM-Modelle)** | ✅ Nativ |
| **Kubernetes Operator** | ✅ (Enterprise) |

---

<!-- Gutenberg: Heading Block -->
## Häufig gestellte Fragen

<!-- Gutenberg: FAQ Block -->

### Muss ich meinen bestehenden Code anpassen?
Wenn Sie PostgreSQL verwenden: in der Regel nein. Das PostgreSQL Wire Protocol wird vollständig
unterstützt – Ihre Treiber, ORMs und Tools funktionieren ohne Änderungen.

### Welche LLM-Modelle werden unterstützt?
ThemisDB unterstützt GGUF- und GGML-kompatible Modelle via llama.cpp. Das umfasst alle gängigen
Open-Source-Modelle (Llama 3, Mistral, Phi-3, u.v.m.). Modelle werden lokal geladen – keine
API-Abhängigkeit.

### Wie viel Arbeitsspeicher/GPU wird benötigt?
ThemisDB läuft ab 8 GB RAM (ohne LLM) und unterstützt GPU-Beschleunigung für LLM und Embeddings
(CUDA, ROCm, Metal). Genaue Anforderungen hängen von Workload und Modellgröße ab.

### Gibt es eine Community oder Open-Source-Version?
Details zu Community-Editionen und Testversionen besprechen wir gerne im Gespräch. Es gibt keine
öffentliche Open-Source-Version.

### Wie skaliere ich ThemisDB?
ThemisDB unterstützt vertikales Scaling (mehr RAM/CPU) und horizontales Scaling via Kubernetes
Operator (Enterprise) mit Multi-Region Replication. RAID Sharding ermöglicht Partitionierung
auf DB-Ebene.

### Kann ich ThemisDB in meine CI/CD-Pipeline integrieren?
Ja. ThemisDB läuft als Docker-Container und ist Kubernetes-native (Enterprise Edition).
Integration in bestehende CI/CD-Pipelines (GitHub Actions, GitLab CI, etc.) ist direkt möglich.

---

<!-- Gutenberg: Cover Block (CTA-Sektion) -->
## Starten Sie Ihren PoC – wir begleiten Sie

30 Minuten bis zur ersten Abfrage. 2 Wochen bis zum aussagekräftigen Ergebnis.

<!-- Gutenberg: Buttons Block -->
[PoC mit Solution Engineer planen](/kontakt?intent=poc) | [Sales kontaktieren](/kontakt)

> Antwort innerhalb von 24 Stunden (Werktage). Wir helfen Ihnen, den richtigen Einstieg zu finden.
