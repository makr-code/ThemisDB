

# **Architektonischer Entwurf eines hochperformanten Multi-Modell-Datenbanksystems: Eine Kernel-Level-Analyse von kanonischem Speicher, Projektionsschichten und Speicherhierarchien in C++ und Rust**

## **Teil 1: Die Architektur der „Kerndaten“: Ein einheitliches Multi-Modell-Speicherformat**

Die zentrale Anforderung ist die Definition eines „Kerndatensatzes“, der von allen vier Datenmodellen (relational, graph, vector, datei/dokument) gleichzeitig genutzt werden kann. Dies impliziert den Entwurf einer „echten“ Multi-Modell-Datenbank (MMDBMS), die über eine einheitliche Speicherschicht (unified storage layer) und eine Modellübersetzungsschicht (model translation layer) verfügt. Im Gegensatz zu Polyglot-Persistence-Ansätzen, die mehrere separate Datenbank-Engines bündeln 1, speichert eine native MMDBMS alle Datentypen in einer einzigen, kanonischen Repräsentation.8

### **1.1. Das Paradigma der „Base Entity“: Der kanonische Dokumentspeicherkern**

Die effektivste kanonische Repräsentation für disparate Datenmodelle ist das Dokumentenmodell. Führende native MMDBMS wie ArangoDB und Cosmos DB nutzen dieses Prinzip. ArangoDB speichert alle Daten, einschließlich Graphen, als JSON-Objekte 9, die intern in einem binär-optimierten Format namens VelocyPack serialisiert werden.11 Cosmos DB verwendet ein internes Atom-Record-Sequence (ARS)-Format, das Daten logisch als JSON-Dokumente projiziert.

Für den hier skizzierten Entwurf definieren wir die atomare Speichereinheit als „Base Entity“.14 Jede logische Entität – sei es eine relationale Zeile, ein Graph-Knoten, ein Vektor-Objekt oder ein Dokument – wird als ein *einziges* JSON-ähnliches Dokument (im Folgenden als „Blob“ bezeichnet) gespeichert.

Diese Wahl eines flexiblen, binär-optimierten Dokumentenformats ist der entscheidende architektonische Enabler, der alle vier Modelle in einer einzigen Struktur vereint:

1. **Relationale Daten:** Eine Zeile aus einer Tabelle users wird als flaches JSON-Objekt gespeichert (z. B. {"id": 123, "name": "Alice", "age": 30}).  
2. **Dokumenten-Daten:** Werden nativ als verschachtelte JSON-Strukturen gespeichert, wie von ArangoDB demonstriert.9  
3. **Graph-Daten:** Das Labeled Property Graph (LPG)-Modell 23 wird abgebildet, indem Knoten und Kanten als separate Dokumente behandelt werden. Ein Knoten ist ein Dokument (z. B. {"id": "user/123", "name": "Alice"}). Eine Kante ist ebenfalls ein Dokument, das die Verweise \_from und \_to enthält (z. B. {"\_from": "user/123", "\_to": "company/456", "role": "developer"}).23  
4. **Vektor-Daten:** Das Vektor-Embedding wird als ein Attribut (typischerweise ein Array von Floats) innerhalb des „Base Entity“-Dokuments gespeichert (z. B. {"id": "doc/789", "text": "...", "embedding": \[0.12, 0.45,..., 0.91\]}).24

Die "Kerndaten" sind somit definiert als ein **binär-serialisiertes Dokument (Blob)** pro logischer Entität.

### **1.2. Die physische Speicher-Engine: Das Log-Structured-Merge-Tree (LSM-Tree) Fundament**

Diese „Base Entity“-Blobs müssen physisch auf einem persistenten Medium gespeichert werden. Die modernste und leistungsfähigste Architektur für schreibintensive Workloads ist eine eingebettete Key-Value-Storage-Engine (KV-Store), die auf einem Log-Structured-Merge-Tree (LSM-Tree) basiert.25

Systeme wie ArangoDB 27, CockroachDB und viele andere nutzen RocksDB als ihre zugrundeliegende Speicher-Engine.28 RocksDB ist eine in C++ geschriebene, hochperformante LSM-Tree-Bibliothek, die für schnelle Speichermedien (insbesondere SSDs) optimiert ist.30 Sie speichert beliebige Byte-Ströme als Schlüssel und Werte.30

Das physische Speicherschema für die „Base Entity“ ist daher wie folgt:

* **Key:** Der Primärschlüssel (PK) der „Base Entity“ (z. B. eine UUID oder ein String wie user/123).  
* **Value:** Das vollständige, binär-serialisierte „Base Entity“-Dokument (der "Blob").

Diese Wahl hat fundamentale Auswirkungen auf die CRUD-Leistung:

* **Create / Update (Schreiben):** LSM-Trees sind von Natur aus "append-only". Ein Create oder Update ist ein extrem schneller, sequentieller Schreibvorgang in eine In-Memory-Struktur (das *Memtable*), die später auf die SSD "gefusht" wird.33 Dies *maximiert* die Schreibleistung (hoher C/U-Durchsatz).  
* **Delete:** Eine Löschung ist ebenfalls ein schneller Schreibvorgang, bei dem ein „Tombstone-Marker“ (Löschmarkierung) gesetzt wird.33  
* **Read (Lesen):** Hier liegt die inhärente Schwäche dieser Architektur. Ein einfacher Punktabruf über den Primärschlüssel (Get(PK)) ist schnell. Eine Abfrage, die Filter auf Attribute anwendet (z. B. SELECT \* FROM users WHERE age \> 30), ist jedoch *katastrophal* langsam. Sie würde einen Full-Scan aller „Base Entity“-Blobs in der users-Tabelle erfordern, wobei jeder einzelne Blob von der Festplatte gelesen, deserialisiert und gefiltert werden müsste.

Die Entscheidung für eine LSM-Tree-Architektur zur Speicherung von Blobs maximiert die Schreibleistung auf Kosten der Leseleistung. Dies *erzwingt* architektonisch die Notwendigkeit der „Layer“ (Teil 2), die als leseschnelle Indexprojektionen dienen, um diesen Nachteil auszugleichen.17

### **1.3. Die Parsing-Herausforderung: Serialisierung und On-the-Fly-Extraktion**

Da die „Layer“ (Indizes) aus den „Kerndaten“ (Blobs) erstellt werden müssen, wird die Geschwindigkeit der Deserialisierung des Binärformats zu einem kritischen Systemengpass. Dieser Engpass tritt bei jedem Schreibvorgang (zur Aktualisierung der Indizes) und bei jedem Lese-Cache-Miss auf.

* **C++ Implementierung:** Die Standardlösung für das Parsen von JSON in Hochleistungssystemen ist simdjson. Diese Bibliothek nutzt SIMD-Instruktionen (Single Instruction, Multiple Data) moderner CPUs, um JSON mit einer Geschwindigkeit von mehreren Gigabyte pro Sekunde zu verarbeiten, und ist Berichten zufolge bis zu viermal schneller als gängige Parser wie RapidJSON.35 Die Architektur muss simdjson verwenden, um die Blobs während der Indexerstellung (typischerweise während der LSM-Tree-Compaction-Phase) oder bei Bedarf bei einem Lesezugriff zu parsen und die zu indexierenden Felder zu extrahieren.39 Für die Serialisierung kann VelocyPack 11 (wie in ArangoDB 27) oder Bibliotheken wie Protobuf/FlatBuffers verwendet werden.  
* **Rust Implementierung:** Im Rust-Ökosystem ist serde (ein Serialisierungs-Framework) in Kombination mit serde\_json 45 oder einem Binärformat wie bincode 49 der idiomatisache Weg. Native Rust-Bibliotheken wie rocksmap 51 oder das Projekt mokuroku 52 demonstrieren, wie Sekundärindizes über einem RocksDB-Backend verwaltet werden, wobei serde zur Serialisierung der Daten verwendet wird.52

Die Parsing-Engine ist somit ein performance-kritisches Gateway zwischen dem kanonischen, schreiboptimierten Speicher (Write-Path) und den leseschnellen Index-Projektionen (Read-Path).

### **1.4. Tabelle 1: Multi-Modell-Datenabbildung (Architektonische Blaupause)**

Die folgende Tabelle fasst die Architektur von Teil 1 zusammen und zeigt die konkrete Abbildung von der logischen Entität (Modell) zur physischen Speicherebene (KV-Schema).

| Logisches Modell | Logische Entität | Physischer Speicher (Key-Value-Paar) | Key-Format (Byte-Array) | Value-Format (Byte-Array) |
| :---- | :---- | :---- | :---- | :---- |
| **Relational** | Eine Zeile | (PK, Blob) | String("table\_name:pk\_value") | VelocyPack/Bincode(Serialisiertes Dokument) |
| **Dokument** | Ein JSON-Dokument | (PK, Blob) | String("collection\_name:pk\_value") | VelocyPack/Bincode(Serialisiertes Dokument) |
| **Graph (Knoten)** | Ein Knoten | (PK, Blob) | String("node:pk\_value") | VelocyPack/Bincode(Serialisiertes Knotendokument) |
| **Graph (Kante)** | Eine Kante | (PK, Blob) | String("edge:pk\_value") | VelocyPack/Bincode(Serialisiertes Kantendokument inkl. \_from/\_to) |
| **Vektor** | Ein Objekt | (PK, Blob) | String("object\_name:pk\_value") | VelocyPack/Bincode(Dokument inkl. Vektor-Array) |

## **Teil 2: Die Multi-Modell-Projektionsschichten (Die „Layer“)**

Die in der Anfrage genannten „Layer“ sind keine separaten Speichersysteme. Es handelt sich um *leseoptimierte Indexprojektionen*, die aus den in Teil 1 definierten „Base Entity“-Blobs abgeleitet werden. Sie werden physisch im selben RocksDB-Speicher abgelegt und dienen ausschließlich der Beschleunigung von Leseoperationen (dem 'R' in CRUD). Jede Schicht stellt eine "Sicht" auf die kanonischen Daten bereit, die für die jeweilige Abfragesprache (SQL, Graph-Traversal, ANN-Suche) optimiert ist.17

### **2.1. Schicht 1: Relationale Projektionen (Sekundärindizes)**

* **Problem:** Beschleunigung einer SQL-ähnlichen Abfrage, z. B. SELECT \* FROM users WHERE age \= 30\. Wie in 1.2 dargelegt, ist ein Tabellenscan der Blobs inakzeptabel.  
* **Architektonischer Entwurf:** Es wird ein klassischer Sekundärindex erstellt. Physisch ist dies ein separates Set von Key-Value-Paaren innerhalb von RocksDB, das einen Wert (z. B. age \= 30\) auf den Primärschlüssel des „Base Entity“-Blobs abbildet.  
* **KV-Schema (Relationaler Index):**  
  * **Key:** String("idx:users:age:30:PK\_des\_Users\_123")  
  * **Value:** (leer) oder PK\_des\_Users\_123 (je nach Design).  
* **Abfrage-Flow:** Der Query Optimizer (siehe Teil 4\) schreibt die Abfrage um. Statt eines „Table Scan“ wird ein „Index Scan“ durchgeführt. Die Engine führt einen RocksDB Seek() zum Präfix idx:users:age:30: durch und iteriert über alle übereinstimmenden Schlüssel. Für jeden gefundenen Primärschlüssel (PK\_des\_Users\_123) wird ein Get(PK\_des\_Users\_123) auf die Haupttabelle (Teil 1\) ausgeführt, um das vollständige „Base Entity“-Blob abzurufen.  
* **Implementierung (Rust):** Die rocksmap-Bibliothek demonstriert dieses Muster elegant mit ihrer SecondaryIndex-Struktur.51 Das mokuroku-Projekt zeigt ebenfalls Beispiele für die Implementierung von Sekundärindizes in Rust über RocksDB.52  
* **Performance-Trade-off:** Dieser Ansatz beschleunigt Lesezugriffe (R) massiv, verlangsamt jedoch Schreibvorgänge (C, U, D). Bei jedem Create oder Update eines User-Blobs muss nun *auch* der Blob geparst werden (mit simdjson 35 / serde 45) und die entsprechenden Sekundärindex-Einträge müssen atomar (unter Verwendung einer WriteBatch 28 oder RocksMapBatch 51) aktualisiert werden.

