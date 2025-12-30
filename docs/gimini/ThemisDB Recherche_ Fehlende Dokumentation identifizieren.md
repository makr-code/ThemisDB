# **Technischer Forschungsbericht: Architektur- und Implementierungsanalyse von ThemisDB**

## **1\. Executive Summary**

Die vorliegende Analyse widmet sich der detaillierten technischen Untersuchung von ThemisDB, einem hochperformanten Multi-Modell-Datenbanksystem, das relationale, graphbasierte, vektororientierte, zeitreihenbezogene und geospatial-Datenmodelle in einer einzigen, kohärenten Architektur vereint. In einer Ära, die zunehmend von der Fragmentierung der Dateninfrastruktur ("Polyglot Persistence") geprägt ist, verfolgt ThemisDB den gegenteiligen Ansatz der Konvergenz. Durch die Nutzung eines einheitlichen "Base Entity"-Speicherparadigmas auf Basis einer transaktionalen RocksDB-Engine adressiert das System die inhärenten Konsistenzprobleme verteilter Architekturen.1

Dieser Bericht beleuchtet die tiefgreifenden architektonischen Entscheidungen, die ThemisDB definieren. Von der Implementierung eines persistierten HNSW-Index (Hierarchical Navigable Small World) für Vektorsuchen, der die Lücke zwischen flüchtigem RAM-Speicher und dauerhafter Persistenz schließt, bis hin zur Integration von Gorilla-Kompressionsalgorithmen für hochfrequente Zeitreihendaten.1 Ein besonderer Fokus liegt auf der "Advanced Query Language" (AQL), die es ermöglicht, hybride Abfragen – beispielsweise die Kombination einer semantischen Vektorsuche mit relationalen Filtern und Graphtraversierungen – in einem einzigen Ausführungsplan zu verarbeiten.1

Darüber hinaus evaluiert der Bericht die "Enterprise Readiness" des Systems, einschließlich der Implementierung von fortgeschrittenen Governance-Mechanismen wie spaltenbasierter Verschlüsselung (Column-Level Encryption), rollenbasierter Zugriffskontrolle (RBAC) und eIDAS-konformer PKI-Integration.1 Die Analyse der Skalierbarkeitsfunktionen, wie adaptive Load Shedding und Token-Bucket-Rate-Limiting, unterstreicht die Positionierung von ThemisDB als robuste Plattform für datenintensive Anwendungen im Unternehmensumfeld. Die Untersuchung stützt sich auf eine umfassende Auswertung der internen Dokumentation, Quellcode-Strukturen und Benchmark-Daten.

## **2\. Systemarchitektur und Kern-Engine**

Die Architektur von ThemisDB ist durch einen monolithischen Kern mit modularer Erweiterbarkeit gekennzeichnet. Anstatt verschiedene Engines lose zu koppeln, integriert ThemisDB spezialisierte Indizes tief in den Speicher- und Transaktionslayer.

### **2.1 Unified Storage Architecture**

Das Fundament von ThemisDB bildet eine vereinheitlichte Speicherschicht, die auf **RocksDB** als Storage-Engine aufsetzt. RocksDB, ursprünglich von Facebook entwickelt und auf LevelDB basierend, bietet eine Log-Structured Merge-Tree (LSM) Struktur, die für schreibintensive Workloads optimiert ist.1 ThemisDB nutzt spezifisch die TransactionDB-Variante von RocksDB, um ACID-Garantien über alle Datenmodelle hinweg zu gewährleisten.

#### **2.1.1 Das "Base Entity" Paradigma**

Zentral für die Datenhaltung ist das Konzept der "Base Entity". Anstatt Dokumente, Knoten oder Zeitreihenpunkte in isolierten Formaten zu speichern, normalisiert ThemisDB diese in ein kanonisches binäres Blob-Format.1

* **Serialisierung:** Die Daten werden mittels **VelocyPack** oder einem vergleichbaren hochperformanten Binärformat (wie Bincode) serialisiert. VelocyPack bietet gegenüber JSON den Vorteil, dass es kompakt ist und direkten Zugriff auf Unterelemente erlaubt, ohne das gesamte Dokument parsen zu müssen – ein entscheidender Vorteil für Filteroperationen in der Storage-Engine.1  
* **Key-Value-Mapping:** Jede Entität wird unter einem Primärschlüssel (Primary Key, PK) im LSM-Tree abgelegt. Das Schema folgt typischerweise dem Muster table:primary\_key \-\> Blob.1 Dies ermöglicht extrem schnelle Punktzugriffe (Point Lookups).

#### **2.1.2 Speicher-Hierarchie und Caching**

Die Performance von ThemisDB wird maßgeblich durch eine sorgfältig abgestimmte Speicherhierarchie bestimmt, die Latenzen minimiert und den Durchsatz maximiert:

