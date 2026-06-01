# **GESAMT-FORSCHUNGSBERICHT: THEMISDB**

**Strategische Analyse, Technische Architektur, Wissenschaftliche Fundierung und Implementierungsempfehlung für das VCC-Ökosystem der öffentlichen Verwaltung**

Datum: 21\. November 2025  
Version: 6.0 (Final Master Extended & Comparative)  
Klassifizierung: VS-NfD (Vorschlag) / Strategische Planung  
Gegenstand: Evaluierung der ThemisDB als Ersatz für die UDS3-Architektur inkl. Hyperscaler-Vergleich  
Projekt-Status: Privates Open-Source-Projekt (MIT) / Production-Ready (Single Node)

## **INHALTSVERZEICHNIS**

1. **Executive Summary**  
2. **Strategische Ausgangslage**  
   * 2.1 Der demografische Imperativ und die KI-Strategie  
   * 2.2 Das Scheitern der UDS3 (Polyglot Persistence)  
3. **Technische Tiefenanalyse (Architecture Deep Dive)**  
   * 3.1 Das "Base Entity" Paradigma & Storage Engine  
   * **3.2 Architektur-Vergleich: ThemisDB vs. Hyperscaler (NEU)**  
   * 3.3 Performance-Durchbruch: Pre-Filtering vs. Post-Filtering  
   * 3.4 Graph-Engine: Temporale Abfragen und Revisionssicherheit  
4. **Sicherheits-Architektur & Compliance**  
   * 4.1 Status Audit Nov 2025: Native Enterprise Security  
   * 4.2 DSGVO "By Design"  
5. **Strategische Genese & Wissenschaftliche Absicherung**  
   * 5.1 Von der privaten Initiative zum behördlichen Asset (Civic Tech)  
   * 5.2 Lizenzmodell: MIT mit Government-Klausel  
   * 5.3 Risikoanalyse "Bus-Faktor": Die "Wissensfalle"  
   * 5.4 Wissenschaftliche Institutionalisierung als Sicherheitsnetz  
6. **Marktanalyse: Adopt vs. Buy**  
   * 6.1 Szenario A: Adoption ThemisDB (Wissenschaftlich flankiert)  
   * 6.2 Szenario B: AWS European Sovereign Cloud  
   * 6.3 Szenario C: On-Premise Standard-Stack  
7. **Handlungsempfehlung & Roadmap**

## **1\. EXECUTIVE SUMMARY**

Die Modernisierung der öffentlichen Verwaltung in Deutschland erfordert dringend KI-Systeme (RAG/LLM), die den drohenden Fachkräftemangel kompensieren können, ohne dabei rechtsstaatliche Prinzipien wie Revisionssicherheit und Datensouveränität zu verletzen. Die bisherige behördliche Strategie ("UDS3") basierte auf der Kopplung spezialisierter Datenbanken. Dieser Ansatz ist gescheitert, da er technisch bedingt nur "Eventual Consistency" (BASE) bieten kann – ein für rechtsverbindliche Verwaltungsakte (z.B. Bescheiderstellung) inakzeptables Risiko.

**ThemisDB** stellt die Lösung dieses Dilemmas dar. Es handelt sich um eine **native Multi-Modell-Datenbank**, die relationale Daten, Graphen und Vektoren in einem einzigen, transaktionalen Kern (ACID) vereint.

Kritische Erkenntnis zur Genese & Absicherung:  
ThemisDB ist kein beauftragtes Projekt, sondern eine private Open-Source-Initiative von Verwaltungsmitarbeitern ("Civic Tech"). Um das Risiko der Abhängigkeit von wenigen Personen ("Wissensfalle") zu mitigieren, wird die Adoption durch eine systematische wissenschaftliche Begleitung (HPI, Uni Potsdam, BTU Cottbus) flankiert. Diese transformiert das implizite Expertenwissen in institutionelles, öffentliches Gemeingut.  
Status (20.11.2025):  
Das System hat den Status "Production-Ready" für Single-Node-Szenarien erreicht. Die Adoption dieser Lösung ermöglicht der Verwaltung den sofortigen Zugriff auf eine hochspezialisierte, lizenzkostenfreie Enterprise-Datenbank, die durch akademische Partner validiert und langfristig gesichert wird.