### **2.2. Schicht 1: Native Graph-Projektionen (Simulierte Adjazenz)**

* **Problem:** Beschleunigung von Graph-Traversierungen (z. B. Freunde-von-Freunden-Abfragen). Native Graph-Datenbanken nutzen hierfür die „Index-freie Adjazenz“ ($O(1)$) 23, die auf direkten Speicherzeigern basiert. Dies ist in einem abstrahierten KV-Store wie RocksDB unmöglich.  
* **Architektonischer Entwurf:** Wir müssen die Adjazenz *simulieren*. Der Ansatz von ArangoDB, bei dem Kanten als Dokumente mit \_from- und \_to-Attributen gespeichert werden 9, wird als Grundlage verwendet. Wir erstellen zwei dedizierte Sekundärindizes (Projektionen), um diese Kantenbeziehungen schnell aufzulösen: einen Index für ausgehende Kanten ("Outdex") und einen für eingehende Kanten ("Index").  
* **KV-Schema (Graph-Indizes):**  
  * **Ausgehende Kanten (Outdex):**  
    * **Key:** String("graph:out:PK\_des\_Startknotens:PK\_der\_Kante")  
    * **Value:** PK\_des\_Zielknotens  
  * **Eingehende Kanten (Index):**  
    * **Key:** String("graph:in:PK\_des\_Zielknotens:PK\_der\_Kante")  
    * **Value:** PK\_des\_Startknotens  
* **Abfrage-Flow (Traversal):** Eine Traversierung (z. B. "finde alle Entitäten, mit denen user/123 verbunden ist") wird zu einem hocheffizienten RocksDB-Präfix-Scan: Seek("graph:out:user/123:"). Die Engine iteriert über dieses Präfix und erhält sofort die Primärschlüssel aller direkten Nachbarn (aus dem Value). Dies ist zwar kein $O(1)$-Zeiger-Lookup, aber ein $O(k \\cdot \\log N)$-Scan (wobei $k$ die Anzahl der Nachbarn ist), was die optimale Performance für Graph-Traversierungen auf einem LSM-Tree-basierten KV-Store darstellt.

### **2.3. Schicht 1: Vektor-Projektionen (ANN-Indizes)**

* **Problem:** Beschleunigung der Ähnlichkeitssuche (Approximate Nearest Neighbor, ANN) für die in den „Base Entity“-Blobs gespeicherten Vektoren.24  
* **Architektonischer Entwurf:** Der ANN-Index ist eine separate, hochspezialisierte Projektionsschicht. Er speichert *nicht* die Vektoren selbst, sondern eine komplexe Datenstruktur (z. B. einen HNSW-Graphen 24), die auf die *Primärschlüssel* der „Base Entity“ verweist. Der HNSW-Algorithmus (Hierarchical Navigable Small World) ist ein de-facto-Standard, der einen mehrschichtigen Graphen aufbaut, bei dem obere Schichten als "Autobahnen" für die schnelle Navigation dienen und untere Schichten dichte "lokale Straßen" darstellen.24  
* **Implementierung (C++):** HNSWlib oder Faiss 5 werden verwendet, um diesen Index zu erstellen. Wenn ein Vektor zur Datenbank hinzugefügt wird, wird die index.add(vektor, PK\_des\_Base\_Entity)-Funktion der Bibliothek aufgerufen, um den PK dem Index zuzuordnen.  
* **Abfrage-Flow (ANN-Suche):**  
  1. Eine Vektor-Abfrage trifft ein.  
  2. Die ANN-Indexschicht (z. B. HNSWlib) wird durchsucht (index.search(query\_vektor, k=10)).  
  3. Der Index gibt eine Liste von Primärschlüsseln zurück (z. B. \[PK\_7, PK\_42, PK\_99\]).  
  4. Die Abfrage-Engine führt einen MultiGet(\[PK\_7, PK\_42, PK\_99\])-Aufruf auf RocksDB aus, um die vollständigen „Base Entity“-Blobs abzurufen.  
* **Lösung für gefilterte Suchen:** Diese Architektur löst elegant die in 24 beschriebene "Achillesferse" der Vektorsuche: die gefilterte Suche (z. B. "finde ähnliche Bilder, aber nur die mit year \> 2020" 24). Anstatt einer ineffizienten Nachfilterung (Post-Filtering 24) wird eine hocheffiziente Vorfilterung (Pre-Filtering 24) durch die Kombination der Projektionsschichten ermöglicht 24:  
  1. **Phase 1 (Relational):** Der relationale Index (aus 2.1) wird gescannt (z. B. idx:year:2020:\*, idx:year:2021:\*,...), um eine *Kandidatenliste* von PKs zu erstellen (typischerweise als Bitset repräsentiert).  
  2. **Phase 2 (Vektor):** Die HNSW-Graph-Traversierung 24 wird modifiziert. An jedem Navigationsschritt wird *nur* zu Knoten navigiert, deren Primärschlüssel im Kandidaten-Bitset aus Phase 1 vorhanden sind. Dies ist die effiziente Implementierung des "Pre-Filtering"-Ansatzes 24, die nur durch die enge Verzahnung der Index-Layer möglich wird.62

### **2.4. Schicht 1: Datei/Blob-Projektionen (Externalisierung)**

* **Problem:** Speicherung von großen Binärdateien (z. B. Bilder, Videos, PDFs), die die „Base Entity“-Blobs ineffizient aufblähen und die Scan-Performance des LSM-Trees beeinträchtigen würden.  
* **Architektonischer Entwurf:** Es gibt zwei primäre Lösungen, die parallel existieren können:  
  1. **RocksDB BlobDB:** RocksDB bietet eine spezialisierte Funktion namens BlobDB (oder "Titan"). Werte (Blobs), die eine definierte Größe überschreiten, werden automatisch aus dem LSM-Tree extrahiert und in separaten, "append-only" Blob-Dateien gespeichert. Im LSM-Tree verbleibt nur ein kleiner Zeiger auf den Blob.28 Dies hält den LSM-Tree klein und performant für Scans, während große Daten effizient verwaltet werden.  
  2. **Referenz-Speicherung:** Das „Base Entity“-Blob speichert *nicht* die Datei, sondern nur Metadaten und einen URI (z. B. einen S3- oder MinIO-Pfad 65), der auf einen externen Objektspeicher verweist.  
* **Trade-off:** Die BlobDB-Lösung 28 ermöglicht es, die Blob-Speicherung atomar mit den Metadaten-Updates im LSM-Tree zu verwalten. Die externe Referenz-Speicherung ist flexibler und skalierbarer, erfordert aber komplexe verteilte Transaktionen (z. B. Two-Phase-Commit 29) oder die Akzeptanz von Eventual Consistency 38, falls ein Update im KV-Store erfolgreich ist, der Upload zum S3-Speicher jedoch fehlschlägt.

### **2.5. Transaktionale Konsistenz über Layer hinweg (Saga-Pattern vs. ACID)**

Die Architektur mit einem kanonischen Speicher (Teil 1\) und mehreren darauf aufbauenden Index-Projektionen (Teil 2\) wirft die Frage nach der Konsistenz auf, insbesondere bei Schreibvorgängen. Wenn eine „Base Entity“ aktualisiert wird, müssen *alle* relevanten Index-Projektionen (relational, graph, vektor) ebenfalls atomar aktualisiert werden.

* **ACID-Garantie (Innerhalb einer TMM-DB):** Eine echte Multi-Modell-Datenbank (TMM-DB), die diese Architektur intern implementiert, bietet ACID-Transaktionen.72 Die Aktualisierung des „Base Entity“-Blobs und aller zugehörigen Index-Einträge erfolgt innerhalb einer einzigen atomaren Transaktion, die von der Datenbank-Engine verwaltet wird. Dies garantiert starke Konsistenz.  
* **Saga-Pattern (Bei Externalisierung):** Wenn diese Architektur *nicht* innerhalb einer einzigen DB-Engine, sondern als Komposition verschiedener Systeme implementiert wird (z.B. RocksDB für Blobs \+ externe ANN-Bibliothek \+ separate Graph-Index-Verwaltung), dann entspricht dies konzeptionell wieder dem Polyglot-Persistence-Muster 1, nur auf einer niedrigeren Ebene. In diesem Fall *muss* das **Saga-Pattern** 1 auf Anwendungsebene verwendet werden, um die Konsistenz zu gewährleisten.38  
  * **Ablauf:** Eine Saga würde eine Sequenz lokaler Transaktionen orchestrieren: 1\. Schreibe Blob in RocksDB (lokale Tx). 2\. Aktualisiere relationalen Index in RocksDB (lokale Tx). 3\. Aktualisiere Graph-Index in RocksDB (lokale Tx). 4\. Füge Vektor zum externen ANN-Index hinzu (lokale Tx).67  
  * **Kompensation:** Schlägt ein Schritt fehl (z.B. Schritt 4), müssen kompensierende Transaktionen die vorherigen Schritte rückgängig machen (z.B. Lösche Index-Einträge aus Schritt 2 und 3, markiere Blob in Schritt 1 als ungültig).67  
  * **Konsequenz:** Dies führt zu **Eventual Consistency** (BASE) 72 und verlagert die Komplexität der Konsistenzverwaltung in die Anwendung.38  
  * **Compliance-Implikationen (DSGVO/EU AI Act):** Die Verwendung des Saga-Patterns in einem verteilten System erhöht die Komplexität bei der Einhaltung von Vorschriften wie der DSGVO.83 Das Recht auf Vergessenwerden erfordert beispielsweise das zuverlässige Löschen von Daten über alle beteiligten Systeme hinweg, was durch kompensierende Transaktionen sichergestellt werden muss. Ebenso können Anforderungen des EU AI Acts an Datenprovenienz und Auditierbarkeit schwerer zu erfüllen sein, da die Nachverfolgung einer logischen Transaktion über mehrere physische, eventuell inkonsistente Zustände hinweg erfolgen muss. Eine ACID-konforme TMM-DB vereinfacht diese Compliance-Aufgaben erheblich.

## **Teil 3: Detaillierter Entwurf der Speicherhierarchie (CRUD-Leistungsoptimierung)**

Dies ist die direkte Antwort auf die Kernfrage zur physischen Platzierung von Datenkomponenten zur Maximierung der CRUD-Geschwindigkeit. Der Entwurf basiert auf der Standard-Speicherhierarchie (Registers, Cache, RAM, SSD, HDD) 154 und wendet sie auf die in Teil 1 und 2 definierten Architekturkomponenten an.

### **3.1. Die Persistenz-Grundlage (HDD / Massenspeicher)**

* **Medium:** Hard Disk Drives (HDD).  
* **Analyse:** Aufgrund der extrem hohen Latenz bei wahlfreien Zugriffen (verursacht durch physische Suchzeiten und Rotationslatenz 68) sind HDDs für *alle* primären CRUD-Operationen in einem Hochleistungssystem ungeeignet.69  
* **Platzierung:** HDDs werden *ausschließlich* für kalte Backups, Langzeitarchivierung (Tertiary Storage 75) oder als kostengünstiges Ziel für die Replikation des Write-Ahead Log (WAL) für die Notfallwiederherstellung (Disaster Recovery) verwendet.

### **3.2. Die „Workhorse“-Schicht (NVMe-SSD)**