1. **Write-Ahead Log (WAL):** Alle Schreiboperationen werden zunächst sequenziell in das WAL auf NVMe-SSDs geschrieben. Dies garantiert Persistenz (Durability) bei minimaler Latenz, da wahlfreie Zugriffe vermieden werden.1  
2. **Memtable:** Daten werden in einer In-Memory-Datenstruktur (Memtable, typischerweise eine SkipList) gepuffert. ThemisDB konfiguriert hier standardmäßig 256 MB pro Memtable, wobei mehrere Memtables (immutable und mutable) parallel existieren können, um Write-Stalls während des Flush-Vorgangs zu verhindern.1  
3. **Block Cache:** Für Leseoperationen unterhält ThemisDB einen Block Cache im RAM (Standard: 1 GB, LRU-Eviction). Dieser Cache speichert dekomprimierte Datenblöcke, um die CPU-Last für wiederholte Zugriffe zu reduzieren.1  
4. **SSTables (Sorted String Tables):** Sobald ein Memtable voll ist, wird er als SSTable auf die Festplatte (SSD) geflusht. Diese Dateien sind unveränderlich und nach Schlüsseln sortiert.

#### **2.1.3 Kompressionsstrategie**

ThemisDB implementiert eine hybride Kompressionsstrategie, um das Gleichgewicht zwischen Speicherplatz und CPU-Last zu optimieren 1:

* **LZ4 (Level 0-5):** In den oberen Ebenen des LSM-Trees, wo Daten häufiger gelesen und kompaktiert werden, kommt LZ4 zum Einsatz. LZ4 bietet extrem hohe Dekompressionsgeschwindigkeiten, was die Latenz bei Lesezugriffen auf "heiße" Daten minimiert.1  
* **ZSTD (Bottommost Level):** Auf der untersten Ebene (L6), die den Großteil der Datenmenge (oft \>90%) enthält und selten kompaktiert wird, verwendet ThemisDB Zstandard (ZSTD). ZSTD bietet eine deutlich höhere Kompressionsrate als LZ4, was die Speicherkosten für archivierte Daten signifikant senkt, bei immer noch akzeptabler Dekompressionsgeschwindigkeit.1

### **2.2 MVCC und Transaktionsmanagement**

Die Unterstützung von **Multi-Version Concurrency Control (MVCC)** ist ein entscheidendes Merkmal von ThemisDB, das es von simplen Key-Value-Stores unterscheidet. MVCC ermöglicht "Snapshot Isolation", was bedeutet, dass Leseoperationen einen konsistenten Zustand der Datenbank zu einem bestimmten Zeitpunkt sehen, selbst wenn parallel Schreiboperationen stattfinden.1

* **Implementierung:** RocksDB implementiert MVCC durch das Anhängen von Sequenznummern an jeden Schlüssel. Wenn ein Datensatz aktualisiert wird, wird eine neue Version mit einer höheren Sequenznummer geschrieben, anstatt die alte zu überschreiben. Alte Versionen werden erst während des Compaction-Prozesses entfernt, wenn sie von keinem aktiven Snapshot mehr benötigt werden.1  
* **Konflikterkennung:** Der TransactionManager von ThemisDB nutzt pessimistisches Locking oder Optimistic Concurrency Control (abhängig von der Konfiguration), um Write-Write-Konflikte zu erkennen. Versuchen zwei Transaktionen denselben Schlüssel zu ändern, wird einer der Vorgänge abgebrochen, um die Datenintegrität zu wahren.1  
* **Atomarität über Indizes hinweg:** Ein kritischer Aspekt der Architektur ist die Garantie, dass Updates an der Base Entity und den zugehörigen Indizes (Vektor, Graph, Sekundärindex) atomar erfolgen. Dies wird durch die Nutzung von RocksDB WriteBatch und Transaktionen erreicht. Entweder werden alle Indexeinträge und das Dokument aktualisiert, oder keines.1

### **2.3 Threading-Modell und Parallelisierung**

Um moderne Multi-Core-CPUs effizient auszulasten, setzt ThemisDB auf **Intel Threading Building Blocks (TBB)** für das Task-Management.1

* **Task-Based Execution:** Anstatt für jede Anfrage einen dedizierten OS-Thread zu blockieren (was zu hohem Context-Switching-Overhead führen kann), zerlegt ThemisDB Anfragen in kleine, unabhängige Tasks. Ein Pool von Worker-Threads arbeitet diese Tasks mittels eines "Work-Stealing"-Schedulers ab. Dies sorgt für eine exzellente Lastverteilung über alle CPU-Kerne.1  
* **Batch-Processing:** Ingest- und Query-Pfade sind auf Batch-Verarbeitung optimiert. Das System erkennt automatisch Möglichkeiten zur Parallelisierung. Beispielsweise werden beim Laden von Entitäten Batches gebildet (Standardgröße: 50, Schwellenwert: 100), die parallel von RocksDB abgerufen werden. Dies amortisiert die Kosten für Funktionsaufrufe und Synchronisation und führt zu einem berichteten Durchsatz von bis zu 120.000 Reads pro Sekunde.1

