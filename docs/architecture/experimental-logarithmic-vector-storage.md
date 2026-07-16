# Architekturvorschlag: Experimentelle logarithmische Zahlenrepräsentation für Vektor- und KI-nahe Workloads in ThemisDB

## Status

Experimental / Research Proposal

## Ziel

ThemisDB soll **experimentell** untersuchen, ob eine **logarithmische Zahlenrepräsentation** für **vektor- und KI-nahe Workloads** Vorteile bringt.

Die zentrale Hypothese ist:

- Viele KI- und Vektor-Workloads bestehen überwiegend aus Multiplikationen, Dot-Products, Matrixoperationen und Similarity-Berechnungen.
- Logarithmische Repräsentationen können solche Workloads unter bestimmten Bedingungen effizienter machen.
- Der mögliche Nutzen liegt primär in:
  - geringerem Speicherbedarf,
  - besserer Cache-Lokalität,
  - reduzierter Bandbreite,
  - potenziell effizienterer Ausführung spezialisierter numerischer Operatoren.

Diese Architektur ist **kein Vorschlag für den allgemeinen numerischen Kern von ThemisDB**. Sie ist ausdrücklich auf **Vektor-, Embedding-, Retrieval- und KI-nahe Verarbeitungspfade** begrenzt.

---

## Motivation

ThemisDB ist als hybrides Datenbanksystem mit Vektor-, Graph-, Relational- und Dateimodell positioniert. Dadurch existiert ein sinnvoller Einsatzbereich für alternative numerische Repräsentationen, insbesondere dort, wo:

- große Mengen an Embeddings gespeichert werden,
- Similarity Search ausgeführt wird,
- Dot-Product- oder Cosine-Similarity-Berechnungen dominieren,
- Re-Ranking oder kleine inferenznahe Operatoren integriert werden,
- Speicherbandbreite und Cache-Effizienz begrenzende Faktoren sind.

Im klassischen relationalen Kern gelten dagegen andere Prioritäten:

- exakte Semantik,
- stabile Vergleichbarkeit,
- deterministische Aggregation,
- Interoperabilität,
- möglichst geringe Komplexität im Storage- und Transaktionssystem.

Deshalb wird die logarithmische Darstellung **nicht** als universelles Speicherformat vorgeschlagen.

---

## Geltungsbereich

### Im Scope

Die Untersuchung soll sich auf folgende Bereiche konzentrieren:

1. **Embedding-Storage**
   - Speicherung dichter Vektoren in optional log-kodierter Form
   - Vergleich mit Float32, Float16, BFloat16, Int8 und weiteren kompakten Formaten

2. **Similarity Search**
   - Dot Product
   - Cosine Similarity
   - eventuell weitere ANN-nahe Distanz- oder Scoring-Funktionen

3. **Vektorindizes und abgeleitete Speicherformate**
   - materialisierte, workload-spezifische Repräsentationen
   - optional separate physische Formate neben dem kanonischen Datentyp

4. **KI-nahe Ausführungspfade**
   - Retrieval
   - Re-Ranking
   - vorbereitende numerische Operatoren für Inference-/RAG-ähnliche Abläufe

### Nicht im Scope

Folgende Bereiche sollen zunächst **nicht** betroffen sein:

- MVCC
- WAL / Recovery
- relationale Kernoperatoren
- generische numerische Datentypen
- exakte DECIMAL-/NUMERIC-Semantik
- B-Tree-/Hash-Schlüsselrepräsentationen
- allgemeine Aggregationen wie SUM/AVG/COUNT
- Sortierung, Range-Filters und Standard-Vergleichssemantik

---

## Architekturprinzip

### 1. Kanonisches Format bleibt unverändert

ThemisDB behält für Vektordaten zunächst ein etabliertes kanonisches Format bei, z. B.:

- Float32
- Float16
- BFloat16

Dieses Format bleibt die Referenz für:

- Persistenzsemantik,
- Interoperabilität,
- Debugging,
- deterministische Validierung,
- exaktes Re-Ranking oder Fallback-Pfade.

### 2. Logarithmische Repräsentation ist ein optionales Derived Format

Zusätzlich kann ThemisDB ein **abgeleitetes physisches Format** bereitstellen, z. B.:

- `LOG8`
- `LOG12`
- `LOG16`

Diese Formate sind keine neuen universellen SQL-/AQL-Standardtypen, sondern **interne oder explizit deklarierbare Optimierungsformate** für spezialisierte Vektor-Workloads.

### 3. Dual-Representation-Modell