* **Medium:** Solid State Drives (SSD), idealerweise NVMe (Non-Volatile Memory Express) für die geringste Latenz.  
* **Analyse:** SSDs sind der Standard für schnelle Speicherung.31 Ihr Hauptvorteil ist der Wegfall der Latenz für wahlfreie Zugriffe, was sie ideal für Datenbank-Workloads macht.68  
* **Platzierung:**  
  1. **Write-Ahead Log (WAL):** Dies ist die kritischste Komponente für die Schreibleistung. Jede Create, Update, Delete-Operation (CRUDs C, U, D) *muss* synchron in den WAL geschrieben werden, bevor sie als "committed" gilt.69 Die Latenz des WAL *definiert* die Schreiblatenz des gesamten Systems. Der WAL wird sequentiell geschrieben, was ein optimales Zugriffsmuster für SSDs darstellt.68 Er muss auf dem schnellsten persistenten Medium (NVMe-SSD) liegen.72  
  2. **LSM-Tree SSTables / BlobDB:** Die Hauptdaten, d. h. die persistenten, sortierten Dateien (SSTables) des LSM-Trees, die die „Base Entity“-Blobs (Teil 1\) und alle Sekundärindizes (Teil 2\) enthalten, liegen auf der NVMe-SSD-Flotte.30 Hier finden alle wahlfreien Lesezugriffe statt, die nicht aus dem RAM-Cache bedient werden können.  
  3. **On-Disk-ANN-Indizes:** Für Vektor-Workloads im Terabyte-Maßstab, die den RAM übersteigen, müssen SSD-optimierte ANN-Strukturen verwendet werden. DiskANN ist ein Algorithmus, der explizit für SSDs entwickelt wurde.78 Faiss (C++) unterstützt ebenfalls On-Disk-Indizes 13, wie IndexIVFPQ 80, die für SSD-Zugriffsmuster optimiert sind.

### **3.3. Die „Hot“-Schicht (Haupt-RAM / DRAM)**

* **Medium:** Hauptspeicher (DRAM).  
* **Analyse:** RAM ist die primäre Arbeitsschicht mit Latenzen, die um Größenordnungen geringer sind als bei SSDs.154  
* **Platzierung:**  
  1. **LSM-Tree Memtable:** Alle neuen Schreibvorgänge (CRUDs C, U, D) landen *zuerst* hier. Das RocksDB-Memtable ist eine In-Memory-Datenstruktur (z. B. eine Skip-List), die vollständig im RAM lebt.30  
  2. **LSM-Tree Block Cache:** Analog zum Puffer-Cache relationaler Datenbanken *muss* der RocksDB-Block-Cache im RAM liegen. Er speichert heiße, kürzlich gelesene Datenblöcke (SSTable-Blöcke) von der SSD, um wiederholte Lesezugriffe (CRUDs R) zu beschleunigen und I/O zu vermeiden.30  
  3. **HNSW-Index (Obere Schichten):** Dies beantwortet eine spezifische Teilfrage des Benutzers. Für eine performante Vektorsuche muss der HNSW-Index-Graph im RAM sein.24 Bei Indizes, die zu groß für den RAM sind, wird ein hybrider Ansatz verwendet: Die *oberen Schichten* des HNSW-Graphen (die "Autobahnen" 24) sind spärlich und werden bei *jeder* Suche durchlaufen.82 Sie *müssen* daher permanent im RAM gehalten werden ("gepinnt").83 Die dichteren *unteren Schichten* 24 (die "lokalen Straßen") können bei Bedarf von der SSD in den Block-Cache geladen werden.87 Dies kann durch Memory-Mapping (mmap) 24 oder durch benutzerdefiniertes Caching der Indexknoten realisiert werden.24  
  4. **Graph-Topologie (Hot):** Für Graph-Workloads mit Latenzanforderungen im Sub-Millisekunden-Bereich (z. B. Echtzeit-Betrugserkennung) ist selbst der $O(k \\cdot \\log N)$-Scan des Adjazenz-Index auf der SSD (aus 2.2) zu langsam. In diesem "High-Performance"-Modus muss die *gesamte* Graphtopologie (d. h. die Adjazenzlisten/Indizes graph:out:\* und graph:in:\*) beim Systemstart proaktiv von der SSD in den RAM geladen werden.  
* **RAM-Implementierung der Graph-Topologie:** Diese In-Memory-Topologie wird *nicht* als RocksDB-Block-Cache verwaltet, sondern als native C++- oder Rust-Datenstruktur für maximale Traversierungsgeschwindigkeit.147  
  * **C++:** Verwendung der C++-Kernkomponenten von graph-tool 23, die Adjazenzlisten hocheffizient speichern 88, oder einer std::vector\<std::vector\<AdjacencyInfo\>\>.  
  * **Rust:** Verwendung der petgraph-Bibliothek 89 oder, für höhere Performance, einer benutzerdefinierten Vec\<Vec\<usize\>\>-Adjazenzlisten-Implementierung.147  
* **Resultierender Abfrage-Flow:** Eine Graph-Traversierung ist nun ein $O(k)$-Lookup im RAM (um die PKs der Nachbarn zu finden), gefolgt von $k$ Get(PK)-Operationen auf RocksDB, um die "Base Entity"-Blobs abzurufen (die idealerweise im RAM-Block-Cache (Punkt 2 oben) liegen).

### **3.4. Die Beschleuniger-Schicht (VRAM / Grafik-RAM)**

* **Medium:** VRAM (Video Random Access Memory) auf einer dedizierten GPU.  
* **Analyse:** VRAM ist *nicht* Teil der allgemeinen CPU-Speicherhierarchie, sondern der Speicher eines Co-Prozessors.154 Er ermöglicht massiv-parallele Berechnungen.  
* **Platzierung:**  
  1. **ANN-Index-Fragmente:** VRAM wird *ausschließlich* zur Beschleunigung von rechenintensiven ANN-Vektor-Suchen (CRUDs R) verwendet.24  
  2. **C++ Implementierung:** Faiss-GPU 5 ist der De-facto-Standard.5  
  3. Abfrage-Flow (VRAM):  
     a. Der Query Optimizer (Teil 4\) entscheidet (kostenbasiert), die GPU zu verwenden.  
     b. Die relevanten Teile des ANN-Index (z. B. die Quantizer-Daten eines IndexIVFPQ 80 oder HNSW-Graph-Fragmente) werden vom Haupt-RAM über den PCIe-Bus 76 in den VRAM kopiert.98  
     c. Der Abfragevektor wird ebenfalls in den VRAM kopiert.  
     d. Tausende von GPU-Kernen führen die Distanzberechnungen massiv-parallel durch.  
     e. Die GPU gibt eine Liste der Top-k-PKs zurück an den CPU-RAM.  
     f. Die CPU-Engine ruft die „Base Entity“-Blobs von RocksDB (RAM-Cache/SSD) ab.  
* **Performance-Trade-off:** Dieser Prozess ist *nicht* immer schneller. Der Kopiervorgang von CPU-RAM zu VRAM über den PCIe-Bus hat eine signifikante Latenz. GPU-Beschleunigung lohnt sich daher primär für *Batch-Abfragen* (Suche nach Tausenden von Vektoren gleichzeitig) oder bei extrem hochdimensionalen Vektoren, bei denen die parallele Rechenleistung der GPU die Transferlatenz übersteigt.

### **3.5. Tabelle 2: Strategie der Speicherhierarchie für CRUD-Optimierung**

Die folgende Tabelle fasst die in Teil 3 entwickelte Strategie zusammen und beantwortet die Kernfrage des Benutzers nach der optimalen Platzierung der Datenkomponenten zur Maximierung der CRUD-Leistung.

| Datenkomponente | Physischer Speicher | Primär optimierte Operation | Begründung (Latenz/Durchsatz) |
| :---- | :---- | :---- | :---- |
| **Write-Ahead Log (WAL)** | **NVMe-SSD** (schnellstes persistent) | Create, Update, Delete | Minimale Latenz für synchrone, sequentielle Schreibvorgänge.69 Definiert die Schreib-Commit-Zeit.\[77\] |
| **LSM-Tree Memtable** | **RAM (DRAM)** | Create, Update, Delete | In-Memory-Pufferung von Schreibvorgängen; schnellste Aufnahme (Ingestion).30 |
| **LSM-Tree Block Cache** | **RAM (DRAM)** | Read | Caching von heißen Datenblöcken (Base Entities, Indizes) von der SSD.30 Reduziert wahlfreie Lese-I/O. |
| **LSM-Tree SSTables (Kerndaten & Indizes)** | **SSD (NVMe/SATA)** | Read (Cache Miss) | Persistente Speicherung. Benötigt schnelle wahlfreie Lese-I/O für Punkt- und Bereichsabfragen.\[31\] |
| **HNSW-Index (Obere Schichten)** | **RAM (DRAM)** | Read (Vektor-Suche) | "Autobahnen" des Graphen.24 Müssen bei *jeder* Suche im Speicher sein, um Navigations-Hotspots zu vermeiden.\[83, 86\] |
| **HNSW-Index (Untere Schichten) / DiskANN** | **SSD (NVMe)** | Read (Vektor-Suche) | Zu groß für RAM. Optimiert für SSD-basierte wahlfreie Lesezugriffe während der Endphase der ANN-Suche.78 |
| **Graph-Topologie (Hot)** | **RAM (DRAM)** | Read (Graph-Traversal) | Simulierte "Index-freie Adjazenz".23 Topologie \[88, 90\] wird für $O(k)$-Traversierungen im RAM gehalten. |
| **ANN-Index (GPU-Kopie)** | **VRAM (Grafik-RAM)** | Read (Batch-Vektor-Suche) | Temporäre Kopie 98 zur massiv-parallelen Beschleunigung der Distanzberechnung.\[5, 76\] |
| **Kalte Blobs / Backups** | **HDD / Cloud Storage** | (Offline) | Günstigste Speicherung für Daten ohne Latenzanforderungen.75 |

## **Teil 4: Implementierungs-Toolkit und Parallelisierungsstrategien (C++ / Rust)**

Dieser Teil adressiert die Anfrage nach spezifischen C++- und Rust-Bibliotheken und skizziert, wie die Abfrageausführung parallelisiert wird, um moderne Multi-Core-CPUs voll auszunutzen.99

### **4.1. Die parallele Abfrage-Engine: Task-basierter Parallelismus**

* **Problem:** Eine hybride Abfrage (z. B. relationaler Filter \+ Graph-Traversal \+ Vektor-Suche) 58 besteht aus mehreren voneinander abhängigen oder unabhängigen Tasks. Diese müssen effizient auf N CPU-Kernen ausgeführt werden.  
* **Architektonischer Entwurf (C++):** Intel Threading Building Blocks (TBB).100 TBB ist für diesen Anwendungsfall OpenMP 63 überlegen.6 OpenMP ist primär auf die Parallelisierung von Schleifen (data parallelism) ausgelegt, während TBB ein robustes, Task-basiertes Laufzeitsystem (task parallelism) mit einem "Work-Stealing"-Scheduler bietet.18 Eine hybride Abfrage wird in einen Graphen von tbb::task zerlegt. Beispielsweise können task\_A (relationaler Scan) und task\_B (Vektor-Suche) parallel ausgeführt werden (tbb::parallel\_invoke 112), und task\_C (Join/Schnittmenge) wird als Fortsetzungs-Task definiert, der erst ausgeführt wird, wenn beide Vorgänger abgeschlossen sind.  
* **Architektonischer Entwurf (Rust):** Rayon.99 Rayon ist das Äquivalent zu TBB in Rust und bietet eine extrem einfache und sichere Datenparallelität.113 Die Funktion rayon::join 117 wird verwendet, um zwei unabhängige Tasks (z. B. zwei parallele Datenbankabfragen) rekursiv aufzuteilen und auf dem Thread-Pool auszuführen.118 par\_iter() 113 wird verwendet, um Datenverarbeitungsschritte (z. B. die Verarbeitung von Suchergebnissen) auf alle verfügbaren Kerne zu verteilen. Für die Verwaltung der asynchronen I/O-Operationen auf der SSD (das Warten auf Lesevorgänge von RocksDB) wird Tokio als primäre Laufzeitumgebung eingesetzt.10

### **4.2. Das analytische In-Memory-Format: Apache Arrow**