## **3\. Datenmodelle und Indexierungsstrategien**

Die wahre Stärke von ThemisDB liegt in der Fähigkeit, unterschiedliche Datenparadigmen nicht nur zu speichern, sondern sie durch spezialisierte Indizes effizient abfragbar zu machen.

### **3.1 Relationales Modell und Sekundärindizes**

Obwohl ThemisDB schemalos ist, unterstützt es relationale Zugriffsmuster durch explizite Sekundärindizes.

* **Index-Typen:**  
  * **Equality Index:** Hash-basierte oder sortierte Indizes für exakte Übereinstimmungen.  
  * **Range Index:** Ermöglicht Bereichsabfragen (\>, \<, BETWEEN) und Sortierung.  
  * **Composite Index:** Indizes über mehrere Felder für komplexe Filterkriterien.  
  * **Sparse Index:** Indiziert nur Dokumente, die das betreffende Feld enthalten, was Speicherplatz spart.  
  * **TTL Index:** Ermöglicht das automatische Löschen von Daten nach einer definierten Zeitspanne (Time-To-Live), ideal für Caching oder Session-Daten.1  
* **Speicherlayout:** Sekundärindizes werden in RocksDB als separate Key-Value-Paare abgebildet. Ein typisches Schema für einen Indexeintrag ist idx:\<table\>:\<column\>:\<value\>:\<pk\>. Durch dieses Prefix-Schema liegen alle Einträge für eine bestimmte Spalte und einen bestimmten Wert im LSM-Tree nacheinander, was extrem effiziente Range-Scans (Seek \+ Next) ermöglicht.1

### **3.2 Graph-Modell (Labeled Property Graph)**

ThemisDB implementiert ein natives Graph-Modell, bei dem Knoten und Kanten (Edges) als First-Class-Entities behandelt werden. Dies unterscheidet es von Systemen, die Graphen nur simulieren.1

* **Adjazenz-Indexierung:** Um schnelle Traversierungen zu ermöglichen, ohne auf langsame JOIN-Operationen zurückgreifen zu müssen, materialisiert ThemisDB die Graph-Topologie in zwei Richtungen:  
  * **Outdex:** Speichert ausgehende Kanten (graph:out:\<from\_pk\>:\<edge\_id\> \-\> \<to\_pk\>).  
  * Indeg: Speichert eingehende Kanten (graph:in:\<to\_pk\>:\<edge\_id\> \-\> \<from\_pk\>).  
    Diese Struktur erlaubt es, Nachbarschaftsabfragen in $O(1)$ (bzw. $O(k)$ für $k$ Nachbarn) durchzuführen, unabhängig von der Gesamtgröße des Graphen.1  
* **Traversierungs-Algorithmen:** Die Engine implementiert Standardalgorithmen wie **BFS** (Breadth-First Search) und **Dijkstra** für Kürzeste-Pfade-Probleme. Zusätzlich wird **A\*** (A-Star) unterstützt, was heuristische Optimierungen bei der Pfadsuche ermöglicht.  
* **RAM-Optimierte Topologie:** Für performancekritische Graphen lädt ThemisDB die Topologie (Knoten-IDs und Kantenbeziehungen) in einen optimierten In-Memory-Cache. Dies eliminiert Disk-I/O während der Traversierung fast vollständig und ermöglicht komplexe Multi-Hop-Abfragen in Millisekunden.1

### **3.3 Vektor-Modell und Semantische Suche**

Für KI-Anwendungen integriert ThemisDB eine Vektordatenbank-Engine, die direkt im Kern verankert ist.1

* **HNSW (Hierarchical Navigable Small World):** Der Vektorindex basiert auf dem HNSW-Algorithmus. HNSW ist der aktuelle Industriestandard für ANN (Approximate Nearest Neighbor) Suche, da er einen hervorragenden Kompromiss zwischen Suchgeschwindigkeit und Genauigkeit (Recall) bietet. Er baut einen mehrschichtigen Graphen auf, der es erlaubt, sich logarithmisch der Zielregion im hochdimensionalen Raum zu nähern.1  
* **Persistenz und Warm Start:** Ein bekanntes Problem von HNSW ist der hohe Aufwand beim Aufbau des Index und dessen Flüchtigkeit (Memory-Only). ThemisDB löst dies durch eine Persistenzschicht. Beim Herunterfahren wird der HNSW-Graph (Knoten, Links, Layer) serialisiert auf die Festplatte geschrieben. Beim Start wird dieser Zustand direkt geladen ("Warm Start"), anstatt den Index neu aufbauen zu müssen. Dies reduziert die Startzeit bei großen Datensätzen von Stunden auf Sekunden.1  
* **Metriken:** Unterstützt werden Euklidische Distanz (L2), Kosinus-Ähnlichkeit (Cosine) und Skalarprodukt (Dot Product), konfigurierbar pro Index.1

