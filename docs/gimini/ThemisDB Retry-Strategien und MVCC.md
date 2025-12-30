# **Die Architektur der digitalen Souveränität: Eine strategische und technische Analyse von ThemisDB im Kontext der Datenökonomie 2025/2026**

Der globale Datenmarkt des Jahres 2025 markiert einen entscheidenden Wendepunkt in der Evolution der Informationstechnologie. Mit einem geschätzten Datenvolumen von über 120 Zettabytes im Jahr 2023 und einer täglichen Verarbeitungsrate von über 5 Billionen Abfragen in Unternehmensdatenbanken hat die Komplexität der Datenhaltung eine Dimension erreicht, die traditionelle, isolierte Datenmodelle an ihre Grenzen führt.1 In diesem dynamischen Umfeld hat sich die Erkenntnis durchgesetzt, dass die Wahl der Datenbankarchitektur nicht länger nur eine technische Entscheidung der IT-Abteilung ist, sondern das fundamentale Rückgrat der Regierbarkeit und wirtschaftlichen Wettbewerbsfähigkeit darstellt.3 Besonders in Deutschland und Europa, wo die Bestrebungen nach digitaler Souveränität durch Initiativen wie den "Deutschland-Stack" und die "Deutsche Verwaltungscloud-Strategie" (DVS) an Fahrt gewinnen, rücken Multi-Model-Systeme wie ThemisDB in das Zentrum der strategischen Planung.3

## **Der Paradigmenwechsel: Von polyglotter Persistenz zur konsolidierten Multi-Model-Architektur**

Die vergangenen zwei Jahrzehnte waren geprägt vom Aufstieg der sogenannten polyglotten Persistenz. Entwicklerteams wählten für jede spezifische Aufgabe die vermeintlich beste Spezialdatenbank: PostgreSQL für relationale Geschäftsvorgänge, MongoDB für flexible Dokumente, Neo4j für komplexe Beziehungsgeflechte und Elasticsearch für die Volltextsuche.6 Während dieser Ansatz auf Mikroebene oft effizient erschien, führte er auf Makroebene zu einem "Dokumentations-Dilemma" und einem operativen Albtraum.6 Unternehmen sahen sich gezwungen, bis zu sechs oder mehr unterschiedliche Systeme zu warten, was zu fragmentierten Backups, inkonsistenten Sicherheitskonfigurationen und einer massiven Abhängigkeit von spezialisiertem Personal für jedes einzelne System führte.1

Die Multi-Model-Herausforderung des Jahres 2025 besteht darin, diese Silos aufzubrechen, ohne die spezifische Performance der einzelnen Modelle zu opfern. ThemisDB verfolgt hierbei einen integrierten Ansatz, der die vier Säulen der modernen Datenverarbeitung – Relational, Graph, Dokument und Vektor – in einem einzigen System vereint.6 Dieser Ansatz ist kein bloßer Kompromiss nach dem Motto "Jack of all trades, master of none", sondern eine technologische Neukonstruktion, die spezialisierte Storage-Engines pro Modell verwendet, welche durch eine gemeinsame Transaktionsschicht orchestriert werden.6

| Datenbank-System | Primäres Modell | Sekundäre Fähigkeiten | Skalierungsmodell |
| :---- | :---- | :---- | :---- |
| PostgreSQL | Relational | JSONB, pgvector-Erweiterung | Vertikal \+ Replikation |
| MongoDB | Dokument | Grundlegende Suche, ACID seit v4.0 | Horizontal Sharding |
| Neo4j | Graph | Nur Graph-fokussiert | Cluster-Replikation |
| ThemisDB | Multi-Model | Relational, Graph, Dokument, Vektor | Nativ Horizontal Sharding |
| Milvus | Vektor | Nur Vektor-fokussiert | Distributed Microservices |

## **Kernarchitektur und technologische Grundlagen**