Ein Vektor kann künftig in zwei Formen existieren:

- **Canonical Representation**
- **Optimized Log Representation**

Die optimierte Repräsentation darf:

- beim Ingest erzeugt werden,
- lazy erzeugt werden,
- materialisiert oder rebuildbar sein,
- für spezialisierte Ausführungspfade selektiv verwendet werden.

Dieses Modell reduziert das Risiko, die allgemeinen Systemeigenschaften von ThemisDB zu verschlechtern.

---

## Zielbild

### Logische Ebene

Benutzer arbeiten weiterhin mit normalen Vektorwerten und Standard-APIs.

Beispielhaft:

- Vektorspalten werden regulär definiert.
- Eine zusätzliche Storage- oder Index-Option aktiviert experimentelle log-kodierte Nebenstrukturen.

### Physische Ebene

Unterhalb der logischen Ebene kann ThemisDB optional erzeugen:

- log-kodierte Embedding-Blöcke,
- log-kodierte ANN-/Similarity-optimierte Segmente,
- Operatorpfade, die auf diesen Formaten direkt arbeiten.

### Ausführungsebene

Der Query Planner oder Execution Layer kann später entscheiden:

- normaler Pfad mit kanonischen Werten,
- optimierter Pfad mit log-kodierter Repräsentation,
- hybrider Pfad mit Approximation für Candidate Generation und exaktem Re-Ranking im kanonischen Format.

---

## Mögliche Komponenten

### A. Logarithmic Vector Encoding Layer

Verantwortlich für:

- Umwandlung von Vektorkomponenten in eine log-kodierte Darstellung,
- Verwaltung von Vorzeichen, Exponent-/Basiswahl und Spezialfällen,
- Behandlung von Nullwerten und sehr kleinen Beträgen,
- optionale Rekonstruktion in linearen Raum.

Offene Designfragen:

- Welche Basis wird verwendet?
- Wie werden Vorzeichen kodiert?
- Wie werden Null, Near-Zero, Underflow und Overflow behandelt?
- Welche Präzisionsstufen sind sinnvoll?

### B. Log-Optimized Vector Storage

Verantwortlich für:

- Speichern log-kodierter Embeddings,
- Blocklayout für Cache-Lokalität,
- Kompatibilität mit Segmenten, Pages oder Columnar-Strukturen,
- optional getrennte Persistenz oder materialisierte Nebenstruktur.

### C. Log-Aware Similarity Operators

Verantwortlich für spezialisierte Operatoren wie:

- dot product,
- cosine similarity,
- candidate scoring,
- ANN-nahe Vergleichsoperatoren.

Diese Operatoren sollen experimentell prüfen, ob ein Teil der numerischen Arbeit direkt auf der log-kodierten Repräsentation effizienter ausführbar ist.

### D. Planner / Execution Integration

Verantwortlich für:

- Auswahl des geeigneten Ausführungspfads,
- Fallback auf kanonische Repräsentation,
- Mischbetrieb zwischen Approximation und exakter Endbewertung,
- Kostenmodell für Speicher, CPU, Latenz und Genauigkeit.

### E. Evaluation & Benchmark Harness

Verantwortlich für:

- reproduzierbare Benchmarks,
- Recall-/Accuracy-Messung,
- Speicherverbrauch,
- Latenzmessung,
- Vergleich mit anderen Repräsentationen.

---

## Erwartete Vorteile

Die Untersuchung basiert auf folgenden erwarteten Potenzialen:

1. **Geringerer Speicherbedarf**
   - kompaktere Repräsentation von Embeddings
   - mehr Vektoren im RAM / Cache

2. **Bessere Cache-Lokalität**
   - geringere Transferkosten zwischen Speicherhierarchien

3. **Reduzierte Bandbreite**
   - insbesondere relevant bei großen Similarity-Scans

4. **Spezialisierte numerische Beschleunigung**
   - potenzielle Vereinfachung bestimmter multiply-dominierter Operatoren

5. **Bessere Skalierung für Vektor-Features**
   - vor allem bei Retrieval-, RAG- und Embedding-zentrierten Anwendungen

---

## Risiken

Die Untersuchung muss ausdrücklich folgende Risiken validieren:

1. **Genauigkeitsverlust**
   - Recall-Verlust bei Similarity Search
   - Verzerrung von Scores
   - instabile Rangfolgen bei knappen Abständen

2. **Konvertierungskosten**
   - Transformation in den Log-Raum kann Nutzen teilweise aufheben