### **3.4 Zeitreihen-Modell (Time-Series)**

Das Zeitreihen-Modul ist auf die effiziente Speicherung und Abfrage von Metriken und Sensordaten ausgelegt.1

* **Gorilla Kompression:** ThemisDB implementiert den von Facebook vorgestellten Gorilla-Algorithmus.  
  * **Zeitstempel:** Da Messdaten oft in regelmäßigen Intervallen eintreffen, wird nicht der volle Zeitstempel (64 Bit) gespeichert, sondern das Delta zum vorherigen Zeitstempel, und davon wiederum das Delta (Delta-of-Delta). Wenn das Intervall konstant ist, ist das Delta-of-Delta 0 und kann mit einem einzigen Bit kodiert werden.  
  * Werte: Fließkommazahlen (Values) werden mittels XOR mit dem vorherigen Wert verknüpft. Da sich physikalische Messwerte oft nur langsam ändern, resultiert das XOR in vielen führenden und folgenden Nullen, die effizient komprimiert werden können.  
    Dies führt zu Kompressionsraten von 10-20x, was den Speicherbedarf massiv reduziert und mehr Daten im Cache hält.1  
* **Continuous Aggregates:** Um Abfragen über große Zeiträume zu beschleunigen, berechnet ThemisDB kontinuierlich Aggregate (Summe, Mittelwert, Min, Max) in definierten Fenstern (z.B. 1 Minute, 1 Stunde). Diese vorberechneten Rollups werden gespeichert, sodass eine Abfrage über ein Jahr nicht Millionen von Rohdatenpunkten, sondern nur wenige Tausend Aggregate scannen muss.1

### **3.5 Geospatial-Modell**

Geodaten werden nicht als isoliertes Modell, sondern als querschnittliche Fähigkeit implementiert.1

* **R-Tree Index:** Zur Indexierung von zweidimensionalen Daten (Längengrad, Breitengrad) wird ein R-Tree verwendet. Dieser organisiert Daten in hierarchischen Bounding Boxes, was effiziente Bereichsabfragen (ST\_Within, ST\_Intersects) ermöglicht.  
* **Funktionalität:** ThemisDB unterstützt den OGC-Standard (Open Geospatial Consortium) mit Funktionen wie ST\_Point, ST\_Distance, ST\_Contains und dem Import von GeoJSON und WKB/EWKB Formaten.1

## **4\. Query Engine und AQL (Advanced Query Language)**

Die Schnittstelle zwischen dem Anwender und den komplexen Datenmodellen bildet AQL. Sie ist deklarativ, SQL-ähnlich und mächtig genug, um relationale Joins, Graphtraversierungen und Vektorsuchen zu kombinieren.1

### **4.1 AQL Syntax und Fähigkeiten**

Die Syntax orientiert sich an etablierten Standards, erweitert diese aber gezielt.

* **Basis-Operationen:** FOR (Iteration), FILTER (Selektion), SORT, LIMIT, RETURN (Projektion).  
* **Graph Traversierung:**  
  Code-Snippet  
  FOR v, e, p IN 1..3 OUTBOUND "users/alice" GRAPH "social"  
  RETURN p

  Dieser Befehl startet bei "users/alice" und folgt ausgehenden Kanten im Graphen "social" für 1 bis 3 Hops. v repräsentiert den Knoten, e die Kante und p den gesamten Pfad.1  
* **Hybride Suche (Vektor \+ Volltext):**  
  Code-Snippet  
  FOR doc IN documents  
  SEARCH PHRASE(doc.text, "database") OR VECTOR\_DISTANCE(doc.embedding, @query\_vec) \< 0.5  
  RETURN doc

  Hier wird eine Volltextsuche (SEARCH PHRASE) mit einer semantischen Ähnlichkeitssuche (VECTOR\_DISTANCE) kombiniert. Dies ist ein Paradebeispiel für die Multi-Modell-Fähigkeit, bei der lexikalische und semantische Relevanz fusioniert werden.1  
* **Komplexe Analytik:** Mit COLLECT (ähnlich SQL GROUP BY), Aggregatfunktionen (COUNT, SUM, AVG) und Subqueries (auch korrelierte) können komplexe analytische Fragestellungen direkt in der Datenbank gelöst werden. Die Unterstützung von Common Table Expressions (CTEs) via WITH verbessert die Lesbarkeit und Modularität komplexer Abfragen.1

### **4.2 Query Optimizer**