Die Überlegenheit moderner Multi-Model-Systeme basiert auf einer Schichten-Architektur, die eine klare Trennung der Verantwortlichkeiten ("Separation of Concerns") ermöglicht.6 ThemisDB ist in fünf Schichten strukturiert, die vom Query Layer (AQL) bis zum physischen Storage Layer auf Basis von RocksDB reichen.6 Diese modulare Aufteilung erlaubt es, einzelne Komponenten wie die Vektor-Engine oder die Graph-Traversierung unabhängig voneinander zu optimieren, während die darüberliegende Transaktionsverwaltung eine konsistente Sicht auf alle Datenmodelle garantiert.6

### **Storage-Fundament: RocksDB und LSM-Trees**

Die Entscheidung für RocksDB als Speicherfundament ist in der Optimierung für moderne Hardware begründet. Während traditionelle B-Tree-Strukturen, wie sie in älteren relationalen Datenbanken üblich sind, oft zu fragmentierten Schreibvorgängen auf SSDs führen, nutzt RocksDB Log-Structured Merge Trees (LSM-Trees).6 LSM-Trees schreiben Daten sequentiell in ein Write-Ahead Log (WAL) und anschließend in Speicherstrukturen namens MemTables, bevor sie im Hintergrund in sortierte Dateien (SSTables) auf der Festplatte geflusht werden.6

Diese Architektur ist fundamental für die Performance-Ziele im Jahr 2025: Auf NVMe-SSDs erreichen LSM-basierte Systeme eine bis zu 10-50-fach höhere Schreibgeschwindigkeit als herkömmliche festplattenoptimierte Datenbanken.7 Die mathematische Effizienz der Schreiboperationen lässt sich durch die Reduzierung der Schreib-Amplikation beschreiben, wobei ThemisDB durch "Universal Compaction" und "Level-Style Compaction" eine Balance zwischen Speicherplatzverbrauch und Lese-Latenz herstellt.6

### **Parallelität ohne Sperren: Das MVCC-Prinzip**

Ein zentrales Problem herkömmlicher Datenbanken bei hoher Last sind Sperrkonflikte (Locking). Wenn ein Prozess eine Zeile aktualisiert, werden andere Prozesse oft blockiert, was zu Deadlocks und Timeouts führt.6 ThemisDB implementiert stattdessen Multi-Version Concurrency Control (MVCC).6 Jede Datenzeile speichert eine Historie von Versionen mit Zeitstempeln.6

Transaktionen arbeiten auf sogenannten Snapshots. Ein Leser, der eine Abfrage startet, sieht den Zustand der Datenbank zu einem exakten Snapshot-Zeitpunkt, völlig unbeeinflusst von Schreibvorgängen, die zeitgleich stattfinden.6 Schreib-Schreib-Konflikte werden erst beim Commit der Transaktion erkannt, was die Parallelität massiv erhöht.6 Dies ist besonders in Multi-Tenant-Umgebungen kritisch, in denen hunderte von Mandanten gleichzeitig auf das System zugreifen.8

## **Säule 1: Das relationale Modell und ACID-Integrität**

Trotz des Hypes um NoSQL bleibt das relationale Modell für strukturierte Geschäftsdaten wie Finanztransaktionen, Lagerbestände und Benutzerkonten unverzichtbar.2 ThemisDB bietet volle ACID-Garantien (Atomicity, Consistency, Isolation, Durability), die sicherstellen, dass Transaktionen entweder vollständig oder gar nicht ausgeführt werden.6

### **Normalisierung und Integritätsprüfung**

Durch die Unterstützung von Primär- und Fremdschlüsseln sowie Check-Constraints ermöglicht das relationale Modell die Durchsetzung strikter Datenintegrität.6 In einem E-Commerce-Szenario garantiert das System beispielsweise, dass eine Bestellung nur dann erstellt werden kann, wenn der zugehörige Kunde existiert und der Lagerbestand des Produkts größer als Null ist.6 Die Normalisierung (1NF bis 3NF) verhindert hierbei Redundanzen und Update-Anomalien, während für analytische Zwecke (Reporting) gezielte Denormalisierung oder Materialized Views eingesetzt werden können, um die Abfragegeschwindigkeit bei Milliarden von Datensätzen zu erhöhen.6