3. **Komplexität der Addition / Akkumulation**
   - nicht alle numerischen Operationen profitieren gleichermaßen

4. **Implementierungskomplexität**
   - CPU-/SIMD-/GPU-Pfade werden komplizierter
   - Debugging und Validierung werden schwieriger

5. **Unklarer Mehrwert gegenüber etablierter Quantisierung**
   - Float16, BFloat16, Int8, Binary oder PQ können in der Praxis bereits ausreichend gut sein

6. **Technische Fragmentierung**
   - zu viele Spezialpfade können Wartbarkeit und Portabilität verschlechtern

---

## Forschungsfragen

Die experimentelle Untersuchung soll mindestens folgende Fragen beantworten:

1. Ist log-kodierter Embedding-Storage in ThemisDB speichereffizienter als Float16 oder Int8?
2. Welche Auswirkungen hat die Repräsentation auf Recall@K und Ranking-Qualität?
3. Für welche Operatoren entsteht tatsächlich ein Laufzeitvorteil?
4. Wie hoch sind die Konvertierungs- und Materialisierungskosten?
5. Welche Basis und Präzision liefern den besten Kompromiss aus Genauigkeit und Effizienz?
6. Lohnt sich der Ansatz nur für Candidate Generation oder auch für spätere Scoring-Phasen?
7. Ist ein hybrider Ansatz besser als eine vollständig log-kodierte Ausführung?

---

## Evaluationsstrategie

Die Entscheidung über eine Weiterentwicklung darf nur benchmarkbasiert erfolgen.

### Vergleichsbaselines

Mindestens zu vergleichen sind:

- Float32
- Float16
- BFloat16
- Int8
- gegebenenfalls Binary / weitere Quantisierungsverfahren
- experimentelle log-kodierte Formate

### Metriken

Zu messen sind mindestens:

- Recall@K
- nDCG / Ranking-Qualität
- Speicherverbrauch pro Vektor und pro Datensatz
- Ingest-Kosten
- Materialisierungskosten
- Query-Latenz (p50 / p95 / p99)
- Durchsatz
- Indexgröße
- Rebuild-/Compaction-Kosten
- optional Energieverbrauch

### Datensätze

Empfohlen sind:

- synthetische Datensätze mit kontrollierter Verteilung
- reale Embedding-Datensätze
- unterschiedliche Dimensionalitäten
- kleine, mittlere und große Korpora

---

## Rollout-Vorschlag

### Phase 1: Research Prototype

- isolierter Encoder/Decoder
- experimentelles Speicherformat
- Benchmark-Harness
- Vergleich gegen Float16 / Int8

### Phase 2: Operator Prototype

- Dot-Product- und Cosine-Similarity-Prototypen
- Candidate Generation auf log-kodierten Vektoren
- exaktes Re-Ranking im kanonischen Format

### Phase 3: Optional Engine Integration

- experimentelle Storage-Option
- plannergesteuerter Pfad
- Feature Flag / Build Flag

### Phase 4: Produktentscheidung

Nur wenn die Benchmarks klar zeigen, dass mindestens einer der folgenden Punkte signifikant besser ist:

- Speicherbedarf,
- Latenz,
- Durchsatz,
- TCO,
- oder Skalierbarkeit

bei akzeptabler Genauigkeit und beherrschbarer Komplexität.

---

## Architekturentscheidung

**Entscheidung:**

ThemisDB untersucht logarithmische Zahlenrepräsentationen **experimentell und optional** für **Vektor- und KI-nahe Workloads**.

**Nicht-Ziel:**

Die allgemeine relationale oder transaktionale Kernarchitektur von ThemisDB wird dadurch nicht ersetzt oder umgestellt.

**Bevorzugter Ansatz:**

- kanonisches Format beibehalten,
- log-kodierte Repräsentationen als abgeleitete physische Optimierung einsetzen,
- Approximation zunächst auf Similarity Search / Retrieval konzentrieren,
- Ergebnisse benchmarkbasiert bewerten.

---

## Vorläufige Empfehlung

Aus heutiger Sicht ist die logarithmische Repräsentation für ThemisDB am vielversprechendsten in:

- Embedding-Storage,
- Vektorindizes,
- Similarity Search,
- Retrieval-/RAG-nahe Pfade,
- numerisch kompakten KI-Hilfsoperatoren.

Sie sollte **nicht** als allgemeines Standardformat für sämtliche numerischen Daten eingeführt werden, solange kein klarer wissenschaftlicher und benchmarkbasierter Nachweis für einen breiten Nutzen vorliegt.