Der Query Optimizer ist das "Gehirn", das entscheidet, wie eine Abfrage ausgeführt wird.

* **Cost-Based Optimization (CBO):** Der Optimizer nutzt Statistiken über die Datenverteilung (Kardinalität, Selektivität), um den günstigsten Ausführungsplan zu erstellen.  
* **Index-Auswahl:** Er entscheidet intelligent, welcher Index genutzt werden soll. Bei einer Abfrage mit Geo-Filter und Vektor-Suche prüft er, welcher Filter selektiver ist. Ist der Geo-Filter sehr strikt (z.B. "im Umkreis von 10m"), wird dieser zuerst angewendet, um die Kandidatenmenge für die teure Vektorberechnung drastisch zu reduzieren.1  
* **Predicate Pushdown:** Filterbedingungen werden so nah wie möglich an die Datenquelle (RocksDB) geschoben. Dies minimiert die Datenmenge, die in den Speicher geladen und durch die Engine verarbeitet werden muss.1

### **4.3 Hybride Ausführungsstrategien**

ThemisDB verwendet fortschrittliche Strategien für hybride Abfragen:

* **Pre-Filtering für Vektorsuche:** Anstatt erst alle ähnlichen Vektoren zu finden und dann zu filtern (Post-Filtering, was bei selektiven Filtern oft zu wenigen oder keinen Ergebnissen führt), nutzt ThemisDB Pre-Filtering. Dabei wird erst die Menge der zulässigen Dokument-IDs (z.B. durch einen relationalen Index) bestimmt und als Bitmaske an den HNSW-Index übergeben. Der HNSW-Algorithmus ignoriert dann während der Traversierung alle Knoten, die nicht in der Bitmaske enthalten sind.1  
* **Apache Arrow Integration:** Für OLAP-Workloads ist eine Integration mit Apache Arrow vorhanden. Dies ermöglicht es, Abfrageergebnisse in einem spaltenorientierten In-Memory-Format bereitzustellen, das ideal für die Weiterverarbeitung in Data-Science-Tools (wie Pandas oder Spark) geeignet ist, ohne teure Serialisierungskosten.1

## **5\. Content Pipeline und Ingestion**

Um unstrukturierte Daten in die strukturierte Welt der Datenbank zu überführen, bietet ThemisDB eine integrierte "Content Pipeline".1

### **5.1 Architektur der Ingestion Pipeline**

Die Pipeline ist als "Unified Ingestion Pipeline" konzipiert. Sie nimmt rohe Dateien entgegen und routet sie basierend auf ihrem MIME-Type an spezialisierte Prozessoren.1

* **JSON Ingestion Spec:** Eine formale Spezifikation definiert, wie Daten und Metadaten übergeben werden müssen. Dies entkoppelt den Client von der internen Verarbeitungslogik.1

### **5.2 Spezialisierte Prozessoren**

* **Image Processor:**  
  * Extrahiert EXIF-Metadaten (Kameramodell, Zeitstempel, GPS) und speichert diese als durchsuchbare Felder.  
  * Erzeugt Thumbnails für die Vorschau.  
  * **Chunking:** Implementiert eine Tiling-Strategie (z.B. 3x3 Raster), um große Bilder in kleinere Segmente zu zerlegen. Diese Segmente können einzeln vektorisiert werden, was eine fein-granulare Bildsuche ermöglicht.1  
* **Geo Processor:**  
  * Verarbeitet GeoJSON und GPX-Dateien.  
  * Normalisiert Koordinaten und berechnet Bounding Boxes.  
  * Zerlegt komplexe Pfade (z.B. GPX-Tracks) in Segmente, um sie effizient im R-Tree indexieren zu können.1

## **6\. Sicherheitsarchitektur, Governance und Compliance**

Sicherheit ist in ThemisDB tief verwurzelt und nicht nur ein Aufsatz. Dies ist besonders relevant für den Einsatz in regulierten Branchen (Finanz, Gesundheit, Regierung).

### **6.1 Verschlüsselung (Encryption)**

ThemisDB verfolgt eine "Defense-in-Depth"-Strategie bei der Verschlüsselung.1

* **Column-Level Encryption (Spaltenverschlüsselung):** Sensible Felder (z.B. Kreditkartennummern, medizinische Daten) werden individuell verschlüsselt. Dies geschieht transparent für die Anwendung, aber die Daten liegen verschlüsselt auf der Platte (Data-at-Rest). Verwendet wird typischerweise **AES-256-GCM** für authentifizierte Verschlüsselung.  
* **Key Management:** Das System unterstützt eine Hierarchie von Schlüsseln:  
  * **DEK (Data Encryption Key):** Verschlüsselt die eigentlichen Daten.  
  * **KEK (Key Encryption Key):** Verschlüsselt die DEKs.  
  * **Provider:** Schlüssel können aus externen Quellen wie **HashiCorp Vault** oder **HSMs** (Hardware Security Modules) via PKCS\#11 bezogen werden. Für Entwicklungsumgebungen gibt es MockKeyProvider.1  
