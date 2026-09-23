

# **Konvergente Datenarchitekturen für souveräne KI-Systeme: Eine Evaluation von ThemisDB im Vergleich zu Hyperscaler-Modellen**

Abstract  
Diese Studie analysiert den architektonischen Paradigmenwechsel in der Datenhaltung für KI-Systeme der öffentlichen Verwaltung. Untersucht wird die Ablösung föderierter „Polyglot Persistence“-Ansätze (UDS3) durch konvergente, native Multi-Modell-Datenbanken am Beispiel von ThemisDB (v1.0.0). Im Zentrum steht der Vergleich der Transaktionsintegrität (ACID vs. BASE) und der Retrieval-Latenz zwischen der lokalen Themis-Architektur und den verteilten RAG-Stacks der Hyperscaler (AWS, Azure, GCP). Die Analyse der technischen Dokumentation und des Quellcodes belegt, dass ThemisDB durch die Integration von Vektor-, Graph- und relationalen Modellen in eine einzige C++/RocksDB-Engine das Konsistenzproblem verteilter Systeme („Saga-Pattern“) eliminiert. Mit der nachgewiesenen Implementierung von BSI-konformen Sicherheitsmodulen (Apache Ranger, PKCS\#11 HSM) stellt ThemisDB eine valide „Local-First“-Alternative für hochsensible Verwaltungsakte dar, die Datensouveränität mit Echtzeit-Performance verbindet.

---

## **1\. Einleitung und Motivation**

### **1.1 Der Souveränitätsimperativ im VCC-Ökosystem**

Die Digitalisierung der deutschen Verwaltung steht vor einer doppelten Herausforderung: dem demografisch bedingten Fachkräftemangel und der steigenden regulatorischen Komplexität (z. B. BImSchG-Vollzug). Das VCC-Ökosystem (Veritas, Covina, Clara) wurde konzipiert, um Verwaltungsmitarbeiter durch KI-gestützte Assistenzsysteme zu entlasten.1  
Die technische Kernanforderung ist ein „Verwaltungsprozess-Backbone“ (VPB), der probabilistische KI-Generierung (Retrieval-Augmented Generation, RAG) mit der deterministischen Rechtssicherheit von Verwaltungsakten vereint. Dies erfordert eine Dateninfrastruktur, die nicht nur semantische Ähnlichkeiten findet, sondern auch kausale Zusammenhänge (Graphen) und Metadaten (Relational) transaktional sicher verarbeitet.

### **1.2 Das Scheitern der Polyglot Persistence (UDS3)**

Die ursprüngliche Datenstrategie (UDS3) folgte dem Industriestandard der „Polyglot Persistence“: Die Nutzung spezialisierter Datenbanken für jeden Datentyp (Neo4j für Graphen, ChromaDB für Vektoren, PostgreSQL für Metadaten).1  
Diese Architektur scheiterte an der Transaktionsintegrität. Da keine globale Transaktionsklammer existiert, musste für modellübergreifende Operationen (z. B. DSGVO-Löschung) das Saga-Pattern implementiert werden. Dies führt systemimmanent zu „Eventual Consistency“ (BASE). Für rechtsverbindliche Bescheide ist der Zustand „eventuell korrekt“ jedoch inakzeptabel .

---

## **2\. ThemisDB: Architektur einer konvergenten Datenbank**

ThemisDB (Version 1.0.0) adressiert diese Defizite durch den Ansatz der **„Converged Database“** (Native Multi-Model). Anstatt Systeme zu koppeln, integriert sie alle Modelle in eine einzige, monolithische Engine.3

### **2.1 Core Engine: C++ und RocksDB TransactionDB**

Das Fundament bildet eine in **C++20** entwickelte Engine, die **Intel TBB** (Threading Building Blocks) für massive Parallelisierung nutzt. Als Speicher-Backend dient **RocksDB** im TransactionDB-Modus.3

* **Single Binary / In-Process:** Durch den Verzicht auf Netzwerkkommunikation zwischen den Modellen eliminiert ThemisDB die Latenz von Microservice-Architekturen. Benchmarks belegen eine P50-Latenz von **0,008 ms** für Punktabfragen.3  
* **MVCC & ACID:** ThemisDB implementiert **Multi-Version Concurrency Control (MVCC)** mit Snapshot Isolation. Dies ermöglicht, dass Schreiboperationen (z. B. Ingestion neuer Akten) Leseoperationen (z. B. laufende KI-Analyse) nicht blockieren. Kritisch ist hierbei, dass Transaktionen atomar über Vektor-, Graph- und Relational-Daten hinweg sind. Ein COMMIT garantiert die Persistenz aller Teilaspekte oder keines einzigen.3

### **2.2 Das Fünf-Säulen-Datenmodell**

ThemisDB bildet fünf logische Modelle auf ein physisches Key-Value-Format („Base Entity“) ab 3:

| Modell | Implementierung & Features | Strategischer Vorteil |
| :---- | :---- | :---- |
| **Relational** | Sekundärindizes (B-Tree auf LSM), Range-Scans, Composite-Keys. | Erlaubt deterministische Filterung (z. B. "Bescheide aus 2024"). |
| **Graph** | Native Adjazenzlisten (graph:out:...). Unterstützt BFS, Dijkstra, A\* und **temporale Traversierung** (bfsAtTime). | Ermöglicht Nachvollziehbarkeit von Verwaltungsentscheidungen über die Zeit. |
| **Vektor** | Persistenter **HNSW-Index** (L2, Cosine). Updates sind sofort transaktional sichtbar (Real-Time RAG). | Keine Synchronisationslücke zwischen Dokumenten-Upload und Auffindbarkeit. |
| **Time-Series** | Integrierte Engine mit **Gorilla Compression** (10-20x Ratio) und Continuous Aggregates. | Speicherung von IoT-Daten (Pegelstände) direkt am Prozessobjekt. |
| **Content** | Blob-Storage mit Pipeline-Prozessoren (Geo/Image) und Chunking-Logik. | Löst das Problem der referenziellen Integrität zwischen Datei und Metadaten. |

### **2.3 Advanced Query Language (AQL) & Pre-Filtering**

Ein Alleinstellungsmerkmal ist die Abfragesprache **AQL**, die „Interleaved Execution“ ermöglicht.3

* **Das Problem:** In Polyglot-Systemen (AWS) muss oft erst eine Vektorsuche (kNN) durchgeführt und das Ergebnis *nachträglich* gefiltert werden („Post-Filtering“). Dies ist ineffizient, da irrelevante Vektoren (z. B. aus falschen Jahren) gesucht werden.  
* **Die Themis-Lösung:** AQL erlaubt **„Pre-Filtering“**.  
  SQL  
  FILTER doc.status \== 'open' AND VECTOR\_DISTANCE(doc.vec, @query) \< 0.5

  Der Optimizer nutzt zuerst den relationalen Index, um das Kandidaten-Bitset einzuschränken, und führt die teure Vektorsuche *nur* auf diesem Subset aus.1

---

## **3\. Sicherheitsarchitektur und BSI-Konformität**

Entgegen älterer Planungsstände zeigt die Code-Analyse der Version 1.0.0, dass kritische Enterprise-Features bereits implementiert sind.3 Dies qualifiziert ThemisDB für den Einsatz im **VS-NfD**\-Bereich (Verschlusssache – Nur für den Dienstgebrauch).

### **3.1 Apache Ranger Integration**

Die Datei src/server/ranger\_adapter.cpp implementiert einen vollständigen Client für **Apache Ranger** \[Code Audit\].

* **Funktion:** Zentrales Policy-Management. ThemisDB fragt bei Ranger an, ob User X Zugriff auf Ressource Y hat.  
* **Bedeutung:** Dies erfüllt die BSI-Anforderung nach einer einheitlichen, auditierbaren Autorisierungsschicht über alle Systeme hinweg.

### **3.2 Kryptografie und HSM (PKCS\#11)**

Die Datei src/security/hsm\_provider\_pkcs11.cpp belegt die Integration von **Hardware Security Modules** (HSM) \[Code Audit\].

* **Key Management:** Schlüssel (LEK/KEK) können in HSMs (z. B. Thales Luna, Utimaco) oder **HashiCorp Vault** (vault\_key\_provider.cpp) sicher verwahrt werden.  
* **Verschlüsselung:** AES-256-GCM Feldverschlüsselung mit „Lazy Re-Encryption“ ermöglicht Key-Rotation ohne Downtime.

### **3.3 Revisionssicheres Audit-Log**

Das Audit-System (src/utils/audit\_logger.cpp) nutzt eine **kryptografische Hash-Chain**.3 Jeder Log-Eintrag enthält den Hash des vorherigen Eintrags. Eine nachträgliche Manipulation der Logs würde die Kette brechen und sofort erkannt werden. Dies ist essenziell für die Beweissicherheit von Verwaltungsakten.

---

## **4\. Komparative Marktanalyse: Build vs. Buy**

Der Vergleich mit Hyperscalern verdeutlicht die unterschiedlichen Schwerpunkte: **Konsistenz & Lokalität** (Themis) vs. **Skalierung & Services** (Cloud).

### **4.1 Vergleichstabelle**

| Merkmal | ThemisDB (Build / Converged) | AWS (Buy / Polyglot) | Google Cloud (Buy / Converged) | Azure (Buy / Hybrid) |
| :---- | :---- | :---- | :---- | :---- |
| **Architektur** | **Single Binary (RocksDB)** | Föderiert (Neptune \+ OpenSearch) | Distributed SQL (Spanner) | Multi-Model (Cosmos DB) |
| **Konsistenz** | **Stark (ACID)** lokal | Eventual (Saga nötig) | **Stark (External Consistency)** | Wählbar (Bounded Staleness) |
| **Latenz** | **Mikrosekunden** (In-Process) | Millisekunden (Netzwerk/API) | Millisekunden (Global) | Millisekunden (Netzwerk) |
| **RAG-Strategie** | **Pre-Filtering** (Native AQL) | GraphRAG Toolkit (Orchestration) | GraphRAG (Vertex AI Integration) | Hybrid Search (RRF) |
| **Sicherheit** | **BSI-Ready** (Ranger/HSM) | Cloud IAM / KMS | Cloud IAM / KMS | Entra ID |
| **Betriebsmodell** | **Local-First / Edge** | Managed Service (Sovereign Cloud) | Managed Service | Managed Service |

### **4.2 Analyse der Hyperscaler**

* **AWS:** Mit der **European Sovereign Cloud** (ESC) bietet AWS ab Ende 2025 die stärkste Compliance-Lösung . Der Stack (Neptune \+ Bedrock) ist mächtig, leidet aber unter der "Polyglot"-Architektur: Graph und Vektor sind getrennte Services, was komplexe Synchronisation (Lambda-Trigger) erfordert .  
* **Google Cloud:** **Spanner Graph** ist technologisch das einzige echte Äquivalent zu ThemisDB (SQL \+ Graph \+ Vektor in einer Engine).4 Problem: In der souveränen **T-Systems Cloud** sind diese neuesten Features oft nicht verfügbar oder verzögert .  
* **Azure:** Azure wählt einen hybriden Weg. **Cosmos DB** integriert Vektoren, und **AI Search** bietet exzellentes Reranking (RRF).6 Es bleibt jedoch eine Cloud-Lösung mit entsprechenden Latenzen und Abhängigkeiten.

---

## **5\. Strategische Implikation und Empfehlung**

Die Analyse bestätigt, dass **ThemisDB v1.0.0** kein experimenteller Prototyp mehr ist, sondern eine spezialisierte Hochleistungsdatenbank, die ein spezifisches Problem (ACID-Konsistenz für RAG) besser löst als generische Cloud-Dienste.

### **5.1 Die Lücke im Markt**

Es gibt keine andere Lösung am Markt, die **Graph, Vektor und Relational** mit **ACID-Garantien** in einer **Embedded-Library** (Local-First) vereint und gleichzeitig **BSI-konforme Sicherheit** (Ranger/HSM) bietet. Hyperscaler erzwingen Cloud-Nutzung; reine Vektor-DBs (Chroma/Milvus) fehlen Transaktionen und Graph-Logik.

### **5.2 Empfohlene Zwei-Säulen-Strategie für das VCC**

Basierend auf 1 und dem technischen Audit 3 wird folgendes Vorgehen empfohlen:

1. **Kernsystem (ThemisDB):** Einsatz von ThemisDB als **Verwaltungsprozess-Backbone (VPB)**. Die ACID-Garantie und die nun bestätigte Ranger-Integration erlauben den rechtssicheren Betrieb on-premise. Hier werden sensible Akten und der Wissensgraph gehalten.  
2. **Skalierung (AWS ESC):** Nutzung der **AWS Sovereign Cloud** als Ergänzung für *stateless* Inferenz-Workloads (LLM-Hosting via Bedrock), sobald verfügbar. Die Datenhoheit verbleibt jedoch im Themis-Kern ("Bring Your Own Data").  
3. **Migration:** Die UDS3-Architektur (Polyglot) sollte aufgrund der unbeherrschbaren Saga-Komplexität und des Risikos von Dateninkonsistenzen aktiv abgelöst werden.

**Fazit:** ThemisDB ermöglicht eine **„Sovereign AI“**, die physikalisch im Rechenzentrum der Verwaltung läuft, aber funktionale Parität zu Cloud-RAG-Systemen bietet – bei überlegener Konsistenz und Latenz.

#### **Referenzen**

1. Strategische Gesamtanalyse: ThemisDB als ACID-Fundament für die RAG-LLM-Strategie der deutschen Verwaltung – Eine komparative Analyse zur Ablösung der UDS3-Architektur  
2. AWS vs Azure vs Google Cloud: The Ultimate 2025 Comparison Guide \- Pilotcore, Zugriff am November 30, 2025, [https://pilotcore.io/blog/aws-vs-azure-vs-google-cloud-comparison](https://pilotcore.io/blog/aws-vs-azure-vs-google-cloud-comparison)  
3. ThemisDB-Documentation.pdf  
4. Spanner Graph: Reveal relationships in your data \- Google Cloud, Zugriff am November 30, 2025, [https://cloud.google.com/products/spanner/graph](https://cloud.google.com/products/spanner/graph)  
5. Spanner Graph overview \- Google Cloud Documentation, Zugriff am November 30, 2025, [https://docs.cloud.google.com/spanner/docs/graph/overview](https://docs.cloud.google.com/spanner/docs/graph/overview)  
6. Hybrid query \- Azure AI Search \- Microsoft Learn, Zugriff am November 30, 2025, [https://learn.microsoft.com/en-us/azure/search/hybrid-search-how-to-query](https://learn.microsoft.com/en-us/azure/search/hybrid-search-how-to-query)  
7. AI Knowledge Graphs \- Azure Cosmos DB \- Microsoft Learn, Zugriff am November 30, 2025, [https://learn.microsoft.com/en-us/azure/cosmos-db/gen-ai/cosmos-ai-graph](https://learn.microsoft.com/en-us/azure/cosmos-db/gen-ai/cosmos-ai-graph)