* **Problem:** Wie werden *analytische* Abfragen (OLAP) schnell ausgeführt, die *Millionen* von Entitäten scannen (z. B. AVG(age) über die gesamte User-Tabelle)? Das zeilenweise Abrufen und Deserialisieren (OLTP-Stil 119) von Millionen von Blobs (Teil 1\) wäre ein Performance-Desaster.  
* **Architektonischer Entwurf (Hybride Lösung):** Die Architektur nutzt Apache Arrow 119 nicht als primäres *Speicherformat* auf der Festplatte (das bleibt der RocksDB-Blob 119), sondern als kanonisches *In-Memory-Format für die analytische Abfrageausführung*.  
* **Abfrage-Flow (OLAP):**  
  1. Ein analytischer Scan (z. B. SQL GROUP BY) wird gestartet.  
  2. Ein paralleler Worker-Pool (mit Rayon 99 oder TBB 100) liest die RocksDB-SSTable-Blöcke.  
  3. Anstatt die Blobs einzeln zu verarbeiten, werden sie (mit simdjson \[C++\] oder serde) *direkt in Apache Arrow RecordBatches* 121 deserialisiert.  
  4. Die gesamte weitere Verarbeitung (Filter, Aggregationen) findet nun *nicht* mehr auf den einzelnen Objekten statt, sondern hochperformant auf den spaltenbasierten Arrow-Arrays, die CPU-Cache-freundlich (SIMD-optimiert) sind.  
* **C++/Rust-Implementierung:** Apache DataFusion 142 ist eine in Rust geschriebene, Arrow-native Query-Engine 122, die *genau das* tut: Sie führt SQL-Abfragen direkt auf Arrow-Daten aus.123 Sie kann als analytische Ausführungsschicht *über* der RocksDB/Arrow-Deserialisierungsschicht (Teil 1.3) sitzen. Apache Arrow Flight 36 dient als hochperformantes RPC-Framework (basierend auf gRPC und FlatBuffers 64), um diese Arrow-Batches effizient zwischen den Knoten des Systems zu übertragen, ohne Serialisierungs-Overhead.124

### **4.3. Der hybride Abfrage-Optimierer: Ein Kostenmodell für alle Modelle**

* **Problem:** Das System muss für jede hybride Abfrage 58 den effizientesten Ausführungsplan wählen.  
* **Beispiel-Query:** MATCH (u:User)--\>(f:User) WHERE u.name \= 'Alice' AND f.age \> 30 AND vector\_similarity(f.profile\_vec, \[...\]) \> 0.9  
* **Architektonischer Entwurf (Optimizer):** Der Optimizer muss die Kosten (I/O, CPU) für jeden potenziellen Plan schätzen :  
  1. Plan A (Start: Graph/Relational):  
     a. Beginne mit u.name \= 'Alice' (Relationaler Index 2.1; Selektivität: 1 Treffer).  
     b. Traversal (u)--\>(f) (Graph-Index 2.2; Selektivität: 50 Treffer).  
     c. Filter f.age \> 30 (Relationaler Index 2.1; Selektivität: 10 Treffer).  
     d. Führe Vektor-Suche (2.3) auf den verbleibenden 10 Vektoren durch.  
     e. Geschätzte Kosten: Niedrig.  
  2. Plan B (Start: Vektor):  
     a. Beginne mit globaler Vektor-Suche vector\_similarity \> 0.9 (Vektor-Index 2.3; Selektivität: 10.000 Treffer).  
     b. Filter f.age \> 30 (10.000 Treffer $\\rightarrow$ 2.000 Treffer).  
     c. Führe Graph-Lookup (2.2) für 2.000 Treffer durch, um zu prüfen, ob sie Freunde von 'Alice' sind.  
     d. Geschätzte Kosten: Hoch.  
* **Fazit:** Der Optimizer (der "Gehirn"-Layer 126) ist entscheidend. Er muss die Selektivität und Kosten *aller* Index-Projektionen (relational, graph, vektor) kennen und verstehen, wie sie kombiniert werden können (z. B. "Hybrid Search" 127), um den Ausführungsplan mit den geringsten Gesamtkosten auszuwählen.

### **4.4. Tabelle 3: C++ vs. Rust Implementierungs-Toolkit (Empfohlene Bausteine)**

Die folgende Tabelle beantwortet die letzte Frage des Benutzers nach konkreten Bibliotheken und Codefragmenten für eine C++- oder Rust-Implementierung.

| Komponente | C++ Bibliothek(en) | Rust Bibliothek(en) | Begründung |
| :---- | :---- | :---- | :---- |
| **Key-Value Storage Engine** | RocksDB \[28, 30\] | rocksdb (Wrapper) 51, redb, sled \[49, 131, 32, 132, 61, 133, 134\] | RocksDB ist der C++-Standard.\[135, 136, 137, 34\] Rust-Alternativen wie sled \[49\] oder redb \[132\] sind vielversprechend, aber RocksDB ist ausgereifter. |
| **Parallel Execution Engine** | Intel TBB (Tasking) \[100, 60\], OpenMP (Loops) \[63, 106\] | Rayon (Tasking/Loops) \[99, 113\], Tokio (Async I/O) 53 | TBB (C++) \[102, 103, 101, 138, 139\] und Rayon (Rust) \[118, 115, 116, 114, 139\] bieten beide erstklassiges, Task-basiertes Work-Stealing \[18\], das für Query-Engines ideal ist.117 |
| **JSON/Binary Parsing** | simdjson \[35, 39\], VelocyPack \[11, 43\] | serde / serde\_json \[51, 50, 47, 48\], bincode \[49\] | simdjson ist ein Performance-Muss für C++.\[48, 37, 38\] serde ist das idiomatisache und flexible Ökosystem in Rust.45 |
| **In-Memory Graph-Topologie** | C++ Backend von graph-tool \[140, 141\], Boost.Graph | petgraph \[89, 92\], Custom Vec\<Vec\<T\>\> 147 | petgraph ist der Standard in Rust \[90, 93, 94, 95, 96, 30, 49\], aber Benchmarks \[91\] und Analysen 88 zeigen, dass für Top-Performance eine benutzerdefinierte Adjazenzliste oft überlegen ist. |
| **Vektor-Index (ANN)** | Faiss (CPU/GPU) \[13, 5\], HNSWlib \[25, 71\] | hnsw (native Rust) oder Wrapper für Faiss/HNSWlib | Das C++-Ökosystem (Faiss) ist hier unübertroffen, insbesondere durch die GPU-Unterstützung 98 und die On-Disk-Formate.\[61, 57, 81\] |
| **In-Memory Analytics & IPC** | Apache Arrow 121, Apache DataFusion \[142, 123\] | arrow-rs 122, datafusion \[122, 129, 123, 34, 120\] | Arrow und DataFusion sind in C++ und Rust (via arrow-rs) erstklassig und bilden das Rückgrat für OLAP-Workloads.\[36, 119, 142, 121, 124, 64, 125, 101, 120\] |

## **Teil 5: Sicherheitsarchitektur: Integration von Kerberos, RBAC und Verschlüsselung**

Dieser Teil integriert die Sicherheitsanforderungen (Kerberos, Benutzerverwaltung, Verschlüsselung) in den entworfenen Kernel. Es wird ein zentralisiertes Governance-Modell bevorzugt, das auf externen Systemen wie Apache Ranger aufbaut, aber auch eine interne Implementierung skizziert wird.83

### **5.1 Authentifizierung: Kerberos/GSSAPI-Integration**

* **Anforderung:** Benutzer sollen sich über Kerberos authentifizieren.83  
* **Architektur:** Die Authentifizierung wird *nicht* direkt im Datenbankkern, sondern in einer vorgelagerten API-Schicht (z.B. mittels FastAPI) implementiert. Diese Schicht fungiert als Gateway.  
* **Implementierung (Python):** Verwendung von Middleware (z.B. asgi-gssapi) und der python-gssapi-Bibliothek. Die Middleware fängt eingehende Anfragen ab, validiert das Kerberos-Ticket (via GSSAPI AcceptContext) anhand der Server-Keytab und extrahiert den authentifizierten Benutzerprinzipal (z.B. user@REALM).

### **5.2 Autorisierung: Rollen- und Rechteverwaltung (RBAC)**

* **Anforderung:** Verwaltung von Benutzerrollen und \-rechten.23  
* **Architektur (Bevorzugt: Extern/Zentralisiert):** Integration mit einem externen, zentralisierten Autorisierungs-Framework wie **Apache Ranger** 83 (oder kommerziellen Äquivalenten wie Privacera 83).  
  * **Komponenten:** Ranger Admin (Policy Definition) 141, Ranger Plugins (im API-Gateway/DB-Kernel) , Ranger UserSync (AD/LDAP-Sync).  
  * **Flow:** Das Plugin im API-Gateway empfängt den authentifizierten Kerberos-Prinzipal (aus 5.1). Es fragt die zentralen Ranger-Richtlinien ab (die auf AD/LDAP-Gruppen basieren 83) und trifft die Erlauben/Verweigern-Entscheidung.83  
  * **Vorteil:** Konsistente Richtlinienverwaltung über heterogene Systeme hinweg.83  
* **Architektur (Alternativ: Intern/Graph-nativ):** Implementierung von RBAC direkt innerhalb der Datenbank.  
  * **Modellierung:** Das RBAC-Modell selbst wird als Graph gespeichert: (User)--\>(Role)--\>(Permission)--\>(Resource).  
  * **Implementierung (Python):** Verwendung einer Autorisierungsbibliothek wie pycasbin im API-Gateway. Ein benutzerdefinierter Adapter übersetzt Casbin enforce-Aufrufe in Cypher-Abfragen gegen den RBAC-Graphen in der eigenen Datenbank, um Berechtigungen zur Laufzeit zu prüfen.  
  * **Nachteil:** Geringere Konsistenz bei Verwendung mehrerer Datenbanksysteme; jede DB verwaltet ihre eigenen Rechte.

### **5.3 Verschlüsselung**

* **Anforderung:** Schutz der Daten während der Übertragung und im Ruhezustand.83  
* **Verschlüsselung während der Übertragung (Data-in-Transit):** Die Kommunikation zwischen Clients, dem API-Gateway und dem Datenbankkern muss über TLS erfolgen.83 Kerberos-Tickets selbst bieten ebenfalls kryptographischen Schutz.83  
* **Verschlüsselung im Ruhezustand (Data-at-Rest):** Die physischen Speicherdateien (mmap-Dateien für Topologie, RocksDB für Blobs/Indizes, SQLite für Properties) müssen auf Betriebssystem- oder Dateisystemebene verschlüsselt werden. Schlüsselverwaltung erfolgt idealerweise über ein externes Key Management System (KMS), potenziell integriert mit Ranger KMS.83

### **5.4 Auditing und Compliance (DSGVO/EU AI Act)**

* **Anforderung:** Nachvollziehbarkeit von Datenzugriffen und \-änderungen.83  
* **Architektur (Bevorzugt: Extern/Zentralisiert):** Bei Verwendung von Apache Ranger 141 werden alle Autorisierungsentscheidungen (erlaubt/verweigert) zentral protokolliert.144 Diese Audit-Logs sind essentiell für Compliance-Nachweise (z.B. wer wann auf welche Daten zugegriffen hat).83  
* **Architektur (Intern):** Bei interner RBAC-Implementierung muss ein eigenes, detailliertes Audit-Log-System im API-Gateway oder Datenbankkern entwickelt werden.  
* **Bedeutung für Compliance:** Zentralisierte Audit-Logs vereinfachen die Einhaltung der DSGVO (Rechenschaftspflicht, Auskunftsrecht) und potenzieller AI Act-Anforderungen (Transparenz, Nachvollziehbarkeit von KI-Entscheidungen, die auf den Daten basieren).83

## **Teil 6: API Layer (Client Interface)**

Dieser Teil beschreibt, wie der Server-Teil, der Client-Anfragen entgegennimmt, implementiert wird.

* **Protokollwahl:**  
  * **HTTP (REST/GraphQL):** Gängig, breit unterstützt, einfacher zu debuggen, aber potenzieller Overhead durch textbasiertes JSON.35  
  * **Binär (gRPC, Arrow Flight):** Performanter, geringerer Serialisierungs-Overhead.64 **Apache Arrow Flight** 36 ist ideal für den Hochleistungs-Transport von Arrow-Daten.124  