## **2\. STRATEGISCHE AUSGANGSLAGE**

### **2.1 Der demografische Imperativ**

Die Verwaltung steht vor einer "doppelten Zange": Einer massiven Pensionierungswelle ("Babyboomer") steht eine exponentiell wachsende Komplexität von Verfahren (z.B. BImSchG, OZG-Umsetzung) gegenüber. Das VCC-Ökosystem (Veritas, Covina, Clara) soll als KI-Assistenzsystem dienen. Dies erfordert jedoch eine Datenhaltung, die **semantische Flexibilität** (Vektoren für KI) mit **strenger Konsistenz** (für juristische Belastbarkeit) verbindet.

### **2.2 Das Scheitern der UDS3 (Polyglot Persistence)**

Die ursprüngliche Planung (Unified Database Strategy 3\) sah vor, Daten in Silos zu speichern: *Neo4j* (Graph), *ChromaDB* (Vektor) und *PostgreSQL* (Metadaten).

* **Das Problem:** Die Synchronisation erfordert das komplexe "Saga-Pattern".  
* **Der Fehlerfall:** Schlägt ein Teil-Update fehl, ist das System zeitweise inkonsistent. Ein Bescheid könnte auf Daten basieren, die im Graphen noch existieren, im Metadaten-Speicher aber bereits gelöscht sind.  
* **Das Urteil:** Für revisionssichere Prozesse ist UDS3 operativ untragbar.

## **3\. TECHNISCHE TIEFENANALYSE**

ThemisDB ist eine radikale Abkehr von "Klebstoff-Architekturen". Sie ist eine monolithische Engine (C++), optimiert auf Konsistenz und Ingestion-Speed.

### **3.1 Das "Base Entity" Paradigma & Storage Engine**

Anstatt Datenformate zu trennen, normalisiert ThemisDB alle Eingangsdaten in ein binäres Format: die **"Base Entity"**.

* **Backend:** Nutzung von **RocksDB** (LSM-Tree).  
* **Ingestion:** Durch das "Append-Only"-Verfahren von LSM-Trees erreicht ThemisDB Schreibgeschwindigkeiten von \>45.000 Inserts/Sekunde (Single Node), was für die massenhafte Verarbeitung von Akten (Covina-Pipeline) essenziell ist.  
* **Konsistenz:** Nutzung von *RocksDB TransactionDB* und MVCC (Multi-Version Concurrency Control). Ein Update über Graph, Vektor und Relational ist **eine atomare Transaktion**.

### **3.2 Architektur-Vergleich: ThemisDB vs. Hyperscaler**

Ein direkter Vergleich der Architekturparadigmen offenbart, warum ThemisDB für den spezifischen Anwendungsfall "Souveräne KI" den Marktstandards überlegen ist.