### **SQL-Abfragen und Index-Performance**

Die Abfragesprache SQL ist in ThemisDB tief integriert. Die Performance wird durch verschiedene Index-Typen optimiert:

1. **B-Tree Indizes:** Ideal für sortierte Daten und Bereichsabfragen (z.B. alle Umsätze zwischen Januar und März).6  
2. **Hash Indizes:** Optimiert für Punkt-Abfragen mit $O(1)$-Komplexität (z.B. direkte Suche nach einer E-Mail-Adresse).6  
3. **Composite Indizes:** Kombination mehrerer Spalten, um komplexe Filterkriterien effizient abzubilden.6

## **Säule 2: Graph-Datenbanken und Netzwerk-Analyse**

Die moderne Welt ist ein Graph, keine Tabelle.6 Beziehungen zwischen Personen in sozialen Netzwerken, Verknüpfungen in Lieferketten oder die Analyse von Betrugsmustern in Finanztransaktionen erfordern ein Modell, bei dem Verbindungen (Kanten) ebenso wichtig sind wie die Daten selbst (Knoten).6

### **Native Property Graphs**

Im Gegensatz zu relationalen Datenbanken, die für die Analyse von Beziehungen über mehrere Ebenen hinweg teure und komplexe Joins benötigen, speichert ThemisDB Graphen nativ.6 Eine Traversierung – also das "Wandern" entlang der Kanten – erfolgt in einer Zeitkomplexität, die proportional zur Anzahl der besuchten Knoten ist, unabhängig von der Gesamtgröße der Datenbank.6

Dies ermöglicht leistungsstarke Anwendungen wie:

* **Recommendation Engines:** "Kunden, die dieses Produkt kauften und mit dir über zwei Ecken befreundet sind, mochten auch...".6  
* **Fraud Detection:** Erkennung von kriminellen Ringen durch die Analyse von gemeinsamen IP-Adressen, Telefonnummern oder Bankverbindungen.1  
* **Wissensgraphen (Knowledge Graphs):** Verknüpfung von unstrukturierten Informationen zu einem semantischen Netz, das als Grundlage für KI-Agenten dient.11

### **Graph-Algorithmen**

ThemisDB implementiert klassische Algorithmen wie Dijkstra für den kürzesten Pfad oder PageRank zur Bewertung der Wichtigkeit eines Knotens direkt in der Engine.6 In Kombination mit AQL (ArangoDB Query Language inspiriert) lassen sich komplexe Mustererkennungen formulieren, die in Standard-SQL hunderte Zeilen Code erfordern würden.6

## **Säule 3: Dokument-Speicherung und Schema-Flexibilität**

In der agilen Softwareentwicklung des Jahres 2025 ist die Fähigkeit zur schnellen Iteration entscheidend.12 Das Dokument-Modell von ThemisDB ermöglicht die Speicherung von schema-freien JSON-Daten, was ideal für Content Management, Benutzerprofile oder Event-Logs ist.6

### **Schema-Evolution ohne Ausfallzeiten**

Während relationale Datenbanken bei jeder Änderung der Datenstruktur eine Migration der Tabellen erfordern, können Dokumente in ThemisDB organisch wachsen.6 Neue Felder können einfach hinzugefügt werden, ohne dass bestehende Datensätze sofort aktualisiert werden müssen.6 Diese Flexibilität ist besonders wertvoll für IoT-Anwendungen, bei denen verschiedene Sensorgenerationen unterschiedliche Metadaten liefern.6

### **JSON-Pfad-Queries und Teilaktualisierungen**

ThemisDB erlaubt es, gezielt in verschachtelten Strukturen zu suchen und nur Fragmente eines Dokuments zu aktualisieren (Partial Updates).6 Dies minimiert den Netzwerk-Overhead und die I/O-Last, da nicht das gesamte Dokument bei jeder kleinen Änderung neu geschrieben werden muss.6