* **Lazy Re-Encryption:** Bei einer Schlüsselrotation muss nicht die gesamte Datenbank entschlüsselt und neu verschlüsselt werden. Stattdessen wird oft nur der KEK rotiert (Key Wrapping). Die Daten selbst werden erst beim nächsten Schreibvorgang mit dem neuen Schlüsselversion neu verschlüsselt.1

### **6.2 Rollenbasierte Zugriffskontrolle (RBAC)**

Das RBAC-System ist fein-granular und konfigurierbar.1

* **Rollen:** Standardrollen wie admin, operator, analyst, readonly.  
* **Berechtigungen:** Ressourcen-basierte Permissions wie data:read, data:write, keys:rotate, audit:view. Wildcards (data:\*) erleichtern die Verwaltung.  
* **Policy Engine:** Eine interne Engine prüft bei jedem Zugriff die Berechtigungen. Die Konfiguration kann über JSON/YAML erfolgen und wird zur Laufzeit durchgesetzt.1

### **6.3 Audit und Compliance (eIDAS & GDPR)**

* **Audit Logging:** Alle sicherheitsrelevanten Ereignisse (Login, Datenzugriff, Schemaänderung) werden in einem manipulationssicheren Log ("Tamper-Proof") aufgezeichnet. Eine Hash-Chain (ähnlich einer Blockchain) verknüpft die Log-Einträge, sodass nachträgliche Löschungen oder Änderungen erkennbar wären. Das Format ist JSON, optimiert für SIEM-Systeme (Splunk, ELK).1  
* **PII Detection:** Eine integrierte Engine scannt eingehende Daten mittels Regex und Mustererkennung auf PII (Personally Identifiable Information) wie E-Mail-Adressen, IBANs oder Sozialversicherungsnummern. Dies unterstützt die Einhaltung der DSGVO (GDPR) durch automatisches Tagging oder Verschlüsseln.1  
* **PKI & eIDAS:** ThemisDB integriert eine PKI-Schnittstelle, die qualifizierte elektronische Signaturen gemäß eIDAS-Verordnung unterstützt. Dies ermöglicht die rechtssichere Archivierung von Dokumenten und Transaktionen. Die Implementierung nutzt OpenSSL für RSA-Signaturen und kann Zertifikate verwalten und validieren.1

## **7\. Enterprise Skalierbarkeit und Betrieb**

Für den Betrieb in geschäftskritischen Umgebungen bietet ThemisDB Funktionen, die Stabilität und Kontrolle unter Last garantieren.1

### **7.1 Traffic Management und Schutz**

* **Rate Limiting (Token Bucket):** Um Überlastung und Missbrauch zu verhindern, implementiert ThemisDB einen **Token Bucket** Algorithmus. Im Gegensatz zum "Leaky Bucket", der den Durchfluss starr glättet, erlaubt der Token Bucket kurzzeitige Lastspitzen (Bursts), solange Tokens im "Eimer" sind. Dies ist ideal für API-Gateways, die menschliches Nutzerverhalten (klick-intensiv) abbilden müssen. Die Implementierung unterstützt Priorisierung (High/Normal/Low Priority Lanes) und ist Thread-safe.1  
* **Adaptive Load Shedding:** Wenn das System an seine Grenzen stößt (z.B. CPU \> 95%, Speicher voll), greift der Load Shedder ein. Er verwirft proaktiv Anfragen mit niedriger Priorität, um das System vor dem Kollaps zu bewahren und sicherzustellen, dass kritische Anfragen (z.B. Health-Checks, Admin-Befehle) weiterhin bedient werden können. Dies realisiert das Prinzip der "Graceful Degradation".1

### **7.2 Observability (Beobachtbarkeit)**

* **Prometheus Metrics:** Ein /metrics Endpunkt liefert detaillierte Metriken im Prometheus-Format. Dazu gehören technische Metriken (RocksDB Cache Hits, Compaction Bytes, Memtable Size) sowie Business-Metriken (QPS, Latenz-Histogramme p50/p99, Fehlerraten).1  
* **Distributed Tracing:** Durch die Integration von **OpenTelemetry** können Anfragen über Systemgrenzen hinweg verfolgt werden. Dies ist essentiell, um Latenztreiber in komplexen Microservices-Architekturen zu identifizieren. ThemisDB instrumentiert kritische Pfade wie HTTP-Handler und die Query-Engine.1

### **7.3 Skalierung**