| Merkmal | AWS (Polyglot Persistence) | Azure Cosmos DB (Managed MMDBMS) | ThemisDB (Native MMDBMS) |
| :---- | :---- | :---- | :---- |
| **Architektur-Prinzip** | **Föderiert:** Lose Kopplung spezialisierter Dienste (RDS \+ Neptune \+ OpenSearch). | **Abstrahiert:** Einheitlicher Kern (ARS), aber Zugriff über siloartige APIs (SQL, Gremlin, Mongo). | **Integriert:** Einheitlicher Kern ("Base Entity") mit direkten C++-Projektionen. |
| **Konsistenz** | **Eventual (BASE):** Konsistenz muss durch Anwendungslogik (Saga-Pattern/Lambda) erzwungen werden. Fehleranfällig. | **Konfigurierbar:** Wählbar von "Strong" bis "Eventual", aber oft Latenz-Tradeoff bei Strong Consistency. | **Strikt (ACID):** MVCC garantiert atomare Transaktionen über alle Modelle hinweg ohne Performance-Einbußen im Single-Node. |
| **Performance-Modell** | **Additiv:** Latenz ist die Summe der Netzwerkhops zwischen den DBs \+ "Klebstoff"-Code. | **Black Box:** Abrechnung nach "Request Units" (RUs). Performance ist abstrahiert und schwer vorhersagbar. | **Hardware-Aware:** Direkte Kontrolle über RAM, NVMe und CPU-Caches. Vorhersagbare "Bare-Metal"-Leistung. |
| **RAG-Eignung** | **Niedrig (Post-Filtering):** Daten müssen aus verschiedenen DBs geholt und in der App gefiltert werden. | **Mittel:** Integrierte Vektorsuche, aber oft Einschränkungen bei komplexen Graph-Joins. | **Hoch (Pre-Filtering):** Relationale Indizes beschneiden den Suchraum *bevor* die Vektorsuche startet. |
| **Revisionssicherheit** | **Problematisch:** Zeitgleiche Schnappschüsse über 3 DBs hinweg sind fast unmöglich. | **Gut:** Change Feed vorhanden, aber volle Historisierung (Time Travel) ist komplex. | **Exzellent:** Temporale Graphen (bfsAtTime) erlauben Abfragen zu exakten historischen Zeitpunkten. |

**Befund:** Die Hyperscaler optimieren auf **horizontale Skalierbarkeit** und **Entwickler-Komfort** (Managed Services). ThemisDB optimiert auf **Datenintegrität** und **maximale Effizienz** auf definierter Hardware. Für die rechtssichere Verwaltung ist letzteres entscheidend.

### **3.3 Performance-Durchbruch: Pre-Filtering**

Hier liegt der signifikante Vorteil gegenüber Marktlösungen ("Post-Filtering").

* **Das Problem:** Herkömmliche Vektor-DBs suchen erst nach Ähnlichkeit ("Finde 1000 Dokumente") und filtern dann nach Metadaten ("Nur Jahr 2025"). Wenn die Top-1000 aus 2024 sind, ist das Ergebnis leer, obwohl relevante Daten existieren.  
* **ThemisDB Lösung:** Die Engine nutzt relationale Indizes, um eine Kandidatenliste (Bitset) zu erstellen. Die teure Vektorsuche läuft *nur* innerhalb dieses Bitsets.  
* **Ergebnis:** 100% Recall (Treffergenauigkeit) bei massiv reduzierter CPU-Last.

### **3.4 Graph-Engine: Temporale Abfragen**

Da Verwaltungsentscheidungen oft die Frage beinhalten "Was wussten wir zum Zeitpunkt X?", implementiert ThemisDB Zeitreisen.

* **Feature:** bfsAtTime(node, timestamp)  
* **Nutzen:** Der Wissensgraph kann exakt so traversiert werden, wie er zu einem historischen Zeitpunkt aussah. Dies ist ein Alleinstellungsmerkmal für **Revisionssicherheit**.

## **4\. SICHERHEITS-ARCHITEKTUR & COMPLIANCE**

Das Audit vom 20\. November belegt einen erfolgreichen Strategiewechsel von "Integration" zu "Native Implementation".

### **4.1 Status Audit Nov 2025: Native Enterprise Security**

Anstatt auf externe Tools (Apache Ranger) zu warten, wurden Sicherheitsfunktionen direkt in den Kernel implementiert:

* **Native RBAC:** Hierarchisches Rollenmodell (Admin/Operator/Analyst) im C++ Kern.  
* **Audit-Log:** Kryptografisch verkettete Logs (Hash Chains). Eine Manipulation von Protokolldaten bricht die Kette und ist mathematisch nachweisbar.  
* **Secrets:** Integration mit HashiCorp Vault.