* **Concurrency-Modell:**  
  * **Asynchrones I/O:** Unerlässlich für die Skalierung auf Tausende von Verbindungen (Boost.Asio in C++, Tokio in Rust 53).  
  * **CPU Thread Pool:** Abfrageausführung an einen separaten Pool (TBB 100 / Rayon 99) übergeben, um die I/O-Schleife nicht zu blockieren.  
* **Implementierung C++:**  
  * **Async:** Boost.Asio, libuv.  
  * **HTTP:** Boost.Beast, Crow, oat++.  
  * **gRPC/Flight:** gRPC C++, Apache Arrow C++.121  
  * **Threading:** TBB 100, std::thread.  
  * **Parsing:** simdjson 35, nlohmann/json, Protobuf/FlatBuffers.  
* **Implementierung Rust:**  
  * **Async:** Tokio 53, async-std.  
  * **HTTP:** Actix-web, Axum, Rocket, Warp.  
  * **gRPC/Flight:** Tonic, arrow-flight.125  
  * **Threading:** Tokio Tasks 10, spawn\_blocking 53, Rayon.99  
  * **Parsing:** Serde 45, prost/tonic (Protobuf), arrow-rs.122  
* **Architekturmuster:**  
  * **Proactor/Reactor:** Standard für async Server.  
  * **Middleware:** Für AuthN (Kerberos 83), AuthZ (RBAC 83), Logging.  
  * **Worker Pool Pattern:** Trennung von I/O und CPU-gebundener Arbeit.

## **Teil 7: Strategische Zusammenfassung und Kompromisse**

### **7.1. Synthese des vorgeschlagenen Entwurfs**

Dieser Bericht skizziert eine kohärente Architektur für ein echtes Multi-Modell-Datenbanksystem, das die widersprüchlichen Anforderungen von vier Datenmodellen und Hochleistungs-CRUD-Operationen in Einklang bringt.

Die Architektur basiert auf einem **kanonischen „Base Entity“-Blob**, das in einer **LSM-Tree KV-Engine (RocksDB)** gespeichert ist. Diese Grundarchitektur ist inhärent *schreiboptimiert* (hohe C/U/D-Leistung).

Die „Layer“ sind *leseoptimierte* **Index-Projektionen** (sekundäre, graphische, vektorielle), die aus dem Basis-Blob abgeleitet werden und im selben KV-Store leben. Sie ermöglichen effiziente hybride Abfragen. Die Konsistenz zwischen Blob und Indizes wird idealerweise durch **ACID-Transaktionen** innerhalb der Engine gewährleistet; andernfalls muss das komplexere **Saga-Pattern** mit **Eventual Consistency** verwendet werden, was Compliance erschwert.83

Die Systemleistung wird durch die intelligente Verteilung der Komponenten auf die **Speicherhierarchie** (RAM, VRAM, NVMe-SSD) und **parallele Abfrageausführung** (TBB/Rayon) erreicht.

Die Sicherheit wird durch eine Kombination aus **Kerberos/GSSAPI-Authentifizierung** in einem API-Gateway und einer **zentralisierten RBAC-Autorisierung** (idealerweise Apache Ranger) sowie **Datenverschlüsselung** gewährleistet. Zentralisierte Audit-Logs sind für die **Compliance** (DSGVO, AI Act) entscheidend.83 Der **API Layer** bietet flexible Anbindungsmöglichkeiten (HTTP, Arrow Flight) über ein asynchrones Server-Modell.

### **7.2. C++ vs. Rust: Eine strategische Empfehlung**

Die Wahl zwischen C++ und Rust ist ein strategischer Kompromiss:

* **C++:** Bietet das derzeit **ausgereifteste und leistungsfähigste Ökosystem** für die Schlüsselkomponenten. Insbesondere die GPU-Integration von Faiss 98 und die etablierte Stabilität von RocksDB 30 und TBB 100 sind unübertroffen. Projekte wie **ArangoDB** 27 dienen als Referenz. Dies ist der pragmatische Weg für eine Implementierung, die schnell rohe Performance, insbesondere im Vektor-Bereich, demonstrieren muss.  
* **Rust:** Bietet die **garantierte Speichersicherheit**, die für die Entwicklung eines robusten, hochgradig nebenläufigen Datenbankkernels ein enormer strategischer Vorteil ist.147 Das Ökosystem (Rayon 99, DataFusion 123, petgraph 89, sled 32) ist hervorragend und holt technologisch schnell auf. Projekte wie **CozoDB** zeigen das Potenzial für integrierte Hybrid-Systeme. Die Vermeidung von Pufferüberläufen, Use-after-Free und Datenwettläufen (Data Races) in einem komplexen System wie diesem (mit parallelen Abfragen, Caching und Index-Updates) ist ein entscheidender Vorteil für die langfristige Wartbarkeit und Stabilität.

**Abschließende Empfehlung:** Für einen Prototyp, der rohe Performance (insbesondere GPU-beschleunigte Vektor-Suche) demonstrieren muss, ist der **C++-Stack** (RocksDB, Faiss, TBB, simdjson) überlegen. Für ein langfristiges, robustes und wartbares Produktionssystem, bei dem Speichersicherheit und Korrektheit in einem hochgradig nebenläufigen Kernel von größter Bedeutung sind, ist der **Rust-Stack** (RocksDB-Wrapper/redb, Rayon, DataFusion, serde) die strategisch überlegene Wahl.

#### **Referenzen**