* **Sharding:** ThemisDB unterstützt horizontales Skalieren durch Auto-Sharding. Daten werden basierend auf Shard-Keys auf mehrere Knoten verteilt.  
* **HTTP Client Pool:** Für externe Anfragen (z.B. an LLM-Provider) nutzt ThemisDB einen HTTP Client Pool basierend auf **Boost.Beast**. Connection Pooling (Wiederverwendung von TCP/TLS-Verbindungen) reduziert den Overhead drastisch und steigert den Durchsatz für externe Integrationen.1

## **8\. Entwicklungsstatus und Roadmap**

Eine Analyse des Entwicklungsfortschritts zeigt ein System im Übergang von der Core-Entwicklung zur Enterprise-Härtung.

### **8.1 Abgeschlossene Meilensteine (Stand Q4 2025\)**

* **Core Storage:** MVCC, RocksDB-Integration und Base-Entity-Format sind vollständig implementiert und stabil.1  
* **Indizes:** HNSW-Vektorindex (inkl. Persistenz), Graph-Indizes und Sekundärindizes sind produktionsreif.1  
* **Query:** AQL-Parser und grundlegende Execution-Engine stehen. Hybride Queries sind möglich.1  
* **Security:** Verschlüsselung, RBAC und Audit-Logging sind implementiert.1  
* **Observability:** Prometheus-Metriken und Tracing sind integriert.1

### **8.2 In Entwicklung / Offen**

* **Inkrementelle Backups:** Die Funktionalität für Point-in-Time-Recovery via WAL-Archivierung ist spezifiziert, aber noch in Arbeit.1  
* **Erweiterte Analytik:** Die Integration von Apache Arrow für tiefgehende OLAP-Analysen ist geplant, aber noch nicht vollständig realisiert.1  
* **Cluster-Features:** Fortgeschrittene Cluster-Funktionen wie automatisches Rebalancing und Multi-Datacenter-Replikation stehen auf der Roadmap für 2026\.1

## **9\. Schlussfolgerung**

ThemisDB präsentiert sich als technologisch hochentwickelte Lösung für das Problem der Datenfragmentierung. Durch die Verschmelzung von Graph-, Vektor- und relationalen Modellen auf einer transaktionalen RocksDB-Basis bietet es eine Antwort auf die Anforderungen moderner KI-Anwendungen (z.B. RAG), die Kontext (Graph), Bedeutung (Vektor) und Fakten (Relational) gleichzeitig benötigen.

Die Architekturentscheidungen – insbesondere die Persistierung von HNSW-Indizes, die Nutzung von Gorilla-Kompression und die tiefe Integration von Sicherheitsfeatures wie eIDAS-Signaturen – zeugen von einem Fokus auf reale Enterprise-Anforderungen. Während einige fortgeschrittene Skalierungsfunktionen noch reifen, ist der Core-Engine-Status beeindruckend und positioniert ThemisDB als ernstzunehmende Alternative zu komplexen Multi-Datenbank-Setups.

## ---

**10\. Anhang: Technische Referenzdaten**

### **10.1 Performance-Ziele und Benchmarks**

Basierend auf internen Dokumentationen zielt ThemisDB auf folgende Leistungswerte auf Standard-Hardware (z.B. i7-12700K) ab:

| Operation | Durchsatz-Ziel | Latenz-Ziel (p99) | Anmerkung |
| :---- | :---- | :---- | :---- |
| **Entity Write** | 45.000 ops/sec | \< 0.15 ms | Profitier von LSM-Writes (Append-Only) |
| **Entity Read** | 120.000 ops/sec | \< 0.05 ms | Aus dem Block Cache / Memtable |
| **Vektor-Suche (ANN)** | 1.800 QPS | \< 2.1 ms | HNSW Index (k=10) |
| **Graph Traversal** | 3.200 ops/sec | \< 1.2 ms | Tiefe 3, Hot Topology Cache |

1

### **10.2 Feature-Reifegrad-Matrix**

| Modul | Status | Reife | Kommentar |
| :---- | :---- | :---- | :---- |
| **Core Storage (MVCC)** | Abgeschlossen | Production | RocksDB TransactionDB Basis stabil |
| **Vector Index (HNSW)** | Abgeschlossen | Production | Inkl. Persistenz und Warm-Start |
| **Graph Engine** | Abgeschlossen | Production | BFS/Dijkstra, Topologie-Cache aktiv |
| **Time-Series Engine** | Abgeschlossen | Production | Gorilla Kompression, Aggregates |
| **Security (RBAC/Enc)** | Abgeschlossen | Production | GDPR/eIDAS Features integriert |
| **Content Pipeline** | MVP | Beta | Prozessoren funktional, Erweiterungen geplant |
| **Sharding** | In Arbeit | Alpha | Horizontal Scaling in Entwicklung |

1

#### **Referenzen**