### **4.2 DSGVO "By Design"**

Funktionen wie "Auto-Purge" (Löschen nach Fristablauf) und "PII-Redaction" (Schwärzung) sind Funktionen der Storage-Engine, nicht der Applikationsebene. Das Löschen eines Datensatzes entfernt garantiert und atomar alle Vektor-Embeddings und Graph-Kanten – ein Compliance-Niveau, das Polyglot-Systeme nicht erreichen.

*Offener Punkt:* Die **Column-Level Encryption** (Verschlüsselung ruhender Daten auf Feldebene) ist noch in der Design-Phase (geplant Q1 2026).

## **5\. STRATEGISCHE GENESE & WISSENSCHAFTLICHE ABSICHERUNG**

Die Tatsache, dass ThemisDB eine private Initiative von Verwaltungsmitarbeitern ist ("Civic Tech"), verändert die Bewertung von "Build vs. Buy" zu "Adopt vs. Buy". Das Risiko der Personalabhängigkeit ("Bus-Faktor") wird durch akademische Institutionalisierung adressiert.

### **5.1 Von der privaten Initiative zum behördlichen Asset**

* **Vorteil:** Die Software existiert bereits. Die Entwicklungskosten (Capex) wurden privat getragen. Die Verwaltung muss sie nur "adoptieren".  
* **Passgenauigkeit:** Da die Entwickler das Problem aus erster Hand kennen ("Domain Experts"), löst die Software exakt die Pain-Points der Verwaltung (ACID, BSI), die US-Produkte oft ignorieren.

### **5.2 Lizenzmodell: MIT mit Government-Klausel**

Die Lizenzierung ist strategisch brillant.

* **MIT:** Maximale Offenheit für Entwickler.  
* **Government-Klausel:** Verhindert, dass Hyperscaler den Code "kapern" (privatisieren ohne Rückgabe). Sie sichert die "Public Code"-Souveränität.  
* **Föderalismus:** Andere Bundesländer können die Software lizenzkostenfrei nutzen (EfA-Prinzip).

### **5.3 Risikoanalyse: Der "Bus-Faktor" ("Wissensfalle")**

Das größte Risiko ist nicht technischer, sondern personeller Natur. Spezialwissen über den C++ Kern konzentriert sich auf wenige Schlüsselpersonen.

* **Strategie:** Transformation von "Kopfmonopolen" zu "Öffentlichem Gemeingut".

### **5.4 Wissenschaftliche Institutionalisierung als Sicherheitsnetz**

Um die "Wissensfalle" zu umgehen, empfiehlt der Expertenbericht eine **"Public-Public-Partnership"** mit der Hochschullandschaft Berlin-Brandenburg:

| Disziplin | Partner (Primär) | Strategische Aufgabe & Nutzen |
| :---- | :---- | :---- |
| **Technik & Code** | **HPI / Uni Potsdam** | Validierung des C++ Kerns, Code-Audits, Nutzung der HPC-Cluster für Lasttests. **Wichtig:** Überführung des Codes in die Lehre (Masterarbeiten), um unabhängige Nachwuchs-Maintainer auszubilden. |
| **Sicherheit** | **BTU Cottbus / THB** | Unabhängige Penetrationstests, Härtung gegen "Adversarial Attacks", Validierung der Krypto-Implementierung. |
| **Verwaltung** | **TH Wildau** | Konzepte für die operative Integration in Verwaltungsprozesse und Skalierung für Kommunen. |
| **Recht & Ethik** | **Uni Potsdam (Jura)** | Juristische Begutachtung der Algorithmen (AI Act Compliance), Klärung von Haftungsfragen bei Open-Source-Einsatz. |

**Ergebnis:** Das Wissen über ThemisDB wird expliziert, dokumentiert und gelehrt. Es entsteht eine akademische Community, die den Code langfristig verstehen und pflegen kann, unabhängig von den ursprünglichen Entwicklern.