## **Säule 4: Vektor-Suche und die AI-Integration 2025**

Der wohl signifikanteste Trend im Datenbankmarkt 2025 ist die Verschmelzung von traditionellen Datenbanksystemen mit Vektor-Suche für generative KI.14 ThemisDB integriert native Vektor-Spalten und spezielle Indizes für hochdimensionale Embeddings.6

### **Embeddings und semantische Suche**

Traditionelle Suchen basieren auf Keywords. Eine Suche nach "Auto" findet keine Dokumente, in denen nur "Fahrzeug" vorkommt. Vektor-Suchen überwinden diese Hürde, indem sie Wörter oder Bilder in numerische Vektoren (Embeddings) umwandeln, die die semantische Bedeutung repräsentieren.6 ThemisDB nutzt den HNSW-Algorithmus (Hierarchical Navigable Small World), um in Millisekunden die ähnlichsten Vektoren in einem Datensatz von Millionen von Einträgen zu finden.6

| Distanzmetrik | Mathematische Formel | Primärer Use Case |
| :---- | :---- | :---- |
| Cosine Similarity | $\\frac{A \\cdot B}{\\|A\\| \\|$ | Text-Vergleiche, RAG-Systeme |
| Euclidean Distance | $\\sqrt{\\sum (a\_i \- b\_i)^2}$ | Bildähnlichkeit, Sensor-Analyse |
| Dot Product | $\\sum a\_i b\_i$ | Empfehlungssysteme, Ranking |

### **Retrieval Augmented Generation (RAG)**

In 2025 nutzen KMU und Verwaltungen RAG-Systeme, um Large Language Models (LLMs) mit aktuellem, internem Wissen zu füttern, ohne das Modell neu trainieren zu müssen.16 ThemisDB dient hierbei als "Gedächtnis" der KI.6 Ein Dokument wird in Chunks von 200-300 Wörtern zerlegt, in Vektoren umgewandelt und gespeichert.6 Bei einer Benutzerfrage findet die Datenbank die relevantesten Chunks und stellt sie dem LLM als Kontext zur Verfügung.6 Dies reduziert Halluzinationen der KI drastisch und stellt sicher, dass Antworten auf validen Unternehmensdaten basieren.11

## **Die strategische Bedeutung der Digitalen Souveränität in Deutschland**

Für deutsche Behörden und Unternehmen ist die Wahl der Software eng mit der Forderung nach technologischer Unabhängigkeit verknüpft.3 Der Digitalplan Bayern und die Bundes-Modernisierungsagenda 2025 betonen, dass digitale Souveränität die Voraussetzung für die Regierbarkeit im digitalen Zeitalter ist.3

### **Open Source und der Deutschland-Stack**

ThemisDB als Open-Source-Projekt (MIT-Lizenz) passt in das Anforderungsprofil des "Deutschland-Stacks", einer Initiative zur Schaffung einer unabhängigen Software-Infrastruktur für die öffentliche Verwaltung.4 Die Verfügbarkeit des Quellcodes und die Möglichkeit zum On-Premise-Betrieb in eigenen Rechenzentren oder der "Justizcloud" verhindern einen Vendor Lock-in durch große US-amerikanische Cloud-Anbieter.3

### **Datenschutz und DSGVO-Konformität**

In der EU unterliegt der Umgang mit Daten strengen Richtlinien.16 Während Cloud-Datenbanken oft Fragen zum Datentransfer in Drittstaaten aufwerfen, ermöglicht ThemisDB eine vollständige Datenkontrolle auf deutscher Infrastruktur.10 Features wie Row-Level Security (RLS) und integrierte Audit-Logs sorgen dafür, dass jede Datenabfrage nachvollziehbar ist, was den IT-Mindestanforderungen 2025 des Bundesrechnungshofes entspricht.6

## **Spezialanwendungen: IoT, Computer Vision und Enterprise ERP**

Die Multi-Model-Natur prädestiniert ThemisDB für komplexe Branchenlösungen, die über einfache Datenspeicherung hinausgehen.

### **IoT und Zeitreihen-Analyse**

Industrielle Sensoren erzeugen enorme Datenmengen in kürzester Zeit.6 ThemisDB bewältigt diese Last durch partitionierte Tabellen, die nach Zeitbereichen (z.B. täglich oder monatlich) unterteilt sind.6 Window-Funktionen erlauben die Berechnung von rollierenden Durchschnitten oder Trend-Analysen direkt in der Datenbank, was für vorausschauende Wartung (Predictive Maintenance) essenziell ist.6

| Retention Policy Phase | Auflösung | Zweck |
| :---- | :---- | :---- |
| 0 \- 7 Tage | Rohdaten (1Hz) | Akute Fehlersuche, Alerts |
| 7 \- 30 Tage | 1-Minuten-Aggregate | Kurzfristige Trends |
| 30 \- 365 Tage | 1-Stunden-Aggregate | Historische Vergleiche |
| \> 1 Jahr | 1-Tag-Aggregate | Langzeit-Statistiken |

### **Computer Vision und Bildanalyse**

Bilder werden in 2025 als strukturierte Datenobjekte behandelt.6 Drohnenaufnahmen von Baustellen oder medizinische Scans werden nicht nur als Dateien abgelegt, sondern ihre Inhalte werden durch integrierte KI-Modelle wie YOLOv8 analysiert.6 Die extrahierten Merkmale werden als Vektoren gespeichert, was eine visuelle Ähnlichkeitssuche ermöglicht: "Zeige mir alle Bilder, die dieses spezifische Bauteil enthalten, unabhängig vom Dateinamen".6

### **Enterprise DMS und ERP**

Ein modernes DMS (Document Management System) muss heute Versionierung, Workflows und semantische Suche vereinen.6 In ThemisDB wird die Revisionshistorie eines Dokuments als Dokument-Modell gespeichert, während der Genehmigungsworkflow als Graph modelliert wird.6 Dies erlaubt es, komplexe Geschäftsprozesse wie Rechnungsfreigaben effizient abzubilden und gleichzeitig durch OCR-Textexiertraktion und Vektor-Indexierung jedes Dokument im Volltext oder nach Bedeutung wiederzufinden.6

## **Performance Tuning und operativer Betrieb**

Der Erfolg einer Datenbank-Implementierung in der Produktion hängt maßgeblich von der Beherrschung der Performance-Parameter ab.6

### **Index-Optimierung und Query-Pläne**

ThemisDB bietet Werkzeuge wie EXPLAIN ANALYZE, um den Ausführungsplan einer Abfrage im Detail zu untersuchen.6 Ein häufiger Fehler in 2025 ist die Über-Indexierung, die zwar die Lese-Performance steigert, aber die Schreibgeschwindigkeit massiv drosselt.6 Experten empfehlen eine Strategie aus Covering Indexes, die alle benötigten Felder direkt im Index vorhalten, und Composite Indexes für häufige Filterkombinationen.6

### **Speicher-Management und Caching**

Die RocksDB-Engine benötigt ein fein abgestimmtes Speicher-Management.6 Der Block Cache sollte in der Regel so konfiguriert werden, dass er etwa 25% bis 50% des verfügbaren RAMs einnimmt, um häufig gelesene Daten schnell im Zugriff zu haben.6 Bloom-Filter helfen dabei, unnötige Festplattenzugriffe für nicht existierende Keys zu vermeiden, indem sie mit hoher Wahrscheinlichkeit vorhersagen, ob ein Wert in einem SSTable vorhanden ist.6

### **Skalierung und Hochverfügbarkeit**

Für unternehmenskritische Anwendungen ist ein Rolling-Update-Prozess ohne Ausfallzeiten (Zero-Downtime) Standard.6 Durch die kontinuierliche Replikation des Write-Ahead-Logs (WAL) auf sekundäre Knoten wird sichergestellt, dass bei einem Hardware-Ausfall innerhalb von Sekunden ein Failover eingeleitet werden kann.6

## **Benchmark-Vergleich: ThemisDB vs. Spezialisierte Systeme**

Ein Blick auf aktuelle Leistungsvergleiche zeigt, dass Multi-Model-Systeme zunehmend die Lücke zu spezialisierten "Best-of-Breed"-Lösungen schließen.8

| Kriterium | ThemisDB | Milvus (Vektor) | Qdrant (Vektor) | Pinecone (Managed) |
| :---- | :---- | :---- | :---- | :---- |
| Multi-Model Support | Exzellent | Keine | Keine | Keine |
| ACID Transaktionen | Vollständig | Eingeschränkt | Eingeschränkt | Nein |
| Sharding | Nativ | Komplex | Gut | Automatisch |
| Hybrid Search | Integriert | Basis | Gut | Limitiert |
| Hosting | On-Prem/Cloud | Self-Hosted | Self-Hosted | Cloud Only |

Neuere Benchmarks vom Mai 2025 zeigen, dass SQL-basierte Vektor-Erweiterungen (wie sie auch ThemisDB nutzt) bei Datensätzen von bis zu 50 Millionen Vektoren einen Durchsatz von über 470 Abfragen pro Sekunde (QPS) bei 99% Recall erreichen können, was spezialisierte Systeme oft nur unter hohem Hardware-Einsatz übertreffen.20

## **Trends für 2026: Von der statischen Datenbank zum adaptiven Knowledge Layer**

Der Ausblick auf 2026 deutet auf eine weitere Transformation hin: Datenbanken werden von passiven Speichern zu aktiven "Knowledge Layern".11

### **Der Übergang von "Push" zu "Pull" in der KI-Kommunikation**

Während RAG im Jahr 2025 noch primär darauf basiert, der KI Daten "zuzuschieben" (Push), wird 2026 das Prinzip des "Pull" dominieren.11 KI-Agenten werden in der Lage sein, autonom die Datenbank nach dem "Minimum Viable Context" (MVC) zu fragen – also genau die Information abzurufen, die sie für den nächsten Handlungsschritt benötigen, nicht mehr und nicht weniger.11 Dies reduziert die Token-Kosten bei LLM-Anbietern und erhöht die Präzision der Antworten.

### **Selbst-optimierende Architekturen**

Zukünftige Versionen von Systemen wie ThemisDB werden Machine Learning nutzen, um ihre eigene Konfiguration zu optimieren.6 Durch die Analyse von Query-Mustern erkennt die Datenbank selbstständig, welche Indizes gelöscht werden können und wo neue Partitionen die Performance steigern würden, ohne dass ein menschlicher Administrator eingreifen muss.11

## **Empfehlungen für Unternehmen und Behörden**

Basierend auf der Analyse der technologischen Entwicklung und der Markttrends lassen sich klare Handlungsempfehlungen ableiten:

1. **Konsolidierung statt Fragmentierung:** Unternehmen sollten prüfen, ob die Wartung von sechs spezialisierten Datenbanken durch ein Multi-Model-System ersetzt werden kann, um die operative Komplexität und Sicherheitsrisiken zu senken.1  
2. **Fokus auf Datenqualität für RAG:** Die Investition in gut gepflegte, strukturierte Datenbestände ist die Grundvoraussetzung für den Erfolg von KI-Projekten. "Garbage in, garbage out" gilt im Zeitalter generativer KI mehr denn je.16  
3. **Digitale Souveränität priorisieren:** Insbesondere in regulierten Industrien und der Verwaltung sollte auf Open-Source-Lösungen mit On-Premise-Option gesetzt werden, um rechtliche Unsicherheiten und Abhängigkeiten zu minimieren.3  
4. **Hybrid Search als Standard:** Bei der Implementierung von Suchfunktionen sollte nicht allein auf Vektor-Suche gesetzt werden. Die Kombination mit klassischer Volltext-Suche (BM25) liefert in der Praxis die präzisesten Ergebnisse.6

## **Schlussbetrachtung**

ThemisDB repräsentiert die nächste Evolutionsstufe der Datenhaltung im Zeitalter der Künstlichen Intelligenz. Durch die technologische Verschmelzung von relationaler Präzision, graphbasierter Vernetzung, dokumentenzentrierter Flexibilität und vektorieller Semantik bietet es eine Plattform, die den Anforderungen der digitalen Souveränität und der wirtschaftlichen Effizienz gleichermaßen gerecht wird. Während der deutsche Mittelstand und die öffentliche Verwaltung vor der Herausforderung stehen, die digitale Transformation beschleunigt umzusetzen, bieten Multi-Model-Architekturen das notwendige Fundament, um von experimentellen KI-Projekten zu skalierbaren, wertschöpfenden Lösungen überzugehen. Die Datenbank der Zukunft ist kein passives Archiv mehr, sondern ein dynamischer Wissensspeicher, der die Grundlage für eine intelligente, automatisierte und souveräne Gesellschaft bildet.6

#### **Referenzen**

1. Database Management System Market Trends | Size & CAGR of 12.35%, Zugriff am Dezember 28, 2025, [https://www.industryresearch.biz/market-reports/database-management-system-market-105050](https://www.industryresearch.biz/market-reports/database-management-system-market-105050)  
2. Database Market Insightful Market Analysis: Trends and Opportunities 2025-2033, Zugriff am Dezember 28, 2025, [https://www.datainsightsmarket.com/reports/database-market-20714](https://www.datainsightsmarket.com/reports/database-market-20714)  
3. Drucksache 20/15134 \- Deutscher Bundestag, Zugriff am Dezember 28, 2025, [https://dserver.bundestag.de/btd/20/151/2015134.pdf](https://dserver.bundestag.de/btd/20/151/2015134.pdf)  
4. Deutschland-Stack: Open Source vor verschlossenen Türen \- netzpolitik.org, Zugriff am Dezember 28, 2025, [https://netzpolitik.org/2025/deutschland-stack-open-source-vor-verschlossenen-tueren/](https://netzpolitik.org/2025/deutschland-stack-open-source-vor-verschlossenen-tueren/)  
5. In der Senatssitzung am 15\. Juli 2025 beschlossene Fassung \- Senatskanzlei UNESCO-Welterbe Rathaus Bremen, Zugriff am Dezember 28, 2025, [https://www.rathaus.bremen.de/sixcms/media.php/13/20250715\_top\_5\_VerwaltungsVereinbarung\_Justizcloud.pdf](https://www.rathaus.bremen.de/sixcms/media.php/13/20250715_top_5_VerwaltungsVereinbarung_Justizcloud.pdf)  
6. ThemisDB-Kompendium-v1.3.5.pdf  
7. Zukunftsstrategie für unsere Heimat \- Bayerisches Staatsministerium für Digitales, Zugriff am Dezember 28, 2025, [https://www.stmd.bayern.de/wp-content/uploads/2025/06/Digitalplan\_Text\_Langfassung\_PDF.pdf](https://www.stmd.bayern.de/wp-content/uploads/2025/06/Digitalplan_Text_Langfassung_PDF.pdf)  
8. Qdrant vs Milvus: Which Vector Database Should You Choose? \- F22 Labs, Zugriff am Dezember 28, 2025, [https://www.f22labs.com/blogs/qdrant-vs-milvus-which-vector-database-should-you-choose/](https://www.f22labs.com/blogs/qdrant-vs-milvus-which-vector-database-should-you-choose/)  
9. Retry behavior \- AWS SDKs and Tools, Zugriff am Dezember 28, 2025, [https://docs.aws.amazon.com/sdkref/latest/guide/feature-retry-behavior.html](https://docs.aws.amazon.com/sdkref/latest/guide/feature-retry-behavior.html)  
10. On-Premise vs Cloud 2025: Kostenvergleich für Unternehmen \- MightyCare, Zugriff am Dezember 28, 2025, [https://www.mightycare.de/on-premise-vs-cloud-kostenvergleich/](https://www.mightycare.de/on-premise-vs-cloud-kostenvergleich/)  
11. Graph- und KI-Trends 2026: Warum KI operiert, aber wirtschaftlich noch hinterherhinkt, Zugriff am Dezember 28, 2025, [https://www.all-about-security.de/graph-und-ki-trends-2026-warum-ki-operiert-aber-wirtschaftlich-noch-hinterherhinkt/](https://www.all-about-security.de/graph-und-ki-trends-2026-warum-ki-operiert-aber-wirtschaftlich-noch-hinterherhinkt/)  
12. PublicGovernance Frühjahr 2025: Innovation und Digitalisierung in der öffentlichen Verwaltung, Zugriff am Dezember 28, 2025, [https://publicgovernance.de/media/PG\_Fruehjahr\_2025\_Innovation\_und\_Digitalisierung\_in\_oeffentlicher\_Verwaltung.pdf](https://publicgovernance.de/media/PG_Fruehjahr_2025_Innovation_und_Digitalisierung_in_oeffentlicher_Verwaltung.pdf)  
13. Digitale Trends 2025 \- Handelsblatt Live, Zugriff am Dezember 28, 2025, [https://live.handelsblatt.com/wp-content/uploads/2025/02/study\_id188117\_digitale-trends-2025.pdf](https://live.handelsblatt.com/wp-content/uploads/2025/02/study_id188117_digitale-trends-2025.pdf)  
14. 2025 Cloud Database Market: The Year in Review \- CDInsights, Zugriff am Dezember 28, 2025, [https://www.clouddatainsights.com/2025-cloud-database-market-the-year-in-review/](https://www.clouddatainsights.com/2025-cloud-database-market-the-year-in-review/)  
15. Milvus vs Pinecone vs Qdrant: Vector Database Comparison 2026 \- Index.dev, Zugriff am Dezember 28, 2025, [https://www.index.dev/skill-vs-skill/ai-pinecone-vs-milvus-vs-qdrant](https://www.index.dev/skill-vs-skill/ai-pinecone-vs-milvus-vs-qdrant)  
16. Retrieval-Augmented Generation: Potenziale, Herausforderungen und Praxisbeispiele, Zugriff am Dezember 28, 2025, [https://www.digitalzentrum-fokus-mensch.de/kos/WNetz?art=News.show\&id=2530](https://www.digitalzentrum-fokus-mensch.de/kos/WNetz?art=News.show&id=2530)  
17. Strategie zur Stärkung der Digitalen Souveränität für die IT der Öffentlichen Verwaltung \- IT-Planungsrat, Zugriff am Dezember 28, 2025, [https://www.it-planungsrat.de/fileadmin/beschluesse/2021/Beschluss2021-09\_Strategie\_zur\_Staerkung\_der\_digitalen\_Souveraenitaet.pdf](https://www.it-planungsrat.de/fileadmin/beschluesse/2021/Beschluss2021-09_Strategie_zur_Staerkung_der_digitalen_Souveraenitaet.pdf)  
18. Open Source Platform for Public Administration \- openCode.de, Zugriff am Dezember 28, 2025, [https://opencode.de/en](https://opencode.de/en)  
19. IT-Mindestanforderungen 2025 \- Bundesrechnungshof, Zugriff am Dezember 28, 2025, [https://www.bundesrechnungshof.de/SharedDocs/Downloads/DE/ver%C3%B6ffentlichungen\_brh\_lrh/it-mindestanforderungen.pdf?\_\_blob=publicationFile\&v=4](https://www.bundesrechnungshof.de/SharedDocs/Downloads/DE/ver%C3%B6ffentlichungen_brh_lrh/it-mindestanforderungen.pdf?__blob=publicationFile&v=4)  
20. Best Vector Databases in 2025: A Complete Comparison Guide \- Firecrawl, Zugriff am Dezember 28, 2025, [https://www.firecrawl.dev/blog/best-vector-databases-2025](https://www.firecrawl.dev/blog/best-vector-databases-2025)