1. ThemisDB-Documentation.pdf  
2. Python implementation of Gorilla time series compression \- GitHub, Zugriff am Dezember 5, 2025, [https://github.com/ghilesmeddour/gorilla-time-series-compression](https://github.com/ghilesmeddour/gorilla-time-series-compression)  
3. Vector Database Basics: HNSW | Tiger Data, Zugriff am Dezember 5, 2025, [https://www.tigerdata.com/blog/vector-database-basics-hnsw](https://www.tigerdata.com/blog/vector-database-basics-hnsw)  
4. EIDAS Compliant Electronic Signature \- eSignGlobal, Zugriff am Dezember 5, 2025, [https://www.esignglobal.com/blog/eidas-compliant-electronic-signature](https://www.esignglobal.com/blog/eidas-compliant-electronic-signature)  
5. RocksDB \- Wikipedia, Zugriff am Dezember 5, 2025, [https://en.wikipedia.org/wiki/RocksDB](https://en.wikipedia.org/wiki/RocksDB)  
6. Top 7 ArangoDB Alternatives of 2025 \- PuppyGraph, Zugriff am Dezember 5, 2025, [https://www.puppygraph.com/blog/arangodb-alternatives](https://www.puppygraph.com/blog/arangodb-alternatives)  
7. arangodb/velocypack: A fast and compact format for serialization and storage \- GitHub, Zugriff am Dezember 5, 2025, [https://github.com/arangodb/velocypack](https://github.com/arangodb/velocypack)  
8. Transactions · facebook/rocksdb Wiki \- GitHub, Zugriff am Dezember 5, 2025, [https://github.com/facebook/rocksdb/wiki/Transactions](https://github.com/facebook/rocksdb/wiki/Transactions)  
9. Multiversion Concurrency Control (MVCC): A Practical Deep Dive \- CelerData, Zugriff am Dezember 5, 2025, [https://celerdata.com/glossary/multiversion-concurrency-control](https://celerdata.com/glossary/multiversion-concurrency-control)  
10. How To Choose A Graph Database: We Compare 8 Favorites \- Cambridge Intelligence, Zugriff am Dezember 5, 2025, [https://cambridge-intelligence.com/choosing-graph-database/](https://cambridge-intelligence.com/choosing-graph-database/)  
11. B+ANN: A Fast Billion-Scale Disk-based Nearest-Neighbor Index \- arXiv, Zugriff am Dezember 5, 2025, [https://arxiv.org/html/2511.15557v1](https://arxiv.org/html/2511.15557v1)  
12. P-HNSW: Crash-Consistent HNSW for Vector Databases on Persistent Memory \- MDPI, Zugriff am Dezember 5, 2025, [https://www.mdpi.com/2076-3417/15/19/10554](https://www.mdpi.com/2076-3417/15/19/10554)  
13. Compressing floating point data with Gorilla \- quanttype, Zugriff am Dezember 5, 2025, [https://quanttype.net/posts/2025-06-16-compressing-with-gorilla.html](https://quanttype.net/posts/2025-06-16-compressing-with-gorilla.html)  
14. Differences Between Whole Database and Column Encryption \- NetLib Security, Zugriff am Dezember 5, 2025, [https://netlibsecurity.com/white-papers/difference-between-whole-database-and-column-encryption/](https://netlibsecurity.com/white-papers/difference-between-whole-database-and-column-encryption/)  
15. Column Level Encryption \- CockroachDB, Zugriff am Dezember 5, 2025, [https://www.cockroachlabs.com/docs/stable/column-level-encryption](https://www.cockroachlabs.com/docs/stable/column-level-encryption)  
16. Connect to Azure Cosmos DB for NoSQL using role-based access control and Microsoft Entra ID, Zugriff am Dezember 5, 2025, [https://learn.microsoft.com/en-us/azure/cosmos-db/nosql/how-to-connect-role-based-access-control](https://learn.microsoft.com/en-us/azure/cosmos-db/nosql/how-to-connect-role-based-access-control)  
17. Token Bucket vs Leaky Bucket \- Medium, Zugriff am Dezember 5, 2025, [https://medium.com/@apurvaagrawal\_95485/token-bucket-vs-leaky-bucket-1c25b388436c](https://medium.com/@apurvaagrawal_95485/token-bucket-vs-leaky-bucket-1c25b388436c)  
18. What is Prioritized Load Shedding? \- GeeksforGeeks, Zugriff am Dezember 5, 2025, [https://www.geeksforgeeks.org/system-design/what-is-prioritized-load-shedding/](https://www.geeksforgeeks.org/system-design/what-is-prioritized-load-shedding/)  
19. Ensuring fair benchmark representation for Boost.Beast client · Issue \#3051 \- GitHub, Zugriff am Dezember 5, 2025, [https://github.com/boostorg/beast/issues/3051](https://github.com/boostorg/beast/issues/3051)