1. Polyglot Persistence: A Strategic Approach to Modern Data Architecture \- Medium, Zugriff am Oktober 26, 2025, [https://medium.com/@rachoork/polyglot-persistence-a-strategic-approach-to-modern-data-architecture-e2a4f957f50b](https://medium.com/@rachoork/polyglot-persistence-a-strategic-approach-to-modern-data-architecture-e2a4f957f50b)  
2. Enabling data persistence in microservices \- AWS Prescriptive Guidance, Zugriff am Oktober 26, 2025, [https://docs.aws.amazon.com/prescriptive-guidance/latest/modernization-data-persistence/welcome.html](https://docs.aws.amazon.com/prescriptive-guidance/latest/modernization-data-persistence/welcome.html)  
3. Rust RocksDB — MergeOperator: Multiple Callbacks & Data Model | by Hiraq Citra M, Zugriff am Oktober 26, 2025, [https://medium.com/lifefunk/rust-rocksdb-mergeoperator-multiple-callbacks-data-model-bebf2eb00fc0](https://medium.com/lifefunk/rust-rocksdb-mergeoperator-multiple-callbacks-data-model-bebf2eb00fc0)  
4. nmslib/hnswlib: Header-only C++/python library for fast approximate nearest neighbors \- GitHub, Zugriff am Oktober 26, 2025, [https://github.com/nmslib/hnswlib](https://github.com/nmslib/hnswlib)  
5. FAISS: A quick tutorial to efficient similarity search | by Shayan Fazeli | Medium, Zugriff am Oktober 26, 2025, [https://shayan-fazeli.medium.com/faiss-a-quick-tutorial-to-efficient-similarity-search-595850e08473](https://shayan-fazeli.medium.com/faiss-a-quick-tutorial-to-efficient-similarity-search-595850e08473)  
6. C++ Parallelization Libraries: OpenMP vs. Thread Building Blocks \[closed\] \- Stack Overflow, Zugriff am Oktober 26, 2025, [https://stackoverflow.com/questions/615264/c-parallelization-libraries-openmp-vs-thread-building-blocks](https://stackoverflow.com/questions/615264/c-parallelization-libraries-openmp-vs-thread-building-blocks)  
7. Class faiss::gpu::GpuIndex, Zugriff am Oktober 26, 2025, [https://faiss.ai/cpp\_api/class/classfaiss\_1\_1gpu\_1\_1GpuIndex.html](https://faiss.ai/cpp_api/class/classfaiss_1_1gpu_1_1GpuIndex.html)  
8. A Deep Dive into Multi-Model Databases: Hype vs. Reality \- ChaosSearch, Zugriff am Oktober 26, 2025, [https://www.chaossearch.io/blog/why-enterprises-need-a-true-multi-model-platform](https://www.chaossearch.io/blog/why-enterprises-need-a-true-multi-model-platform)  
9. Data Structure | ArangoDB Documentation, Zugriff am Oktober 26, 2025, [https://docs.arangodb.com/3.11/concepts/data-structure/](https://docs.arangodb.com/3.11/concepts/data-structure/)  
10. Parallelizing rustc using Rayon \- compiler \- Rust Internals, Zugriff am Oktober 26, 2025, [https://internals.rust-lang.org/t/parallelizing-rustc-using-rayon/6606](https://internals.rust-lang.org/t/parallelizing-rustc-using-rayon/6606)  
11. ArangoDB 3.0 Alpha Release: Getting Closer to the Future, Zugriff am Oktober 26, 2025, [https://arangodb.com/2016/05/getting-closer-arangodb-3-0-alpha-release/](https://arangodb.com/2016/05/getting-closer-arangodb-3-0-alpha-release/)  
12. Features and Improvements in ArangoDB 3.0, Zugriff am Oktober 26, 2025, [https://docs.arangodb.com/3.13/release-notes/version-3.0/whats-new-in-3-0/](https://docs.arangodb.com/3.13/release-notes/version-3.0/whats-new-in-3-0/)  
13. Welcome to Faiss Documentation — Faiss documentation, Zugriff am Oktober 26, 2025, [https://faiss.ai/](https://faiss.ai/)  
14. Conceptual Definition Language (CDL) \- capire, Zugriff am Oktober 26, 2025, [https://cap.cloud.sap/docs/cds/cdl](https://cap.cloud.sap/docs/cds/cdl)  
15. Object, Stateful, and Sourced Entities With CQRS \- VLINGO XOOM, Zugriff am Oktober 26, 2025, [https://docs.vlingo.io/xoom-lattice/entity-cqrs](https://docs.vlingo.io/xoom-lattice/entity-cqrs)  
16. Planet Taskcluster, Zugriff am Oktober 26, 2025, [https://planet.mozilla.org/taskcluster/](https://planet.mozilla.org/taskcluster/)  
17. RavenDB's Index Store, Indexing Process and Eventual Consistency \- Code 972, Zugriff am Oktober 26, 2025, [https://code972.com/blog/2013/11/ravendbs-index-store-indexing-process-and-eventual-consistency-513](https://code972.com/blog/2013/11/ravendbs-index-store-indexing-process-and-eventual-consistency-513)  
18. Pro TBB: C++ Parallel Programming with Threading Building Blocks : r/cpp \- Reddit, Zugriff am Oktober 26, 2025, [https://www.reddit.com/r/cpp/comments/cov2xw/pro\_tbb\_c\_parallel\_programming\_with\_threading/](https://www.reddit.com/r/cpp/comments/cov2xw/pro_tbb_c_parallel_programming_with_threading/)  
19. java \- JPA \- create-if-not-exists entity? \- Stack Overflow, Zugriff am Oktober 26, 2025, [https://stackoverflow.com/questions/3562105/jpa-create-if-not-exists-entity](https://stackoverflow.com/questions/3562105/jpa-create-if-not-exists-entity)  
20. Apache CouchDB, Zugriff am Oktober 26, 2025, [https://couchdb.apache.org/](https://couchdb.apache.org/)  
21. HybridRAG: Integrating Knowledge Graphs and Vector Retrieval Augmented Generation for Efficient Information Extraction | Request PDF \- ResearchGate, Zugriff am Oktober 26, 2025, [https://www.researchgate.net/publication/385821579\_HybridRAG\_Integrating\_Knowledge\_Graphs\_and\_Vector\_Retrieval\_Augmented\_Generation\_for\_Efficient\_Information\_Extraction](https://www.researchgate.net/publication/385821579_HybridRAG_Integrating_Knowledge_Graphs_and_Vector_Retrieval_Augmented_Generation_for_Efficient_Information_Extraction)  
22. The difference between Kerberos, SAML og OpenID Connect (OIDC) \- Kantega SSO, Zugriff am Oktober 26, 2025, [https://www.kantega-sso.com/articles/the-difference-between-kerberos-saml-og-openid-connect-oidc](https://www.kantega-sso.com/articles/the-difference-between-kerberos-saml-og-openid-connect-oidc)  
23. Graphdatenbanken: Aufbau, Python-Implementierung, Sicherheit  
24. Vektordatenbanken: Aufbau, Python-Implementierung, Sicherheit  
25. HNSWlib: A Graph-based Library for Fast ANN Search \- Zilliz Learn, Zugriff am Oktober 26, 2025, [https://zilliz.com/learn/learn-hnswlib-graph-based-library-for-fast-anns](https://zilliz.com/learn/learn-hnswlib-graph-based-library-for-fast-anns)  
26. CHASE: A Native Relational Database for Hybrid Queries on Structured and Unstructured Data \- arXiv, Zugriff am Oktober 26, 2025, [https://arxiv.org/html/2501.05006v1](https://arxiv.org/html/2501.05006v1)  
27. ArangoDB \- Wikipedia, Zugriff am Oktober 26, 2025, [https://en.wikipedia.org/wiki/ArangoDB](https://en.wikipedia.org/wiki/ArangoDB)  
28. RocksDB: The Bedrock of Modern Stateful Applications \- DZone, Zugriff am Oktober 26, 2025, [https://dzone.com/articles/rocksdb-the-bedrock-of-modern-stateful-application](https://dzone.com/articles/rocksdb-the-bedrock-of-modern-stateful-application)  
29. Saga Pattern in a Microservices Architecture \- Baeldung, Zugriff am Oktober 26, 2025, [https://www.baeldung.com/orkes-conductor-saga-pattern-spring-boot](https://www.baeldung.com/orkes-conductor-saga-pattern-spring-boot)  
30. RocksDB | A persistent key-value store | RocksDB, Zugriff am Oktober 26, 2025, [https://rocksdb.org/](https://rocksdb.org/)  
31. Save RocksDB stored values between runs \- Stack Overflow, Zugriff am Oktober 26, 2025, [https://stackoverflow.com/questions/37790800/save-rocksdb-stored-values-between-runs](https://stackoverflow.com/questions/37790800/save-rocksdb-stored-values-between-runs)  
32. Some key-value storage engines in Rust \- Reddit, Zugriff am Oktober 26, 2025, [https://www.reddit.com/r/rust/comments/zwb4ri/some\_keyvalue\_storage\_engines\_in\_rust/](https://www.reddit.com/r/rust/comments/zwb4ri/some_keyvalue_storage_engines_in_rust/)  
33. How We Built a High Performance Document Store on RocksDB? | Yugabyte, Zugriff am Oktober 26, 2025, [https://www.yugabyte.com/blog/how-we-built-a-high-performance-document-store-on-rocksdb/](https://www.yugabyte.com/blog/how-we-built-a-high-performance-document-store-on-rocksdb/)  
34. Apache DataFusion is Now the Fastest Single Node Engine for Querying Apache Parquet Files | InfluxData, Zugriff am Oktober 26, 2025, [https://www.influxdata.com/blog/apache-datafusion-fastest-single-node-querying-engine/](https://www.influxdata.com/blog/apache-datafusion-fastest-single-node-querying-engine/)  
35. simdjson/simdjson: Parsing gigabytes of JSON per second : used by Facebook/Meta Velox, the Node.js runtime, ClickHouse, WatermelonDB, Apache Doris, Milvus, StarRocks \- GitHub, Zugriff am Oktober 26, 2025, [https://github.com/simdjson/simdjson](https://github.com/simdjson/simdjson)  
36. Understand Data Models \- Azure Architecture Center | Microsoft Learn, Zugriff am Oktober 26, 2025, [https://learn.microsoft.com/en-us/azure/architecture/data-guide/technology-choices/understand-data-store-models](https://learn.microsoft.com/en-us/azure/architecture/data-guide/technology-choices/understand-data-store-models)  
37. The Basics \- Simdjson, Zugriff am Oktober 26, 2025, [https://simdjson.org/api/0.6.0/md\_doc\_basics.html](https://simdjson.org/api/0.6.0/md_doc_basics.html)  
38. Polyglot Persistence: A Comprehensive Guide for Database Developers Transitioning to Microservices Architecture, Zugriff am Oktober 26, 2025, [https://thedeveloperspace.com/polyglot-persistence/](https://thedeveloperspace.com/polyglot-persistence/)  
39. The Basics \- Simdjson, Zugriff am Oktober 26, 2025, [https://simdjson.org/api/0.4.0/index.html](https://simdjson.org/api/0.4.0/index.html)  
40. How to parse Complex Nested JSON File · Issue \#1316 \- GitHub, Zugriff am Oktober 26, 2025, [https://github.com/simdjson/simdjson/issues/1316](https://github.com/simdjson/simdjson/issues/1316)  
41. CostFed: Cost-Based Query Optimization for SPARQL Endpoint Federation \- AKSW, Zugriff am Oktober 26, 2025, [https://svn.aksw.org/papers/2018/SEMANTICS\_CostFed/public.pdf](https://svn.aksw.org/papers/2018/SEMANTICS_CostFed/public.pdf)  
42. Configure a Ranger DB: PostgreSQL \- Cloudera Docs, Zugriff am Oktober 26, 2025, [https://docs-archive.cloudera.com/HDPDocuments/HDP3/HDP-3.1.0/installing-ranger/content/configure\_postgresql\_db\_for\_ranger.html](https://docs-archive.cloudera.com/HDPDocuments/HDP3/HDP-3.1.0/installing-ranger/content/configure_postgresql_db_for_ranger.html)  
43. Benchmarking Filtered Approximate Nearest Neighbor Search Algorithms on Transformer-based Embedding Vectors \- arXiv, Zugriff am Oktober 26, 2025, [https://arxiv.org/html/2507.21989v1](https://arxiv.org/html/2507.21989v1)  
44. Vector Database Basics: HNSW | Tiger Data, Zugriff am Oktober 26, 2025, [https://www.tigerdata.com/learn/vector-database-basics-hnsw](https://www.tigerdata.com/learn/vector-database-basics-hnsw)  
45. Deserializing mixed data types with Rust's serde\_json \- Stack Overflow, Zugriff am Oktober 26, 2025, [https://stackoverflow.com/questions/78196238/deserializing-mixed-data-types-with-rusts-serde-json](https://stackoverflow.com/questions/78196238/deserializing-mixed-data-types-with-rusts-serde-json)  
46. rust \- How to build json arrays or objects dynamically with serde\_json? \- Stack Overflow, Zugriff am Oktober 26, 2025, [https://stackoverflow.com/questions/59047280/how-to-build-json-arrays-or-objects-dynamically-with-serde-json](https://stackoverflow.com/questions/59047280/how-to-build-json-arrays-or-objects-dynamically-with-serde-json)  
47. A Cost Model to Optimize Queries over Heterogeneous Federations of RDF Data Sources \- DiVA portal, Zugriff am Oktober 26, 2025, [http://www.diva-portal.org/smash/get/diva2:1799183/FULLTEXT01.pdf](http://www.diva-portal.org/smash/get/diva2:1799183/FULLTEXT01.pdf)  
48. simdjson: Parsing gigabytes of JSON per second : r/cpp \- Reddit, Zugriff am Oktober 26, 2025, [https://www.reddit.com/r/cpp/comments/asy87z/simdjson\_parsing\_gigabytes\_of\_json\_per\_second/](https://www.reddit.com/r/cpp/comments/asy87z/simdjson_parsing_gigabytes_of_json_per_second/)  
49. Using Sled, how do I serialize and deserialize? \- Stack Overflow, Zugriff am Oktober 26, 2025, [https://stackoverflow.com/questions/58358179/using-sled-how-do-i-serialize-and-deserialize](https://stackoverflow.com/questions/58358179/using-sled-how-do-i-serialize-and-deserialize)  
50. Multimodel v. Polyglot Databases \- BigBear.ai, Zugriff am Oktober 26, 2025, [https://bigbear.ai/blog/multimodel-v-polyglot-databases/](https://bigbear.ai/blog/multimodel-v-polyglot-databases/)  
51. rocksmap \- Rust \- Docs.rs, Zugriff am Oktober 26, 2025, [https://docs.rs/rocksmap](https://docs.rs/rocksmap)  
52. nlfiedler/mokuroku: Secondary indices for RocksDB in Rust. \- GitHub, Zugriff am Oktober 26, 2025, [https://github.com/nlfiedler/mokuroku](https://github.com/nlfiedler/mokuroku)  
53. Query Processing Architecture Guide \- SQL Server | Microsoft Learn, Zugriff am Oktober 26, 2025, [https://learn.microsoft.com/en-us/sql/relational-databases/query-processing-architecture-guide?view=sql-server-ver17](https://learn.microsoft.com/en-us/sql/relational-databases/query-processing-architecture-guide?view=sql-server-ver17)  
54. Pre and Post Filtering in Vector Search with Metadata and RAG Pipelines \- DEV Community, Zugriff am Oktober 26, 2025, [https://dev.to/volland/pre-and-post-filtering-in-vector-search-with-metadata-and-rag-pipelines-2hji](https://dev.to/volland/pre-and-post-filtering-in-vector-search-with-metadata-and-rag-pipelines-2hji)  
55. Hybrid Vector-Graph Relational Vector Database For Better Context Engineering with RAG and Agentic AI : r/RAGCommunity \- Reddit, Zugriff am Oktober 26, 2025, [https://www.reddit.com/r/RAGCommunity/comments/1nfqyx3/hybrid\_vectorgraph\_relational\_vector\_database\_for/](https://www.reddit.com/r/RAGCommunity/comments/1nfqyx3/hybrid_vectorgraph_relational_vector_database_for/)  
56. CouchDB in Distributed and Offline Systems | Simplyblock, Zugriff am Oktober 26, 2025, [https://www.simplyblock.io/glossary/what-is-couchdb/](https://www.simplyblock.io/glossary/what-is-couchdb/)  
57. SQL Query Engine \- Dremio, Zugriff am Oktober 26, 2025, [https://www.dremio.com/platform/sql-query-engine/](https://www.dremio.com/platform/sql-query-engine/)  
58. The Hybrid Relational-Graph-Vector Database \- The Hippocampus for LLMs \- CozoDB v0.7, Zugriff am Oktober 26, 2025, [https://docs.cozodb.org/en/latest/releases/v0.6.html](https://docs.cozodb.org/en/latest/releases/v0.6.html)  
59. Apache Knox \- Hadoop Security Swiss Army Knife \- YouTube, Zugriff am Oktober 26, 2025, [https://www.youtube.com/watch?v=G7P4nXzc1Y4](https://www.youtube.com/watch?v=G7P4nXzc1Y4)  
60. Intel TBB Parallelization Overhead \- c++ \- Stack Overflow, Zugriff am Oktober 26, 2025, [https://stackoverflow.com/questions/6784523/intel-tbb-parallelization-overhead](https://stackoverflow.com/questions/6784523/intel-tbb-parallelization-overhead)  
61. Storing IVF indexes on disk · facebookresearch/faiss Wiki \- GitHub, Zugriff am Oktober 26, 2025, [https://github.com/facebookresearch/faiss/wiki/Storing-IVF-indexes-on-disk](https://github.com/facebookresearch/faiss/wiki/Storing-IVF-indexes-on-disk)  
62. The Hybrid Multimodal Graph Index (HMGI): A Comprehensive Framework for Integrated Relational and Vector Search \- arXiv, Zugriff am Oktober 26, 2025, [https://arxiv.org/html/2510.10123v1](https://arxiv.org/html/2510.10123v1)  
63. OpenMP and C++: Reap the Benefits of Multithreading without All the Work | Microsoft Learn, Zugriff am Oktober 26, 2025, [https://learn.microsoft.com/en-us/archive/msdn-magazine/2005/october/openmp-and-c-reap-the-benefits-of-multithreading-without-all-the-work](https://learn.microsoft.com/en-us/archive/msdn-magazine/2005/october/openmp-and-c-reap-the-benefits-of-multithreading-without-all-the-work)  
64. Apache Arrow, Parquet, Flight and Their Ecosystem are a Game Changer for OLAP, Zugriff am Oktober 26, 2025, [https://www.influxdata.com/blog/apache-arrow-parquet-flight-and-their-ecosystem-are-a-game-changer-for-olap/](https://www.influxdata.com/blog/apache-arrow-parquet-flight-and-their-ecosystem-are-a-game-changer-for-olap/)  
65. Survey of Filtered Approximate Nearest Neighbor Search over the Vector-Scalar Hybrid Data \- arXiv, Zugriff am Oktober 26, 2025, [https://arxiv.org/html/2505.06501v1](https://arxiv.org/html/2505.06501v1)  
66. HybridRAG and Why Combine Vector Embeddings with Knowledge Graphs for RAG?, Zugriff am Oktober 26, 2025, [https://memgraph.com/blog/why-hybridrag](https://memgraph.com/blog/why-hybridrag)  
67. Saga Design Pattern \- Azure Architecture Center | Microsoft Learn, Zugriff am Oktober 26, 2025, [https://learn.microsoft.com/en-us/azure/architecture/patterns/saga](https://learn.microsoft.com/en-us/azure/architecture/patterns/saga)  
68. How could WAL （write ahead log） have better performance than write directly to disk?, Zugriff am Oktober 26, 2025, [https://stackoverflow.com/questions/58694102/how-could-wal-write-ahead-log-have-better-performance-than-write-directly-to-d](https://stackoverflow.com/questions/58694102/how-could-wal-write-ahead-log-have-better-performance-than-write-directly-to-d)  
69. Optimizing Write-Ahead Logging (WAL) for Analytics-Driven Workloads \- CelerData, Zugriff am Oktober 26, 2025, [https://celerdata.com/glossary/optimizing-write-ahead-logging](https://celerdata.com/glossary/optimizing-write-ahead-logging)  
70. ZWAL: Rethinking Write-ahead Logs for ZNS SSDs with Zone Appends \- Large Research, Zugriff am Oktober 26, 2025, [https://atlarge-research.com/pdfs/2024-zns-wal.pdf](https://atlarge-research.com/pdfs/2024-zns-wal.pdf)  
71. Migrating Server Storage to SSDs: Analysis of Tradeoffs, Zugriff am Oktober 26, 2025, [https://rowstron.azurewebsites.net/MS/ssd.pdf](https://rowstron.azurewebsites.net/MS/ssd.pdf)  
72. Data Consistency Models: ACID vs. BASE Databases Explained \- Neo4j, Zugriff am Oktober 26, 2025, [https://neo4j.com/blog/graph-database/acid-vs-base-consistency-models-explained/](https://neo4j.com/blog/graph-database/acid-vs-base-consistency-models-explained/)  
73. ACID vs. BASE Database Model: Differences Explained \- phoenixNAP, Zugriff am Oktober 26, 2025, [https://phoenixnap.com/kb/acid-vs-base](https://phoenixnap.com/kb/acid-vs-base)  
74. Apache Ranger \- Alluxio, Zugriff am Oktober 26, 2025, [https://www.alluxio.io/blog/alluxio-and-apache-ranger-best-practices](https://www.alluxio.io/blog/alluxio-and-apache-ranger-best-practices)  
75. Memory Hierarchy In Computer Architecture: All Levels & Examples \- Unstop, Zugriff am Oktober 26, 2025, [https://unstop.com/blog/memory-hierarchy-in-computer-architecture](https://unstop.com/blog/memory-hierarchy-in-computer-architecture)  
76. From Zero to Hero \- A Data Scientist's Guide to Hardware \- CFA UK, Zugriff am Oktober 26, 2025, [https://www.cfauk.org/pi-listing/from-zero-to-hero---a-data-scientists-guide-to-hardware](https://www.cfauk.org/pi-listing/from-zero-to-hero---a-data-scientists-guide-to-hardware)  
77. B-tree \- Wikipedia, Zugriff am Oktober 26, 2025, [https://en.wikipedia.org/wiki/B-tree](https://en.wikipedia.org/wiki/B-tree)  
78. HNSW vs. DiskANN | Tiger Data, Zugriff am Oktober 26, 2025, [https://www.tigerdata.com/learn/hnsw-vs-diskann](https://www.tigerdata.com/learn/hnsw-vs-diskann)  
79. Ranger Usersync, Zugriff am Oktober 26, 2025, [https://o.onslip.net/HDPDocuments/HDP3/HDP-3.0.1/configuring-ranger-authe-with-unix-ldap-ad/content/ranger\_ad\_integration\_ranger\_usersync.html](https://o.onslip.net/HDPDocuments/HDP3/HDP-3.0.1/configuring-ranger-authe-with-unix-ldap-ad/content/ranger_ad_integration_ranger_usersync.html)  
80. Faiss indexes · facebookresearch/faiss Wiki \- GitHub, Zugriff am Oktober 26, 2025, [https://github.com/facebookresearch/faiss/wiki/Faiss-indexes](https://github.com/facebookresearch/faiss/wiki/Faiss-indexes)  
81. A Complete Guide to Filtering in Vector Search \- Qdrant, Zugriff am Oktober 26, 2025, [https://qdrant.tech/articles/vector-search-filtering/](https://qdrant.tech/articles/vector-search-filtering/)  
82. HNSW vs DiskANN: comparing the leading ANN algorithms \- Vectroid Resources, Zugriff am Oktober 26, 2025, [https://www.vectroid.com/resources/HNSW-vs-DiskANN-comparing-the-leading-ANN-algorithm](https://www.vectroid.com/resources/HNSW-vs-DiskANN-comparing-the-leading-ANN-algorithm)  
83. Datenbank-Sicherheit und Benutzerverwaltung  
84. Ranger Policies Overview | Cloudera on Cloud, Zugriff am Oktober 26, 2025, [https://docs.cloudera.com/runtime/7.3.1/security-ranger-authorization/topics/security-ranger-policies-overview.html](https://docs.cloudera.com/runtime/7.3.1/security-ranger-authorization/topics/security-ranger-policies-overview.html)  
85. Memory \- CS Notes, Zugriff am Oktober 26, 2025, [https://notes.eddyerburgh.me/computer-architecture/memory](https://notes.eddyerburgh.me/computer-architecture/memory)  
86. The Memory Hierarchy \- Computer Systems: A Programmer's Perspective, Zugriff am Oktober 26, 2025, [https://csapp.cs.cmu.edu/2e/ch6-preview.pdf](https://csapp.cs.cmu.edu/2e/ch6-preview.pdf)  
87. SHINE: A Scalable HNSW Index in Disaggregated Memory \- arXiv, Zugriff am Oktober 26, 2025, [https://arxiv.org/html/2507.17647v1](https://arxiv.org/html/2507.17647v1)  
88. Sparse Graph Implementation & Performance in C++ \- Stack Overflow, Zugriff am Oktober 26, 2025, [https://stackoverflow.com/questions/15284765/sparse-graph-implementation-performance-in-c](https://stackoverflow.com/questions/15284765/sparse-graph-implementation-performance-in-c)  
89. petgraph \- Rust \- Docs.rs, Zugriff am Oktober 26, 2025, [https://docs.rs/petgraph/](https://docs.rs/petgraph/)  
90. Graphs in Memory \- The Rust Programming Language Forum, Zugriff am Oktober 26, 2025, [https://users.rust-lang.org/t/graphs-in-memory/18414](https://users.rust-lang.org/t/graphs-in-memory/18414)  
91. Performance Comparison of Graph Representations Which Support Dynamic Graph Updates \- arXiv, Zugriff am Oktober 26, 2025, [https://arxiv.org/html/2502.13862v1](https://arxiv.org/html/2502.13862v1)  
92. Graphs in Rust: An Introduction to Petgraph | Depth-First, Zugriff am Oktober 26, 2025, [https://depth-first.com/articles/2020/02/03/graphs-in-rust-an-introduction-to-petgraph/](https://depth-first.com/articles/2020/02/03/graphs-in-rust-an-introduction-to-petgraph/)  
93. petgraph review/tutorial \- Timothy Vladimír Hobbs, Zugriff am Oktober 26, 2025, [https://timothy.hobbs.cz/rust-play/petgraph\_review.html](https://timothy.hobbs.cz/rust-play/petgraph_review.html)  
94. Querying data with federated queries in Amazon Redshift, Zugriff am Oktober 26, 2025, [https://docs.aws.amazon.com/redshift/latest/dg/federated-overview.html](https://docs.aws.amazon.com/redshift/latest/dg/federated-overview.html)  
95. Federated Search vs Unified Search: Which Is Right for You? \- Coveo, Zugriff am Oktober 26, 2025, [https://www.coveo.com/blog/unified-search-vs-federated-search/](https://www.coveo.com/blog/unified-search-vs-federated-search/)  
96. Filtered Vector Search: State-of-the-art and Research Opportunities \- VLDB Endowment, Zugriff am Oktober 26, 2025, [https://www.vldb.org/pvldb/vol18/p5488-caminal.pdf](https://www.vldb.org/pvldb/vol18/p5488-caminal.pdf)  
97. Effortless large-scale image retrieval with FAISS: A hands-on tutorial | UnfoldAI, Zugriff am Oktober 26, 2025, [https://unfoldai.com/effortless-large-scale-image-retrieval-with-faiss-a-hands-on-tutorial/](https://unfoldai.com/effortless-large-scale-image-retrieval-with-faiss-a-hands-on-tutorial/)  
98. Faiss GPU — BGE documentation, Zugriff am Oktober 26, 2025, [https://bge-model.com/tutorial/3\_Indexing/3.1.2.html](https://bge-model.com/tutorial/3_Indexing/3.1.2.html)  
99. rayon-rs/rayon \- A data parallelism library for Rust \- GitHub, Zugriff am Oktober 26, 2025, [https://github.com/rayon-rs/rayon](https://github.com/rayon-rs/rayon)  
100. Getting Started with Intel® Threading Building Blocks (Intel® TBB), Zugriff am Oktober 26, 2025, [https://www.intel.com/content/www/us/en/developer/articles/guide/get-started-with-tbb.html](https://www.intel.com/content/www/us/en/developer/articles/guide/get-started-with-tbb.html)  
101. Beating RocksDB by up to 7x in almost every workload | Unum Blog, Zugriff am Oktober 26, 2025, [https://www.unum.cloud/blog/2022-09-13-ucsb-10tb](https://www.unum.cloud/blog/2022-09-13-ucsb-10tb)  
102. Consistency level choices \- Azure Cosmos DB \- Microsoft Learn, Zugriff am Oktober 26, 2025, [https://learn.microsoft.com/en-us/azure/cosmos-db/consistency-levels](https://learn.microsoft.com/en-us/azure/cosmos-db/consistency-levels)  
103. Query Federation \- CelerData, Zugriff am Oktober 26, 2025, [https://celerdata.com/glossary/query-federation](https://celerdata.com/glossary/query-federation)  
104. Using Ranger to Provide Authorization in CDP | Cloudera on Cloud, Zugriff am Oktober 26, 2025, [https://docs.cloudera.com/runtime/7.3.1/security-ranger-authorization/topics/security-ranger-provide-authorization-cdp.html](https://docs.cloudera.com/runtime/7.3.1/security-ranger-authorization/topics/security-ranger-provide-authorization-cdp.html)  
105. Database Storage Engines: The Real Performance Factor | by Kunal Sinha | CodeToDeploy, Zugriff am Oktober 26, 2025, [https://medium.com/codetodeploy/database-storage-engines-the-real-performance-factor-01b839925e2c](https://medium.com/codetodeploy/database-storage-engines-the-real-performance-factor-01b839925e2c)  
106. Relational vs Document Database: 9 Key Differences\! \- Atlan, Zugriff am Oktober 26, 2025, [https://atlan.com/relational-vs-document-database/](https://atlan.com/relational-vs-document-database/)  
107. Relational vs. Document Database: Key Differences \- EDB Postgres, Zugriff am Oktober 26, 2025, [https://www.enterprisedb.com/blog/relational-vs-document-database](https://www.enterprisedb.com/blog/relational-vs-document-database)  
108. Optimizing Filtered Vector Search in MyScale \- Medium, Zugriff am Oktober 26, 2025, [https://medium.com/@myscale/optimizing-filtered-vector-search-in-myscale-77675aaa849c](https://medium.com/@myscale/optimizing-filtered-vector-search-in-myscale-77675aaa849c)  
109. Multi-Threading in C/C++: Implications on Software Library Design | Karl Rupp, Zugriff am Oktober 26, 2025, [https://www.karlrupp.net/2016/03/multithreading-software-library-design/](https://www.karlrupp.net/2016/03/multithreading-software-library-design/)  
110. TBB concurrent\_vector with openmp \- c++ \- Stack Overflow, Zugriff am Oktober 26, 2025, [https://stackoverflow.com/questions/7683204/tbb-concurrent-vector-with-openmp](https://stackoverflow.com/questions/7683204/tbb-concurrent-vector-with-openmp)  
111. OpenMP vs OneTBB for someone new to multithreading/parallel programming? \- Reddit, Zugriff am Oktober 26, 2025, [https://www.reddit.com/r/cpp\_questions/comments/sb6v6s/openmp\_vs\_onetbb\_for\_someone\_new\_to/](https://www.reddit.com/r/cpp_questions/comments/sb6v6s/openmp_vs_onetbb_for_someone_new_to/)  
112. Simple way to execute two parallel tasks in TBB \- c++ \- Stack Overflow, Zugriff am Oktober 26, 2025, [https://stackoverflow.com/questions/67418633/simple-way-to-execute-two-parallel-tasks-in-tbb](https://stackoverflow.com/questions/67418633/simple-way-to-execute-two-parallel-tasks-in-tbb)  
113. Data Parallelism with Rust and Rayon \- Shuttle.dev, Zugriff am Oktober 26, 2025, [https://www.shuttle.dev/blog/2024/04/11/using-rayon-rust](https://www.shuttle.dev/blog/2024/04/11/using-rayon-rust)  
114. Parallel Processing with Rayon: Optimizing Rust for the Multi-Core Era \- Reddit, Zugriff am Oktober 26, 2025, [https://www.reddit.com/r/rust/comments/1hytilt/parallel\_processing\_with\_rayon\_optimizing\_rust/](https://www.reddit.com/r/rust/comments/1hytilt/parallel_processing_with_rayon_optimizing_rust/)  
115. Federated Query Engines | QuestDB, Zugriff am Oktober 26, 2025, [https://questdb.com/glossary/federated-query-engines/](https://questdb.com/glossary/federated-query-engines/)  
116. A Learned Cost Model-based Cross-engine Optimizer for SQL Workloads \- VLDB Endowment, Zugriff am Oktober 26, 2025, [https://www.vldb.org/2025/Workshops/VLDB-Workshops-2025/CDMS/CDMS25\_06.pdf](https://www.vldb.org/2025/Workshops/VLDB-Workshops-2025/CDMS/CDMS25_06.pdf)  
117. Using Rayon for Simple Parallelization of SQL Queries in Rust \- Casey Primozic, Zugriff am Oktober 26, 2025, [https://cprimozic.net/blog/rust-parallelism-rayon-join/](https://cprimozic.net/blog/rust-parallelism-rayon-join/)  
118. Apache CouchDB \- Wikipedia, Zugriff am Oktober 26, 2025, [https://en.wikipedia.org/wiki/Apache\_CouchDB](https://en.wikipedia.org/wiki/Apache_CouchDB)  
119. I noticed that RocksDB is used very often in OLTP scenarios. What's the OLAP equ... | Hacker News, Zugriff am Oktober 26, 2025, [https://news.ycombinator.com/item?id=18939329](https://news.ycombinator.com/item?id=18939329)  
120. RocksDB: Not A Good Choice for a High-Performance Streaming Platform : r/rust \- Reddit, Zugriff am Oktober 26, 2025, [https://www.reddit.com/r/rust/comments/1e9rmxv/rocksdb\_not\_a\_good\_choice\_for\_a\_highperformance/](https://www.reddit.com/r/rust/comments/1e9rmxv/rocksdb_not_a_good_choice_for_a_highperformance/)  
121. Arrow Flight RPC — Apache Arrow v22.0.0, Zugriff am Oktober 26, 2025, [https://arrow.apache.org/docs/format/Flight.html](https://arrow.apache.org/docs/format/Flight.html)  
122. Powered by | Apache Arrow, Zugriff am Oktober 26, 2025, [https://arrow.apache.org/powered\_by/](https://arrow.apache.org/powered_by/)  
123. Apache DataFusion is now the fastest single node engine for querying Apache Parquet files, Zugriff am Oktober 26, 2025, [https://datafusion.apache.org/blog/2024/11/18/datafusion-fastest-single-node-parquet-clickbench/](https://datafusion.apache.org/blog/2024/11/18/datafusion-fastest-single-node-parquet-clickbench/)  
124. Introducing Apache Arrow Flight: A Framework for Fast Data Transport, Zugriff am Oktober 26, 2025, [https://arrow.apache.org/blog/2019/10/13/introducing-arrow-flight/](https://arrow.apache.org/blog/2019/10/13/introducing-arrow-flight/)  
125. Building an Arrow Flight server \- Notes from a data witch \- Danielle Navarro, Zugriff am Oktober 26, 2025, [https://blog.djnavarro.net/posts/2022-10-18\_arrow-flight/](https://blog.djnavarro.net/posts/2022-10-18_arrow-flight/)  
126. ARCADE: A Real-Time Data System for Hybrid and Continuous Query Processing across Diverse Data Modalities \- arXiv, Zugriff am Oktober 26, 2025, [https://arxiv.org/html/2509.19757v1](https://arxiv.org/html/2509.19757v1)  
127. A Practical Guide to Hybrid Search \- CelerData, Zugriff am Oktober 26, 2025, [https://celerdata.com/glossary/hybrid-search](https://celerdata.com/glossary/hybrid-search)  
128. Proximity searches \- CozoDB v0.7, Zugriff am Oktober 26, 2025, [https://docs.cozodb.org/en/latest/vector.html](https://docs.cozodb.org/en/latest/vector.html)  
129. When should I prefer DuckDB over DataFusion? : r/dataengineering \- Reddit, Zugriff am Oktober 26, 2025, [https://www.reddit.com/r/dataengineering/comments/1bm27px/when\_should\_i\_prefer\_duckdb\_over\_datafusion/](https://www.reddit.com/r/dataengineering/comments/1bm27px/when_should_i_prefer_duckdb_over_datafusion/)  
130. Beyond Simple Retrieval: A Hybrid Graph-Vector RAG System for Enhanced Language Model Understanding | by Frank Morales Aguilera | The Deep Hub | Medium, Zugriff am Oktober 26, 2025, [https://medium.com/thedeephub/beyond-simple-retrieval-a-hybrid-graph-vector-rag-system-for-enhanced-language-model-understanding-714e84191ad7](https://medium.com/thedeephub/beyond-simple-retrieval-a-hybrid-graph-vector-rag-system-for-enhanced-language-model-understanding-714e84191ad7)  
131. My thoughts on choosing a graph databases vs vector databases : r/Rag \- Reddit, Zugriff am Oktober 26, 2025, [https://www.reddit.com/r/Rag/comments/1ka88og/my\_thoughts\_on\_choosing\_a\_graph\_databases\_vs/](https://www.reddit.com/r/Rag/comments/1ka88og/my_thoughts_on_choosing_a_graph_databases_vs/)  
132. Apache Ranger – Introduction, Zugriff am Oktober 26, 2025, [https://ranger.apache.org/](https://ranger.apache.org/)  
133. mikeroyal/Apache-Arrow-Guide \- GitHub, Zugriff am Oktober 26, 2025, [https://github.com/mikeroyal/Apache-Arrow-Guide](https://github.com/mikeroyal/Apache-Arrow-Guide)  
134. Ranger architecture overview | ADPS Arenadata Docs, Zugriff am Oktober 26, 2025, [https://docs.arenadata.io/en/ADPS/current/concept/services/ranger\_arch.html](https://docs.arenadata.io/en/ADPS/current/concept/services/ranger_arch.html)  
135. Apache Ranger with Amazon EMR \- AWS Documentation, Zugriff am Oktober 26, 2025, [https://docs.aws.amazon.com/emr/latest/ManagementGuide/emr-ranger-overview.html](https://docs.aws.amazon.com/emr/latest/ManagementGuide/emr-ranger-overview.html)  
136. Developing an Application with a Parallel Execution \- Intel, Zugriff am Oktober 26, 2025, [https://www.intel.com/content/www/us/en/docs/onetbb/cookbook/2021-6/developing-an-application-with-a-parallel.html](https://www.intel.com/content/www/us/en/docs/onetbb/cookbook/2021-6/developing-an-application-with-a-parallel.html)  
137. arangodb-vs-cassandra, Zugriff am Oktober 26, 2025, [https://arangodb.com/solutions/comparisons/arangodb-vs-cassandra/](https://arangodb.com/solutions/comparisons/arangodb-vs-cassandra/)  
138. Graphs in Rust vs C++ vs C \- Reddit, Zugriff am Oktober 26, 2025, [https://www.reddit.com/r/rust/comments/1eaoscl/graphs\_in\_rust\_vs\_c\_vs\_c/](https://www.reddit.com/r/rust/comments/1eaoscl/graphs_in_rust_vs_c_vs_c/)  
139. Evaluating Memory Models for Graph‐Like Data Structures in the Rust Programming Language: Performance and Usability \- DiVA portal, Zugriff am Oktober 26, 2025, [https://www.diva-portal.org/smash/get/diva2:1463648/FULLTEXT01.pdf](https://www.diva-portal.org/smash/get/diva2:1463648/FULLTEXT01.pdf)  
140. Does Couchbase Server have a binary JSON storage format? \- Stack Overflow, Zugriff am Oktober 26, 2025, [https://stackoverflow.com/questions/73678178/does-couchbase-server-have-a-binary-json-storage-format](https://stackoverflow.com/questions/73678178/does-couchbase-server-have-a-binary-json-storage-format)  
141. What's the Difference Between an ACID and a BASE Database? \- AWS, Zugriff am Oktober 26, 2025, [https://aws.amazon.com/compare/the-difference-between-acid-and-base-database/](https://aws.amazon.com/compare/the-difference-between-acid-and-base-database/)  
142. ELI5: What exactly are ACID and BASE Transactions? : r/compsci \- Reddit, Zugriff am Oktober 26, 2025, [https://www.reddit.com/r/compsci/comments/1kpybrz/eli5\_what\_exactly\_are\_acid\_and\_base\_transactions/](https://www.reddit.com/r/compsci/comments/1kpybrz/eli5_what_exactly_are_acid_and_base_transactions/)  
143. ACID Model vs BASE Model For Database \- GeeksforGeeks, Zugriff am Oktober 26, 2025, [https://www.geeksforgeeks.org/dbms/acid-model-vs-base-model-for-database/](https://www.geeksforgeeks.org/dbms/acid-model-vs-base-model-for-database/)  
144. Polyglot persistence vs multi-model databases for microservices \- CircleCI, Zugriff am Oktober 26, 2025, [https://circleci.com/blog/polyglot-vs-multi-model-databases/](https://circleci.com/blog/polyglot-vs-multi-model-databases/)  
145. Memory hierarchy \- Wikipedia, Zugriff am Oktober 26, 2025, [https://en.wikipedia.org/wiki/Memory\_hierarchy](https://en.wikipedia.org/wiki/Memory_hierarchy)  
146. how to use faiss::read\_index in c++? · Issue \#2104 \- GitHub, Zugriff am Oktober 26, 2025, [https://github.com/facebookresearch/faiss/issues/2104](https://github.com/facebookresearch/faiss/issues/2104)  
147. Benchmarking Vector, Graph and Hybrid Retrieval Augmented Generation (RAG) Pipelines for Open Radio Access Networks (ORAN) \- arXiv, Zugriff am Oktober 26, 2025, [https://arxiv.org/html/2507.03608v1](https://arxiv.org/html/2507.03608v1)