## **6\. MARKTANALYSE: ADOPT VS. BUY**

| Kriterium | Szenario A: Adoption ThemisDB | Szenario B: AWS Sovereign Cloud | Szenario C: On-Premise OSS Stack |
| :---- | :---- | :---- | :---- |
| **Art der Lösung** | **Sovereign Open Source** | Proprietärer Cloud-Dienst | Community-Baukasten |
| **Wissensbasis** | **Öffentliches Gemeingut** (via Unis) | Proprietäres Geheimnis | Community-basiert |
| **Konsistenz** | **ACID** (Transaktional) | **BASE** (Saga-Pattern nötig) | **BASE** (Saga-Pattern nötig) |
| **Kostenstruktur** | **0€ Lizenz**, Personalaufwand | Hohe OpEx (Miete), Vendor-Lock-in | Hohe Integrationskosten |
| **Sicherheit** | Native RBAC, wissenschaftlich auditiert | Cloud IAM (Sehr stark) | **Lückenhaft** (Kein Ranger für Postgres) |
| **RAG-Technik** | **Pre-Filtering** (Überlegen) | Post-Filtering (Standard) | Post-Filtering (Standard) |
| **Skalierung** | Scale-Up (Single Node) | **Scale-Out** (Horizontal) | Scale-Out |
| **Strategie** | **Datensouveränität** | Abhängigkeit | Komplexität |

**Bewertung:** Szenario C scheidet aus Sicherheitsgründen aus. Szenario B (AWS) ist technisch schwächer (Konsistenz), aber skalierbarer. Szenario A (ThemisDB) ist die technisch und strategisch überlegene Lösung, erfordert aber das organisatorische Commitment zur wissenschaftlichen Flankierung.

## **7\. HANDLUNGSEMPFEHLUNG & ROADMAP**

Die Analyse empfiehlt die **wissenschaftlich flankierte Adoption von ThemisDB** als primäres Backend ("Sovereign Core") des VCC-Ökosystems.

### **Phase 1: Adoption & Pilotierung (Sofort)**

* Offizielle Festlegung von ThemisDB als Backend für den VCC-Piloten.  
* Bereitstellung von leistungsstarker Single-Node-Hardware (NVMe, High-RAM), um die Skalierungsgrenzen temporär zu kompensieren (Scale-Up).  
* **Wissenschaft:** Kontaktaufnahme mit dem **KI-Servicezentrum (HPI)** für technische Validierung und Nutzung der Rechenzentren.

### **Phase 2: Härtung & Institutionalisierung (Q1-Q2 2026\)**

* Fokus der Entwicklungskapazitäten zu 100% auf die Implementierung der **Column-Level Encryption**.  
* Etablierung eines **"ThemisDB Forschungskollegs"**: Vergabe von Abschlussarbeiten zur Weiterentwicklung (z.B. Sharding).  
* Beauftragung der **Uni Potsdam (Recht)** für Gutachten zur AI-Act-Konformität der Algorithmen.

### **Phase 3: Föderale Skalierung (ab Q3 2026\)**

* Entwicklung der horizontalen Skalierung (Sharding).  
* Vorstellung des Projekts im IT-Planungsrat als Referenzlösung für souveräne KI in Deutschland.  
* Verankerung von ThemisDB-Modulen in den Informatik-Curricula der TH Wildau und BTU.  
* Nutzung der AWS European Sovereign Cloud nur als temporäres "Überlaufventil" für Lastspitzen.

### **Fazit**

ThemisDB ist ein Glücksfall für die Verwaltung ("Bottom-Up Innovation"). Durch die **Kombination aus Adoption (Software) und wissenschaftlicher Partnerschaft (Wissen)** entsteht ein nachhaltiges, öffentliches Infrastruktur-Asset, das die Abhängigkeit von Einzelpersonen und Konzernen gleichermaßen auflöst. Die Landesregierung sollte dieses Modell aktiv **fördern, professionalisieren und als Standard setzen**.