---
Author: "unknown"
Created: "2026-11-20"
Last Updated: "2026-11-21"
Status: "review"
AI_Used: true
AI_Models: "unknown"
AI_Contributor: "GitHub Copilot"
Human_Review: false
Human_Author: "unknown"
---

> Exact model backend not verifiable from final document text alone.
> Der Erstellungszeitpunkt der ursprünglichen Analyse ist nicht verifiziert; `Created` bezeichnet die Erfassung dieser Metadaten.

# Technische Bewertung eines 2PC-Raft-Shard-Konsensmodells für dezentrale Datenbanklandschaften

> **Einordnung, 2026-11-20:** Die folgende KI-gestützte Analyse ist eine Sekundärquelle, kein Implementierungs- oder Freigabenachweis. Die nummerierten Verweise und Modellwerte stammen aus der ursprünglichen Analyse und wurden nicht unabhängig vollständig geprüft. Der Abschnitt „Konkreter Umsetzungsplan“ ergänzt den aktuellen Repository-Stand und hat bei Widersprüchen Vorrang vor den historischen Empfehlungen dieser Analyse. Die verbindliche Quellenreihenfolge bleibt davon unberührt.

Ein dezentrales Konsensmodell, das Two-Phase Commit (2PC) über per Shard organisierte Raft-Konsensgruppen legt, kann die Wiederherstellung nach dem Ausfall eines einzelnen Koordinator-Leaders verbessern: Eine verfügbare Home-Raft-Gruppe kann eine dauerhaft beschlossene Entscheidung durch einen neuen Leader weiter zustellen. Es beseitigt das 2PC-Blocking jedoch nicht, wenn die Home- oder eine benötigte Teilnehmergruppe ihr Quorum verliert. Eine verteilte Koordinatorzuordnung – bei der die Raft-Gruppe des objektbezogenen Home-Shards per Rendezvous Hashing die Transaktionssteuerung übernimmt – vermeidet einen einzigen globalen Koordinationscluster1.
Für die bewerteten ThemisDB-Architekturspezifikationen gilt ein deutlicher Reife- und Umsetzungsvorbehalt: Das skizzierte Raft-gestützte 2PC-Protokoll ist bislang nicht als durchgängig verdrahtete Produktionsimplementierung nachgewiesen1. DIS-05 hat inzwischen `TwoPhaseCommitCoordinator` als kanonischen direkten 2PC-Koordinator für das benannte Koordinatorpaar festgelegt; CrossShard delegiert seinen 2PC-Pfad einschließlich Entscheidungs-Replay an ihn. Andere Koordinatoren und WAL-Domänen bleiben davon getrennt. DIS-05/DIS-06 sind trotz vorhandener Implementierung noch nicht durch alle Build-, Recovery- und Release-Nachweise abgeschlossen. Die verknüpfte Notebook-Rohquelle (notebook://notebooks/9550fd1e-83b5-49c9-9cc8-455b93646edf) war laut ursprünglicher Analyse nicht auslesbar9.
Ob ein 2PC-Raft-Modell hohe Resilienz bei vertretbarem Aufwand verbindet, hängt unter anderem von Fan-out, Fehlerkorrelation, Commit-Pfad und Speicherstrategie ab. Unter dem vereinfachenden Unabhängigkeitsmodell unten sinkt die Verfügbarkeit mit der Zahl benötigter Gruppen. Die synchrone Baseline benötigt mehrere langlebige Konsensschritte; der tatsächliche kritische Pfad ist zu messen. Parallel Commits, Async Commit und Cold-Erasure-Coding sind mögliche Folgearbeiten, deren Kosten und Sicherheitsbedingungen für ThemisDB erst nachzuweisen sind.
Architektonische Einordnung und Protokollmechanik
Klassisches Two-Phase Commit auf Einzelknotenbasis scheitert in verteilten Systemen am Blocking-Problem: Fällt der Koordinator aus, nachdem die Teilnehmer in Phase 1 PREPARED gemeldet und lokale Sperren gesetzt haben, verharren diese Ressourcen im ungewissen Zustand (in-doubt), bis der Koordinator wiederhergestellt ist2. Das 2PC-Raft-Modell löst dieses Problem nicht über ein drittes Protokollstadium, sondern durch Zustandsmaschinen-Replikation der Protokollteilnehmer1. Jeder logische Shard bildet eine eigenständige Raft-Gruppe – typischerweise mit drei oder fünf Replikaten über getrennte Fehlerdomänen hinweg1.
Im Zielentwurf für dezentrale Cross-Shard-Blocktransaktionen verzichtet ThemisDB konsequent auf einen monolithischen globalen Koordinator1. Stattdessen ermittelt Rendezvous Hashing (Highest Random Weight) über Mandanten- und Objekt-ID einen stabilen Home-Shard1. Der jeweilige Raft-Leader dieser Home-Gruppe fungiert als Transaktionskoordinator, protokolliert Transaktions-ID, erwartete Vorversion, beteiligte Shards, Platzierungsepoche sowie die unveränderliche COMMIT- oder ABORT-Entscheidung im eigenen Raft-Log1. Fällt dieser Leader aus, wählt die Home-Raft-Gruppe innerhalb der konfigurierten Election-Timeouts (bei ThemisDB standardmäßig 150–300 ms bei 50 ms Heartbeat-Intervall) einen neuen Leader, der ausstehende Phase-2-Zustellungen idempotent fortsetzt1.
Bemerkenswert ist die klare Abgrenzung des Zielentwurfs gegen 3PC1. Obwohl der bestehende CrossShardTransactionCoordinator noch eine 3PC-Variante mit PreCommit-Phase bereitstellt8, belegen theoretische und praktische Analysen, dass 3PC ein synchrones Netzwerk beziehungsweise ein reines Fail-Stop-Fehlermodell voraussetzt4. Bei realen Netzwerkpartitionen (Asynchronie) können isolierte Teilgruppen oder mehrere Koordinatoren widersprüchliche Commit- und Abort-Entscheidungen treffen und damit die Konsistenz brechen3. Das 2PC-Raft-Modell ordnet sich nach dem PACELC-Theorem hingegen als PC/EC-System ein: Bei Partitionen priorisiert es Konsistenz vor Schreibverfügbarkeit (Minderheiten-Partitionen werden schreibgeschützt oder verweigern Commits), und im Normalbetrieb garantiert es strikte Konsistenz vor minimaler Latenz1.
In der gegenwärtigen Codebasis besteht weiterhin eine erhebliche Lücke zwischen diesem Entwurf und der betrieblichen Realität1. DIS-05 legt `TwoPhaseCommitCoordinator` für das benannte Paar als kanonische Entscheidungsinstanz fest; CrossShard behält seinen öffentlichen Lebenszyklus und andere Protokolle. Coordinator-, Teilnehmer- und Replikations-WAL bleiben getrennt. Andere 2PC-fähige Oberflächen sind nicht durch DIS-05 konsolidiert, und eine Raft-replizierte Entscheidung ist dadurch nicht nachgewiesen. Unaufgelöste ältere CrossShard-2PC-Datensätze blockieren die Wiederherstellung statt eine zweite Entscheidungsquelle zu verwenden. Hinweise auf unbegrenztes WAL-Wachstum und Gossip-Races10 sind aufgrund der nicht auslesbaren Notebook-Quelle nicht unabhängig verifiziert9.
Bewertung der Ausfallsicherheit und Verfügbarkeitsgrenzen
Die Fehlertoleranz eines 2PC-Raft-Modells stützt sich auf das Mehrheitsquorum innerhalb jeder Shard-Gruppe: Eine Gruppe mit 2f + 1 Replikaten verkraftet bis zu f gleichzeitige Knotenausfälle ohne Datenverlust und bei fortlaufendem Schreibbetrieb1. Da bei einer Cross-Shard-Transaktion jedoch alle beteiligten Participant-Gruppen sowie die Home-Koordinator-Gruppe im PREPARED-Schritt ihr jeweiliges Quorum erreichen müssen – eine Mehrheit der betroffenen Gruppen genügt ausdrücklich nicht –, sinkt die Gesamtverfügbarkeit multiplikativ mit der Zahl der beteiligten Shards1.
Unter der vereinfachenden Annahme unabhängiger Knoten-Ausfallwahrscheinlichkeit p gilt für eine Gruppe mit drei Replikaten A3 = (1-p)^3 + 3p(1-p)^2 und für fünf Replikate A5 = (1-p)^5 + 5p(1-p)^4 + 10p^2(1-p)^3. Für S voneinander unabhängige, gleich konfigurierte benötigte Gruppen ergibt sich A_tx = A_group^S. Die Home-Gruppe wird genau einmal mitgezählt, auch wenn sie zugleich Teilnehmer ist. Gemeinsame Infrastruktur, Netzpartitionen und überlappende Replikate verletzen die Unabhängigkeitsannahme; die Rechnung ist keine SLA-Zusage.
Knoten-Ausfallrate p
Replikationsfaktor (RF)
1 Shard (S = 1)
2 Shards (S = 2)
4 Shards (S = 4)
8 Shards (S = 8)
16 Shards (S = 16)
0,1 % (p = 0,001)
RF = 3
99,99970 %
99,99940 %
99,99880 %
99,99760 %
99,99520 %
0,1 % (p = 0,001)
RF = 5
99,999999 %
99,999998 %
99,999996 %
99,999992 %
99,999984 %
1,0 % (p = 0,010)
RF = 3
99,9702 %
99,9404 %
99,8809 %
99,7618 %
99,5243 %
1,0 % (p = 0,010)
RF = 5
99,9990 %
99,9980 %
99,9961 %
99,9921 %
99,9842 %
5,0 % (p = 0,050)
RF = 3
99,2750 %
98,5553 %
97,1314 %
94,3451 %
89,0099 %
5,0 % (p = 0,050)
RF = 5
99,8842 %
99,7685 %
99,5376 %
99,0772 %
98,1630 %

Analytisches Quorum-Verfügbarkeitsmodell bei unabhängigen Fehlerdomänen und strikter Quorum-Pflicht je beteiligtem Shard1.
Die Modellrechnung verdeutlicht die Skalierungsgrenze von Cross-Shard-Transaktionen: Bei einer Knoten-Nichtverfügbarkeit von 1,0 % liegt das Ausfallrisiko eines einzelnen RF=3-Shards bei 0,0298 %, steigt bei 16 beteiligten Shards jedoch auf 0,4757 %15. In Wartungsfenstern oder unter hoher Last mit 5,0 % Knotenausfallrate scheitert eine 16-Shard-Transaktion bei RF=3 in rund 11 % aller Versuche15. Ein Wechsel auf RF=5 senkt die Nichtverfügbarkeit bei S = 16 und p = 0,010 dagegen um rund den Faktor 30 auf 0,0158 %15. Für maximale Resilienz bei vertretbarem Aufwand sollten daher nur kritische Katalog- oder Home-Shards mit fünf Replikaten betrieben werden, während die Anwendungsarchitektur den Cross-Shard-Fan-out S durch objektlokale Datengruppierung eng begrenzt1.
Über das reine Quorum-Risiko hinaus gefährden drei strukturelle Schwachstellen die Ausfallsicherheit einer dezentralen Datenbanklandschaft. Erstens sieht ein Teil der ThemisDB-Dokumentation nach wie vor einen zentralen Sequencer-Dienst als „Single Source of Truth“ für MVCC-Transaktionsnummern über Shards hinweg vor10. Ähnlich wie beim Placement Driver (PD) von TiDB/TiKV, der als einzelner aktiver Timestamp Oracle (TSO) fungiert, bildet ein zentraler Sequencer sowohl einen Latenzengpass bei Cross-Region-Zugriffen als auch einen zentralen Ausfallpunkt bei Regionentrennung24. Zweitens darf ein Timeout während 2PC niemals als automatischer ABORT gewertet werden: Sobald die Home-Gruppe eine COMMIT-Entscheidung per Quorum bestätigt hat, muss diese Entscheidung unveränderlich bleiben; ein Timeout belässt den Client lediglich im Zustand UNKNOWN, den er idempotent gegen das Home-Shard-Ergebnisregister auflösen muss1. Drittens schützt Raft nur bei physisch entkoppelten Fehlerdomänen1; geteilte Netzwerk-Storage-Schichten oder VM-Live-Migrationen können mehrere Replikate gleichzeitig blockieren oder Uhrensprünge auslösen27.
Latenz-, Rechen- und Betriebsaufwände im Protokollvergleich
Ob das 2PC-Raft-Modell auch das Kriterium reduzierter Aufwände erfüllt, hängt maßgeblich von der gewählten Commit-Variante, dem Leseprotokoll, der Speicherhardware und der Uhrensynchronisation ab.
In der streng sequenziellen Ausprägung des ThemisDB-Zielentwurfs schreibt der Client zunächst die Transaktionsankündigung in den Home-Shard, danach loggen alle Teilnehmer ihre Blöcke plus PREPARED-Status über ihr jeweiliges Raft-Quorum, anschließend persistiert der Home-Shard COMMIT, verteilt die Entscheidung in Phase 2 zur erneuten Raft-Persistierung an alle Teilnehmer und veröffentlicht erst nach Erhalt sämtlicher Bestätigungen den neuen Manifest-Zeiger durch einen weiteren Raft-Schreibvorgang1. Dies sind fünf logisch aufeinanderfolgende Persistenzgrenzen, wobei Teilnehmergruppen jeweils parallel arbeiten können; die tatsächliche Zahl der RTTs und fsyncs hängt von Implementierung und Batching ab.
Industrielle Referenzsysteme wie CockroachDB (ab Version 19.2 mit Parallel Commits) und TiKV (Async Commit und 1PC) zeigen, wie sich dieser Aufwand ohne Verlust der Atomarität halbieren lässt11. Bei Parallel Commits repliziert CockroachDB einen STAGING-Zustand des Transaktionsdatensatzes parallel zu den Write Intents über die jeweiligen Raft-Gruppen; sobald der STAGING-Record und sämtliche Intents ihr Quorum erreicht haben, gilt die Transaktion als sicher committet, und die endgültige Statusauflösung läuft asynchron nach der Client-Antwort11. Laut einem Ingenieursbericht und zugehörigen TPC-C-Tests von Cockroach Labs sinkt die Transaktionslatenz damit von zwei auf einen Konsens-Round-Trip11. TiKV geht für Transaktionen innerhalb derselben Region noch weiter und schließt den Vorgang über 1PC bereits in der Prewrite-Phase ohne 2PC ab; bei verteilten Transaktionen mit weniger als 64 Schlüsseln (oder hierarchischer Lock-Struktur) liefert Async Commit die Erfolgsbestätigung ebenfalls nach Abschluss der Prewrites17.
Die folgende analytische Modellierung (unter Annahme einer lokalen fsync-Dauer von 1,0 ms pro Raft-Log-Eintrag sowie 0,5 ms lokalem Lookup bei aktivem Lease) vergleicht den Latenzaufwand über vier Netzwerk-Topologien15:
Topologie-Szenario
Single-Shard Raft (1PC)
Naives Sync-2PC + Publikation (Basis-Zielentwurf)
Async-Phase-2-2PC (Ack nach Decision)
Parallel Commits (1 Konsens-RTT)
Lesezugriff: Leader Lease vs. Durable Reader Pin
Intra-AZ (0,5 ms intra / 0,5 ms inter)
1,5 ms
7,0 ms
3,5 ms
2,0 ms
0,5 ms vs. 1,5 ms
Cross-AZ gleiche Region (1,5 ms intra / 1,5 ms inter)
2,5 ms
13,0 ms
6,5 ms
4,0 ms
0,5 ms vs. 2,5 ms
Multi-Region (40 ms intra / 40 ms inter)
41,0 ms
244,0 ms
122,0 ms
81,0 ms
0,5 ms vs. 41,0 ms
Lokale Shards, Cross-Region-Tx (1,5 ms intra / 50 ms inter)
2,5 ms
110,0 ms
55,0 ms
52,5 ms
0,5 ms vs. 2,5 ms

Analytisches Latenzmodell auf Basis der Protokoll-Round-Trips1 bei t_fsync = 1,0 ms15.
Aus dieser Modellrechnung treten zwei deutliche Kostentreiber im aktuellen ThemisDB-Zielentwurf hervor. Erstens verdoppelt bis verdreifacht das sequenzielle Abwarten von Phase 2 und Manifest-Publikation die Schreiblatenz gegenüber Parallel Commits (13,0 ms statt 4,0 ms im Cross-AZ-Betrieb; 244,0 ms statt 81,0 ms im Multi-Region-Fall)1. Zweitens verlangt der Zielentwurf für Leseoperationen einen dauerhaften Reader Pin über das Home-Raft-Quorum, damit parallele Block-Reads nicht durch Garbage Collection unterbrochen werden1. Dies verteuert jeden Lesevorgang auf einen vollwertigen Raft-Write (41,0 ms im Multi-Region-Setup gegenüber 0,5 ms bei einem lokalen Leader Lease) und bürdet dem Betrieb zusätzlich das explizite Widerrufen verwaister Pins nach Client-Abstürzen auf1.
Hardware-, Transport- und Zeitsynchronisationsaufwand
Da Raft jeden Log-Anhang vor der Bestätigung dauerhaft auf nichtflüchtigen Speicher schreiben muss, bestimmen lokale Disk-Latenzen die Untergrenze des Systemaufwands1. Einem unabhängigen Kubernetes-Self-Hosting-Benchmark für TiDB/TiKV (OpenEverest-Blog) zufolge weist Netzwerk-Block-Storage eine mittlere fsync-Latenz von 2,9 ms (p99: 8,2 ms) auf, während node-lokaler NVMe-Speicher bei 0,19 ms (p99: 0,31 ms) liegt28. Im selben Benchmark kostete obligatorisches TLS zwischen allen Knoten bei CPU-gesättigten SQL-Pods rund 6,9 % Transaktionsdurchsatz (von 31.380 auf 29.200 New-Order-Transaktionen pro Minute bzw. +4,2 ms Medianlatenz)28. Die Ausrichtung von ThemisDB auf lokale NVMe-Laufwerke für das WAL10 senkt somit unmittelbar die Konsenslatenz, verlangt jedoch bei verschlüsseltem Inter-Shard-Transport ausreichende CPU-Reserven1.
Für den operativen Aufwand in einer dezentralen, cloud-unabhängigen Landschaft ist zudem die Wahl des Zeitmodells entscheidend10:

Zeit- und Ordnungsmodell
Infrastruktur- und Hardware-Aufwand
Konsistenz- und Betriebsverhalten
Eignung für dezentrale Landschaften
TrueTime (Google Spanner)
Hoch: GPS-Empfänger und Atomuhren je Rechenzentrum (Unsicherheit ε typ. 1–7 ms)13
Garantiert External Consistency via Commit Wait (~7 ms im Schnitt)2
Gering für On-Premise / Edge wegen Spezialhardware20
EC2 Time Sync (Aurora DSQL)
Plattformgebunden: mikrosekundengenaue Zeitsynchronisation innerhalb von AWS33
Unterstützt Snapshot Isolation per OCC ohne lokale Locks14
Nur innerhalb der AWS-Infrastruktur nutzbar33
Hybrid Logical Clocks / HLC (CockroachDB)
Gering: Standard-NTP mit chrony, typisches --max-offset 500 ms (oft auf 250 ms reduziert)27
Keine Hardwarekosten, aber readwithinuncertainty-Neustarts bei Drift; Selbst-Terminierung ab ~400 ms Offset (bei 500 ms Limit)27
Sehr hoch bei sauberem chrony-Monitoring und deaktivierter VM-Live-Migration27

Der Block-Zielentwurf schlägt Rendezvous Hashing zur Platzierung vor. Es benötigt keine Ringstruktur; sein O(N)-Berechnungsaufwand und die Verteilung sind für die vorgesehene Shard-Zahl und Last zu messen. Hot Keys und ungleiche Last können weiterhin Hotspots erzeugen. Das Manifest enthält die gewählten Shard-IDs; in-flight Platzierungen bleiben an ihre Epoche gebunden, und eine Änderung erfordert explizite Migration.
Speicherökonomie durch hybrides Tiering aus Raft und Erasure Coding
Maximale Ausfallsicherheit führt bei naiver Replikation zu hohen Speicherkosten: Reine 3-fach-Raft-Replikation belegt 300 % des logischen Datenvolumens (200 % Overhead), reine 5-fach-Replikation sogar 500 %10. Der ThemisDB-Zielentwurf stellt dazu folgerichtig fest, dass eine zusätzliche RAID-1-Spiegelung oberhalb Raft-replizierter Shards unnötigen Mehraufwand erzeugen würde und verzichtet darauf1.
Gleichzeitig dokumentiert ThemisDB für große Datenbestände Erasure Coding mittels Reed-Solomon-Parität (etwa RS 6+3), das den Speicher-Overhead auf 50 % drückt und bis zu drei gleichzeitige Ausfälle toleriert10. Für den aktiven 2PC-Raft-Transaktionspfad ist Erasure Coding dennoch ungeeignet. Eine in den Proceedings der SOSP '24 veröffentlichte Studie zu Exascale-Storage-Clustern (Morph) sowie Analysen zu verteilten Dateisystemen zeigen, dass kleine Schreibzugriffe bei Erasure Coding kostspielige Read-Modify-Write-Zyklen erfordern16. Im Morph-Benchmark beim Erzeugen von 8-MB-Dateien stiegen sowohl die Median- als auch die p90-Schreiblatenz unter RS(6,9) gegenüber 3-fach-Replikation um nahezu das Vierfache, während der Durchsatz um 68 % sank und Degraded-Mode-Reads die Tail-Latenz stark verschlechterten16.
Die Kombination beider Verfahren in einem zweistufigen Tiered-Storage-Modell – heißes 3-fach- oder 5-fach-Raft für aktive Transaktionsdaten und nachträgliches Reed-Solomon-Tiering für unveränderliche, abgekühlte 256-KiB-Blöcke1 – verbindet niedrige Commit-Latenz mit massiv reduzierten Speicherkosten. Für einen beispielhaften Bestand von 100 TB logischen Daten bei 10 % heißem und 90 % kaltem Anteil ergibt sich folgende Speicherbilanz15:

Redundanzstrategie (100 TB logisch)
Hot-Anteil (10 TB)
Cold-Anteil (90 TB)
Physischer Speicherbedarf
Speicher-Overhead
Ersparnis vs. reine Replikation
Eignung für 2PC-Raft
Reines 3x-Raft
30 TB
270 TB
300,0 TB
200 %
Basis (0 %)
Niedrige Latenz, hohe Kosten15
Reines 5x-Raft
50 TB
450 TB
500,0 TB
400 %
Basis (0 %)
Höchste Resilienz, extremer Overhead7
Reines RS(6+3)
15 TB
135 TB
150,0 TB
50 %
50,0 % vs. 3x-Raft
Ungeeignet für heiße Writes (~4× p90-Latenz)15
Hybrid: 10 % 3x-Raft + 90 % RS(6+3)
30 TB
135 TB
165,0 TB
65 %
45,0 % vs. 3x-Raft
Optimaler Kompromiss aus Latenz und TCO10
Hybrid: 10 % 5x-Raft + 90 % RS(6+3)
50 TB
135 TB
185,0 TB
85 %
63,0 % vs. 5x-Raft
Maximale Hot-Resilienz bei moderatem TCO7

Berechnet für 100 TB logische Daten bei 10 % Hot- und 90 % Cold-Anteil15.
Systemvergleich alternativer Konsens- und Transaktionsmodelle
Im Vergleich zu alternativen Architekturen für verteilte ACID-Transaktionen zeigt sich, dass ein optimiertes 2PC-Raft-Modell für heterogene, cloud-agnostische Landschaften den flexibelsten Kompromiss darstellt:

Architekturmodell / System
Konsens & Commit-Protokoll
Zeit- & Ordnungsmodell
Cross-Shard-Schreiblatenz
Zentrale Stärken
Zentrale Einschränkungen / Aufwände
2PC über Per-Shard-Raft (ThemisDB-Zielentwurf)
Home-Shard-Raft als Koordinator; 2PC + Manifest-Publikation1
HLC/TrueTime angedacht; teils noch Sequencer referenziert6
Hoch im Basisentwurf (4 Raft-Zyklen + 2 Inter-RTTs)1
Kein globaler Koordinator; ausfallsichere Protokollrollen1
Aktuell Entwurfsstatus (review), 5 Koordinatoren, teure Reader Pins1
Parallel Commits + Raft (CockroachDB 19.2+)
2PC mit parallelen Write-Intents und STAGING-Record11
HLC über NTP/chrony (Standard --max-offset 500 ms)27
1 Konsens-RTT (halbiert ggü. klassischem 2PC)11
Hardware-agnostisch, Leader-Lease-Reads ohne Quorum11
Uncertainty-Restarts bei Clock-Skew; strikte NTP-Hygiene nötig27
Percolator + Async Commit / 1PC (TiKV / TiDB)
Multi-Raft pro Region; 1PC lokal, Async Commit für < 64 Keys12
Zentraler Placement Driver (PD) als Timestamp Oracle24
1 Prewrite-RTT bis zur Client-Antwort17
Sehr schlanker Write-Pfad bei geringer Kontention17
Zentraler PD/TSO verursacht Cross-Region-RTT und Skalierungsgrenze24
2PC + 2PL über Paxos (Google Spanner)
Paxos-Gruppen mit 2PL; Koordinator ebenfalls Paxos-repliziert2
TrueTime (GPS + Atomuhren, ε ca. 1–7 ms)13
2PC-RTT + Paxos + Commit Wait (~7 ms)2
Echte External Consistency weltweit2
Spezialisierte Zeithardware nötig; Lock-Contention durch 2PL20
Disaggregated Journal + OCC (Aurora DSQL)
Entkoppeltes Journal + Adjudicator mit OCC14
Amazon Time Sync Service (Mikrosekunden-Präzision)33
~2 RTTs zum nächsten Endpoint + synchrone Replikation33
Serverlos, minimaler Betriebsaufwand, lokale Reads14
Cloud-Bindung; in Preview harte Limits (10.000 Rows, 10 MiB, kein FK)14
Deterministischer Konsens (Calvin)
Batch-Sequencing + Raft/Paxos-Replikation vor Ausführung45
Globale Epochen-Sequenznummer45
1 Konsens-Phase + Batching-Delay (zehn+ ms)48
Kein 2PC, keine verteilten Deadlocks48
Erfordert vorab bekannte Read-/Write-Sets; keine interaktiven Tx11

Die Vergleichssysteme zeigen unterschiedliche Abwägungen. Aus ihnen folgt jedoch kein Nachweis voller ACID-Fähigkeit für ThemisDB: Der hier geplante Objektversionspfad muss seinen eigenen Sichtbarkeits-, Recovery- und Dauerhaftigkeitsvertrag nachweisen; allgemeine DB-Transaktionen liegen außerhalb dieses Nachweises.
## Handlungsempfehlungen der ursprünglichen Analyse

Die folgenden Empfehlungen sind Diskussionsgrundlage, keine bereits beschlossene Umsetzung. Insbesondere sind globale WAL-/Koordinator-Ablösung, frühes ACK, Ersatz von Reader Pins, HLC-Umstellung, RF=5 und Cold-Tiering separate Entscheidungen; der nachfolgende Plan definiert die sicheren Voraussetzungen.
Um das bewertete 2PC-Raft-Modell aus dem gegenwärtigen Entwurfs- und Fragmentierungsstatus1 in eine belastbare, betriebseffiziente Produktionsarchitektur zu überführen, sollten sieben architektonische Maßnahmen priorisiert werden:
Konsolidierung: DIS-05 als Ausgangspunkt verwenden und die weiteren Koordinatoren und WAL-Domänen zunächst inventarisieren. Ein global einheitliches WAL oder Interface wird dadurch nicht vorausgesetzt; Migration und Entfernung benötigen einen eigenen Vertrag. Der neue Home-Raft-Pfad muss Phase 2 nach Leaderwechsel idempotent erneut zustellen.
Parallel Commits: Erst nach sicherer synchroner Baseline als separates Protokoll mit STAGING, Visibility- und Recovery-Nachweis bewerten; eine Latenzhalbierung ist nicht zugesichert.
Reader Pins und Leseoptimierung: Durable Pins bleiben der Baseline-Vertrag. Leases und Closed Timestamps können nur nach separatem Nachweis für Konsistenz und Versionslebensdauer ergänzen oder ersetzen.
Zeitmodell: Den tatsächlichen Sequencer-/HLC-Bedarf untersuchen. HLC und Uhrüberwachung allein ersetzen weder ein Ordnungsprotokoll noch den Nachweis externer Konsistenz.
Fan-out und Replikationsfaktor: Objektlokalität und eine explizite Teilnehmergrenze prüfen. RF=5 für Home-/Katalog-Shards ist eine mögliche Betriebsoption, die fünf geeignete Fehlerdomänen, Kostenbewertung und eigene Evidenz voraussetzt; die erste Zielkonfiguration verwendet RF=3.
Storage-Tiering: Lokales NVMe und nachgelagertes Reed-Solomon 6+3 sind mögliche Betriebsoptionen, keine zwingend nachgewiesenen Voraussetzungen. Transkodierung, Fehlerdomänen, Wiederherstellung und Platzgewinn sind gesondert zu messen und sicher abzunehmen.
Verbindliche Chaos- und Recovery-Abnahmetests vor Produktivfreigabe: Vor jedem produktiven Rollout sind die im Zielentwurf definierten realen Netzwerk- und Storage-Tests vollständig nachzuweisen – insbesondere Quorum-Verlust während PREPARE und Phase 2, Leader-Wechsel unter Last, WAL-Compaction-Grenzen, Block-Korruption sowie die Bereinigung der dokumentierten Risiken bei WAL-Wachstum und Gossip-Topologie-Updates1.
## Konkreter Umsetzungsplan: Objektversion über 2PC und Raft

**Status:** Vorschlag zur technischen und menschlichen Prüfung, nicht freigegebene Roadmap und kein Nachweis einer bestehenden End-to-End-Funktion. Ziel ist eine einzelne immutable Objektversion mit mehreren Blöcken; allgemeine datenbankweite Multi-Objekt- oder SQL-Transaktionen sind nicht zugesichert. Der bestehende [Zielentwurf](docs/architecture/cross_shard_block_replication_target.md) definiert den synchronen Sichtbarkeitsvertrag; [DIS-05](audit/ADR_DIS-05_CANONICAL_2PC_COORDINATOR.md), [Sharding-API](src/sharding/API.md), [Sharding-Roadmap](src/sharding/ROADMAP.md) und [Replication-Roadmap](src/replication/ROADMAP.md) begrenzen den nachgewiesenen Ist-Stand. Bei Konflikten gelten Root-SOT und Modulquellen vor dieser Analyse. Der Zielentwurf selbst steht unter Review.

### Architektur und Sicherheitsvertrag

1. **Isolation und Rollout:** Neuer explizit aktivierter Pfad mit versionierten Datensätzen und API; bestehende Protokolle und Nicht-2PC-WALs bleiben unverändert. Ohne konfigurierte echte Persistenz, Transport, Authentisierung, passende Replica-Konfiguration und verfügbares Quorum: Fehler, niemals lokale Ein-Knoten-Erfolgsmeldung.
2. **Identität und Platzierung:** Ein Locator bindet Tenant, Objekt, stabilen Home-Shard und Erstellungsepoche. Bei Beginn werden Transaktions-ID, neue Version, erwartete Vorversion, Manifest, Teilnehmer und Platzierungsepoche dauerhaft gebunden. Identische Wiederholung liefert denselben Status; ID-Wiederverwendung mit anderen Eingaben wird abgelehnt. Leere Objekte sind gültig; Home-Reservierung und Publikation sind dennoch erforderlich.
3. **Dauerhaftigkeit:** Jeder betroffene Shard persistiert validierte Blockbytes und PREPARED-Zustand über sein eigenes Raft-Mehrheitsquorum, bevor er YES meldet. Bei RF=3 sind das mindestens zwei dauerhaft bestätigende Replikate in getrennten Fehlerdomänen. Home ist ebenfalls Teilnehmer; alle benötigten Gruppen, nicht nur deren Mehrheit, müssen YES liefern.
4. **Entscheidung:** Erst nach allen YES-Stimmen darf das Home-Quorum COMMIT festschreiben. ABORT muss ebenfalls vor Zustellung im Home-Quorum beschlossen werden. Eine vorgeschlagene, aber nicht quorum-committete Entscheidung genügt nicht; Term-/Leader-Fencing und Zustandsmaschinenvalidierung verhindern widersprüchliche Entscheidungen. Timeouts beim Client oder Teilnehmer bedeuten UNKNOWN bzw. IN_DOUBT, nicht eigenmächtigen ABORT. Eine autoritative Home-Recovery darf gemäß Protokoll nur dann ABORT beschließen, wenn kein COMMIT beschlossen ist und die notwendige Führung/Quorum-Sicherheit besteht.
5. **Sichtbarkeit und ACK:** Teilnehmer wenden die Entscheidung idempotent und dauerhaft über ihre jeweiligen Raft-Gruppen an. Erst nach allen dauerhaften COMMIT-ACKs veröffentlicht Home den Manifestzeiger in einem weiteren quorum-committeten Schritt. Erst nach dieser Publikation meldet der Client Erfolg. Ein verlorener Reply wird über dauerhafte Statusabfrage oder denselben Schlüssel aufgelöst; ein beschlossenes COMMIT wird nie in ABORT umgewandelt.
6. **Lesen und Aufbewahrung:** Ein linearizable Home-Vorgang liefert veröffentlichten Zeiger und dauerhaften Reader Pin atomar. Clients lesen nur Blöcke dieser Version, prüfen Länge und SHA-256 und geben den Pin erst nach Abschluss oder Abbruch aller Reads frei. GC schützt aktive Pins, offene Transaktionen und referenzierte Versionen; Pin-Widerruf erfordert Fencing und Drain, nicht allein einen Timeout.
7. **Migration:** Platzierungsepochen und Teilnehmer laufender Transaktionen bleiben unverändert. Home-Migration erfordert Fencing und dauerhafte Weiterleitung des alten Locators. Alte Transaktionen werden mit ihrer ursprünglichen Autorität abgeschlossen oder ausdrücklich durch Maintainer geklärt; WALs werden nicht still umgedeutet. Ein Rollback nach Aktivierung erfordert getestete Zustandsmigration und Drain; andernfalls fail closed.

### Lieferpakete und Abhängigkeiten

Alle Pakete sind offen. Die Reihenfolge ist eine technische Abhängigkeit, kein Termin- oder Aufwandsversprechen.

| Paket | Voraussetzung | Konkrete Arbeit und betroffene Stellen | Abnahme / Ergebnis |
|---|---|---|---|
| **0 – Bestand und ADR** | Keine | Runtime- und Build-Pfad von `include/sharding/two_phase_commit_coordinator.h`, `include/sharding/raft_consensus.h`, `include/sharding/raft_wal_integration.h`, `include/replication/raft_v2.h` und ihren Implementierungen bis Storage, Netzwerk und Aufrufer verfolgen. Per ADR genau eine Raft-Implementierung für **diesen neuen Pfad** anhand echter Quorum-, fsync-, Fencing-, Snapshot- und Membership-Eigenschaften auswählen; vorhandene Brücken nur bei nachgewiesener Eignung wiederverwenden. DIS-05/DIS-06-Evidenz schließen oder Lücken ausdrücklich als Blocker ausweisen. | Aufruf-/Konfigurationskarte, Format- und Fehlervertrag, Reproduktionsbefehle und verantwortliche Maintainer dokumentiert; kein globales Entfernen anderer Koordinatoren oder Protokolle ohne eigene Migrationsentscheidung. |
| **1 – Raft-Grundlage** | 0 | Gewählte Shard-Gruppe an echte persistente Logs, replizierte State Machine und authentifizierten Netzwerktransport anschließen. Durable Replica-ACK, Leader-/Term-Fencing, Quorum-Commit, Wiederanlauf, sichere Membership-Änderung und Snapshot/Compaction festlegen und implementieren; keine Annahme, dass `RaftWALIntegration` dies bereits end-to-end leistet. | Drei getrennte Prozesse replizieren tatsächlich über Netzwerk; nach Quorum-ACK und Neustart sind Inhalt und Commit-Index vorhanden. Minderheit, fsync-Fehler, alter Leader und unterbrochene Membership-Änderung können keinen Erfolg vortäuschen. |
| **2 – Platzierung und Block-Staging** | 1 | `include/storage/object_block_manifest.h` und `src/storage/object_block_manifest.cpp` als lokale Integritätsbasis verwenden, nicht als Replikationsnachweis. Versionierten Locator/Platzierungsplan, Rendezvous-Zuordnung, begrenzten Blocktransfer und per Shard Raft-replizierte Bytes plus PREPARED implementieren; Object-Version und Ordinal bilden den Speicher-Scope. Authentisierung und Autorisierung pro Tenant, Objekt und Block. | Leeres Objekt und 1-, 256-KiB- sowie Mehrblockfälle; falsche Länge/Hash, Duplikate, fehlende Blöcke und Quorumverlust scheitern sicher. Eine YES-Stimme belegt Bytes und Zustand nach Neustart auf mindestens zwei Replikaten. |
| **3 – Home-Entscheidung und Recovery** | 1, 2 | `TwoPhaseCommitCoordinator`-Vertrag um einen ausdrücklich angebundenen, quorum-committeten Home-Entscheidungsspeicher erweitern bzw. per geprüftem Adapter anbinden; BEGIN, Teilnehmer, erwartete Version, Reservierung, Entscheidung und Lieferfortschritt reproduzierbar in die Home-State-Machine integrieren. Nach Leaderwechsel anhand des beschlossenen Zustands Phase 2 idempotent wiederholen. Keine parallele zweite Entscheidungsautorität oder implizite WAL-Migration. | Crash vor/nach Quorum-Entscheidung und während Phase 2; ein einziges unveränderliches Ergebnis, keine Phase-2-Zustellung vor Entscheidung, keine Commit-zu-Abort-Umwandlung; alter Leader und inkonsistente Wiederholung abgewiesen. |
| **4 – Publikation, Reads und GC** | 3 | Home-Reservierung und Expected-Version-Prüfung mit atomarer Manifestpublikation verdrahten; durable Outcome-API, Reader Pins, Versionserhalt und idempotente GC implementieren. Pin-Erstellung, Publikation und Löschfreigabe in der Home-State-Machine serialisieren. | Zwei konkurrierende Updates derselben Vorversion: höchstens eines publiziert. Leser sehen bei Crash/GC ausschließlich alte vollständige oder neue vollständige Version; keine Freigabe vor allen Teilnehmer-ACKs, kein ACK vor Publikation, kein Löschen gepinnter oder in-doubt Daten. |
| **5 – Echter End-to-End-Pfad und Freigabe** | 4 | Client-/Server-API, Routing, Recovery, Konfiguration, Metriken und Runbook integrieren. Reale Mehrprozess-/Mehrknoten-Tests mit persistentem Storage und TLS/mTLS, getrennten Fehlerdomänen und dokumentiertem Fault-Injection-Harness; keine Mock- oder Callback-Tests als Ersatz für den Transportnachweis. | Szenarien der Abnahmematrix unten bestehen; Topologie, Befehle, Logs und Ergebnisse sind reproduzierbar. Erst nach menschlicher Architektur-, Sicherheits- und Betriebsfreigabe begrenzter Opt-in-Rollout. |
| **6 – Migration und Optimierungen** | 5 und jeweils eigenes Design | Live-Migration, Batch-/Paralleltransfer, Cold-Erasure-Coding und gegebenenfalls spätere asynchrone ACKs, STAGING/Parallel Commits, Leases oder Closed Timestamps separat entwerfen. Cross-Koordinator-, HLC- oder WAL-Konsolidierung nur nach Inventar, Kompatibilitäts-ADR und sicherer Migration. | Jede Optimierung erhält eigene Korrektheitsinvarianten, Recovery-/Rollback- und Benchmark-Nachweise; bis dahin bleibt der Vertrag aus Paket 4 unverändert. |

### Paket 0 – erste quellengestützte Bestandsaufnahme (2026-11-20)

**Status: in Arbeit; Auswahl eines produktiven Raft-Pfads und dessen Freigabe offen.** Keine der folgenden Komponenten ist allein ein Nachweis für das Ziel aus Paket 1:

| Baustein | Nachweis aus aktuellem Quelltext | Konsequenz für die Umsetzung |
|---|---|---|
| `RaftConsensus` | [`propose`](src/sharding/raft_consensus.cpp) nutzt `AppendEntriesRequest`/`AppendEntriesResponse`; ein ACK zählt nur bei passendem Term und `match_index` mindestens bis zum vorgeschlagenen Eintrag. Der Follower prüft Präfix, Kontiguität, Konflikte und Commit-Grenze; volatile Follower ohne gesundes Journal werden abgelehnt. Mehrknoten-Leader benötigen ebenfalls ein gesundes lokales Journal. Der Wahlpfad kann `VoteRequest`s über einen Callback senden, exakte Peer-ID/Term-Antworten zählen und bei höherem Term persistiert zurückstufen; mehrknotige Kandidaten und Voter benötigen gesunde Journale, und der Vote-Handler lehnt Nichtmitglieder sowie veraltete Logs ab. Kandidaten starten nach Timeout eine neue Wahl. | Die AppendEntries-Callback-API ist source-incompatible; C++-Verbraucher müssen neu bauen. Die Vote- und AppendEntries-RPCs sind nun über gRPC/mTLS an explizite SAN-zu-Mitglied-Bindungen und begrenzte Deadlines angeschlossen; die neue Integration ist noch nicht ausgeführt. Konfliktreparatur, sichere Membership und Mehrprozess-/Dreiknoten-ACK-Evidenz bleiben offen. Der Gateway-Simulationscallback ist entfernt; Mehrknoten-Writes bleiben ohne Resolver fail-closed. |
| `RaftShardManager` | [`initializeShard` und `proposeWrite`](src/sharding/raft_shard_manager.cpp) erzeugen bzw. nutzen `RaftConsensus`; ein optionaler Resolver liefert pro entfernter Node-ID eine mTLS-Client-Konfiguration und verdrahtet Wahl-, Replikations- und Heartbeat-Callbacks. | Ohne explizite Peer-Konfiguration bleiben mehrknotige Operationen fail-closed. Laufzeit-, Crash-/Restart- und unabhängige Fehlerdomänen-Evidenz bleibt Paket-1-Blocker. |
| `DistributedGateway` | Der frühere In-Process-Callback wurde entfernt; `proposeConfig` wendet eine Konfiguration erst nach lokalem Raft-Commit an. | Einzelknoten-Konfigurationen können lokal committen; Multi-Node-Proposals scheitern ohne echten Peer-Transport und gesundes lokales Journal fail-closed. |
| `RaftConsensusAdapter` | [`propose`](src/sharding/raft_consensus_adapter.cpp) verwendet den vom Raft-Vorschlag tatsächlich zugewiesenen Index und gibt bei gemeldetem Fehler keinen Index zurück. | Dies behebt weder die offenen Quorum-/Fencing-Grenzen noch belegt es einen über das Netzwerk dauerhaft replizierten Entscheidungsdatensatz. |
| `RaftWALIntegration` | [`write` und `compact`](src/sharding/raft_wal_integration.cpp) verwenden separate WAL-/Raft-Logs; `compact` setzt Commit-Metadaten, erstellt aber keinen Snapshot. Die ACK-Verarbeitung ist eine private Methode; ein produktiver Aufrufer wurde bei der Symbolsuche nicht gefunden. | Kein Adapter für den Zielpfad ohne nachgewiesene Verdrahtung von Replica-ACKs, dauerhafter Speicherung, Fencing und sicherer Snapshot-/Compaction-Recovery. |
| `ReplicationManager` / `raft_v2` | [`ReplicationStream::sendBatch`](src/replication/replication_manager.cpp) meldet ohne Netzwerktransport ausdrücklich Fehlschlag; [`raft_v2`](src/replication/raft_v2.cpp) enthält Membership- und WAL-Logik. | Aus Membership- oder lokalen WAL-Tests darf keine tatsächlich über das Netzwerk replizierte Raft-State-Machine abgeleitet werden. |
| Manifest und 2PC | [`object_block_manifest.h`](include/storage/object_block_manifest.h) bietet lokale Block-/Hash-Metadaten. Die [Sharding-API](src/sharding/API.md) beschreibt die lokalen DIS-05/DIS-06-WAL-Grenzen, nicht einen quorum-replizierten Home-Entscheidungsspeicher. | Beide als vorhandene Grundlagen nutzen; für Replikation, Platzierung, Veröffentlichung und Autorisierung sind eigene Integrationsschritte notwendig. |

**Lokale Bestandsvalidierung:** `ctest --test-dir build-msvc-windows-bench -C Release -R '^(TwoPhaseCommitFocusedTests|RaftPersistenceFocusedTests|CrossShardTransactionFocusedTests|test_storage_object_block_manifest_StorageFocusedTests)$' --output-on-failure --timeout 120` meldete zunächst drei PASS und für den Manifesttest TIMEOUT. Die spätere Prüfung ergab jedoch, dass die drei damaligen Windows-Binaries jeweils **null Tests** registrierten; diese PASS-Ergebnisse sind ungültig. Beim Manifesttest fehlte das Executable; der On-Demand-Build überschritt das Zeitlimit.

Der fokussierte Raft-Persistenztest wurde anschließend ohne das für diese Suite unnötige GMock gelinkt und neu gebaut. Der direkte Aufruf `build-msvc-windows-bench\bin\tests_by_module\sharding\Release\test_raft_persistence_focused.exe --gtest_brief=1 --gtest_fail_if_no_test_selected` führte 14 Tests erfolgreich aus, darunter Indexzuordnung, fehlendes Quorum und überlappende Vorschläge. Dies ist ein lokaler Komponenten-, kein Mehrknoten- oder Adapter-End-to-End-Nachweis. Die übrigen Testregistrierungen und Paket 0 bleiben offen.

Die aktuelle Ergänzung prüft unter demselben Raft-Log-Lock, ob der vorhandene Tail vollständig committed ist, und lehnt andernfalls ein neues Proposal ab. Die Callback-API transportiert jetzt Request-Präfix und Response-Term/Match-Index; Follower-ACKs und Mehrknoten-Leader ohne gesundes dauerhaftes Journal werden fail-closed abgelehnt. Der Follower prüft zusammenhängende Einträge und beschränkt Commit sowie ACK auf den verifizierten Präfix. `Build_CMakeTools` für `test_raft_persistence_focused` und `themis_tests` war erfolgreich. Der fokussierte Lauf führte 23/23 Raft-Persistenztests aus; elf Fencing-/Präfix-/Durability-Regressionen bestanden zusätzlich 20 Wiederholungen (220/220). Zwei Gateway-Regressionen bestanden (Single-Node-Commit und Non-Leader-Ablehnung). Ein breiterer Filter führte 12/13 Raft-/Gateway-Tests aus; `RaftConsensusTest.HeartbeatCallback` schlug fehl, weil der Test vor `becomeLeader()` weder Kandidatenstatus noch Quorum herstellt. Das konfigurierte CTest-Verzeichnis listet nur Benchmark-Tests, deshalb wurde kein CTest-Ergebnis behauptet. Die echte Mehrknoten-Abnahme steht aus; lokale Callback-Tests sind kein End-to-End-Nachweis.

**Nachtrag 2026-11-20:** Die direkten `becomeLeader()`-Aufrufe in den Mehrknoten-Test-Fixtures wurden durch Kandidatenwahl plus Peer-Stimme ersetzt. Zusätzlich ruft `RaftConsensus` nun einen optionalen Vote-Request-Callback auf; ungültige Peer-IDs/Terms werden nicht gezählt, höhere Terms führen zum Step-down, und Kandidaten können nach Timeout erneut wählen. Mehrknoten-Kandidaten und -Voter benötigen ein gesundes lokales Journal; `RaftState::handleVoteRequest` prüft zusätzlich Mitgliedschaft und Log-Frische. Die vier geänderten C++-Translation-Units (Raft-Konsens, Raft-State und deren aktive Tests) wurden direkt erfolgreich kompiliert. Ein vollständiger Link/Testlauf wurde nicht erreicht: der CMake/Ninja-Build scheiterte an `Permission denied` beim Zugriff auf gemeinsam genutzte Build-Artefakte. Es gibt daher keinen neuen Laufzeit-PASS; gRPC/mTLS-Transport und Mehrprozess-E2E bleiben offen. Die ältere 12/13-Testbeobachtung oben ist historische Evidenz vor dem Fixture-Fix.

**Nachtrag 2026-11-21:** Vor dem gRPC-Raft-Transport wurde die Identitätsgrenze explizit festgelegt: `RaftShardManager` verwendet `shard_id` als logische Gruppen-ID und das verschachtelte `RaftConfig::node_id` in `Config::raft_config` als lokale Mitglieds-ID. Bestehende Manager-Konfigurationen müssen diese lokale ID setzen; es gibt keinen Fallback von Gruppen-ID auf Mitgliedsidentität. Die Initialisierung lehnt leere/duplizierte Mitglied-IDs und eine nicht enthaltene lokale ID ab. Produktions-TU und aktive/Legacy-Test-TUs wurden direkt erfolgreich kompiliert; Laufzeit-Tests wurden nicht ausgeführt, weil das aktuelle CMake-Testinventar auf Benchmark-Targets konfiguriert ist.

**Transport-Nachtrag 2026-11-21:** `shard_rpc.proto` enthält jetzt Vote- und AppendEntries-RPCs. Server-Handler verlangen eine konfigurierte Gruppenauflösung und eine eindeutige Bindung des authentisierten Client-SAN an ein Mitglied dieser Gruppe; sie rufen anschließend `RaftState` auf. Der Client verlangt verifiziertes mTLS, prüft den TLS-DNS-SAN des Servers sowie die Antwort-Mitglied-ID und setzt eine Deadline von höchstens fünf Sekunden. `RaftShardManager` kann pro entferntem Mitglied explizite mTLS-Client-Konfigurationen auflösen und damit Wahl-, Replikations- und Heartbeat-Callbacks installieren. Der Linux-gRPC-Test enthält Zwei- und Dreiprozess-Szenarien mit Manager-Wahl über getrennte Follower, dauerhaftem Append, SIGKILL/Restart aus demselben Journal, Prüfung des wiederhergestellten Log-Präfixes und der gespeicherten Stimme sowie Quorumverlust bei Ausfall beider Follower. Zusätzlich startet ein eigener Leader-Prozess als Node 1, repliziert ein durable Entry, wird SIGKILL-gestoppt, und Node 2/3 haben eigene Manager-/mTLS-Prozesse für eine Neuwahl. Nach dem Crash des Leaders werden diese Follower auf einen Leader-ready-Marker geprüft; der Leader-Prozess kann anschließend aus demselben Journal neu gestartet werden. Die Negativfälle umfassen volatile Journalablehnung, stale Term, fehlenden Log-Präfix und einen begrenzten RPC-Timeout. Diese Tests wurden in der aktuellen Windows-Benchmark-Konfiguration nicht kompiliert oder ausgeführt; Linux-Laufzeitevidenz bleibt offen. Konfliktreparatur, sichere Membership-Änderungen und der quorum-committete 2PC-Entscheidungsspeicher sind nicht geliefert. CockroachDB-Parallel-Commits bleiben bis zum Bestehen des vollständigen Gates ausdrücklich außerhalb des Umfangs.

### Abnahmematrix und Ausführung

- **Durability/Quorum:** RF=3 mit drei unabhängigen Fehlerdomänen; nachgewiesen zwei dauerhaft gespeicherte Kopien pro benötigtem Block vor YES. Kill/Restart nach fsync, Quorumverlust, Plattenfehler und Compaction/Snapshot prüfen; Daten und Transaktionszustand müssen wiederherstellbar sein. Ein Prozess-Kill allein beweist kein Verhalten bei Stromverlust.
- **Entscheidung:** Fehler vor und nach Home-BEGIN, Teilnehmer-PREPARE, COMMIT/ABORT-Quorum, jedem Phase-2-ACK und Manifestpublikation; Leaderwechsel und Netzpartition. Prüfen: eine Entscheidung, keine widersprüchlichen Entscheidungen, Wiederholung idempotent, keine unberechtigte Freigabe.
- **Client und Sichtbarkeit:** Timeout vor/nach Entscheidung und nach Publikation; erneute Abfrage und Retry mit derselben ID. Parallel konkurrierende Writer und Reader, alte Reader Pins, GC und verwaiste Pins prüfen. Beobachtungen müssen die vollständige Objektversion und den erwarteten Outcome belegen, nicht nur interne Statusflags.
- **Daten/Transport/Sicherheit:** Fehlende, manipulierte, vertauschte und doppelte Blöcke, abgebrochene Transfers, begrenzte Warteschlangen, ungültige Leader-Redirects sowie tenantübergreifende Zugriffe. Authentisierte TLS-Verbindungen und Autorisierung pro Objekt/Block prüfen.
- **Betrieb/Migration:** Neustarts, neue Leader, sichere Membership- und Platzierungsänderungen mit laufender Transaktion, Upgrade und unterstützter Rollback; alte WALs separat erhalten. Outcome-Retention bzw. Tombstones müssen ID-Wiederverwendung über den unterstützten Retry-Zeitraum verhindern.
- **Leistung:** Erst auf dokumentierter Hardware/Topologie den synchronen Basispfad messen: Durchsatz, p50/p95/p99, fsync, Ressourcen, Fan-out und Recovery-Zeit. Numerische Release-Grenzen werden nach Baseline und Betreiberfreigabe festgelegt; die analytischen Zahlen oben sind keine Produkt-Benchmarks.

Für den Einstieg dienen die vorhandenen Tests `tests/test_two_phase_commit.cpp`, `tests/test_cross_shard_transaction_focused.cpp`, `tests/test_raft_persistence_focused.cpp`, `tests/transaction/test_transaction_process_crash_recovery.cpp` und `tests/storage/test_storage_object_block_manifest.cpp`; sie ersetzen nicht die neue echte Mehrknoten-Abnahme. Die registrierten Sharding-Tests heißen unter anderem `TwoPhaseCommitFocusedTests` und `RaftPersistenceFocusedTests`. Vor jedem Paket wird die tatsächlich konfigurierte CTest-Liste geprüft; Build- und Testbefehle samt Ergebnis werden in der jeweiligen Umsetzung dokumentiert.

### Offene Entscheidungen vor Aktivierung

- Welche Raft-Implementierung und welcher reale Netzwerk-/Speicherpfad erfüllen Paket 1 nachweislich? Welche Limits für Blockgröße, Objektgröße, Teilnehmerzahl, Warteschlangen und Retry-Retention werden für die erste unterstützte Konfiguration freigegeben?
- Wer verantwortet Architektur, Security, Betrieb und Freigabe; auf welcher konkreten Topologie werden unabhängige Fehlerdomänen und Storage-Durability nachgewiesen?
- Wird das Ziel zunächst ausschließlich für Objektversionen angeboten? Allgemeine DB-Transaktionen erfordern einen gesonderten Isolation-/Sichtbarkeitsvertrag und eigene Abnahme.
- Frühere ACKs sind eine **Vertragsänderung**: Selbst nach COMMIT dürfen Leser weder einen Teilzustand noch ein fehlendes Objekt fälschlich als endgültig interpretieren. Intent-Auflösung allein ersetzt die Manifestpublikation nicht; vor einer Änderung sind Client-Erfolgssemantik, atomare Sichtbarkeit, Recovery und GC formal zu spezifizieren und chaotisch zu testen.
- Leader Leases/Closed Timestamps ersetzen **keinen** Reader Pin: Sie erlauben nicht automatisch die sichere Aufbewahrung einer alten Version während langer Block-Reads. Ein Ersatz braucht ein nachgewiesenes Lifetime-/Reclamation-Protokoll mit Fencing und Clock-Annahmen. HLC allein beweist weder linearisierbare Reads noch externe Konsistenz.
- RF=5, Erasure Coding und die vollständige Ablösung anderer Koordinatoren, WALs oder Uhren sind keine Voraussetzung für das erste sichere End-to-End-Inkrement und erfordern eigene Kosten-, Migrations- und Sicherheitsentscheidungen.

## Referenzen
cross_shard_block_replication_target.md
How Spanner Does Distributed Transactions Without Classic 2PC, https://portfolio.shenthar.me/blog/systems/spanner-distributed-transactions
Consensus on Transaction Commit - Leslie Lamport, https://lamport.azurewebsites.net/video/consensus-on-transaction-commit.pdf
Distributed Transactions: Commit Protocols - Rafee Mohamed, https://rafeemohamed.com/blogs/distributed-transactions-commit-protocols/
Rendezvous and Maglev Hashing - Blog Notes, https://blog.vitalvas.com/post/2026/02/01/rendezvous-and-maglev-hashing/
RAID_SHARD_REFERENCING_ARCHITECTURE.md
RAFT_CONSENSUS_PER_SHARD.md
transaction_coordinators.md


ThemisDB, uploaded:ThemisDB
Parallel Commits: An atomic commit protocol for globally distributed, https://www.cockroachlabs.com/blog/parallel-commits/
Architecture - TiKV, https://tikv.org/docs/3.0/concepts/architecture/
Exploring the Power of Google Spanner for Distributed Transaction, https://princemodi.me/post/google-spanner/
Aurora DSQL Through the Lens of the PACELC Theorem - Medium, https://mitek99.medium.com/aurora-dsql-through-the-lens-of-the-pacelc-theorem-489cee1de822
unknown_url
Morph: Efficient File-Lifetime Redundancy Management for Cluster, https://www.cs.cmu.edu/~rvinayak/papers/Morph-sosp-2024.pdf
sig-transaction/design/async-commit/README.md at master - GitHub, https://github.com/tikv/sig-transaction/blob/master/design/async-commit/README.md
Erasure Coding vs. Replication in Distributed Filesystem Durability, https://persistentagent.co/posts/erasure-coding-vs-replication-in-distributed-filesystem-durability
Atomic Commit for a Cloud DBMS with Storage Disaggregation, https://www.vldb.org/pvldb/vol16/p379-guo.pdf
Spanner, 2PC+2PL over Paxos - MyDistributed.Systems, https://www.mydistributed.systems/2020/08/google-spanner.html
Rendezvous hashing - Wikipedia, https://en.wikipedia.org/wiki/Rendezvous_hashing
_Concurrent n Distributed systems reading lists · GitHub, https://gist.github.com/romange/64b4fb1e6c5229f3de302cff2eca25d6
Untitled - World Academic Press, https://www.worldacademic.press/uploads/books/1793253403741-138283649-WAPMS2604104_book__2_.pdf
How We Reduced Multi-region Read Latency and Network Traffic by, https://www.pingcap.com/blog/how-we-reduced-multi-region-read-latency-and-network-traffic-by-50/
How We Reduced Multi-Region Read Latency and Network Traffic, https://dzone.com/articles/how-we-reduced-multi-region-read-latency-and-netwo
Best Practices for Deploying TiDB on Public Cloud, https://docs.pingcap.com/best-practices/best-practices-on-public-cloud/
CockroachDB clock synchronization error: this node is more than, https://www.netdata.cloud/guides/cockroachdb/cockroachdb-clock-synchronization-error-500ms/
Running TiDB on Kubernetes with OpenEverest, https://openeverest.io/blog/self-hosting-tidb-on-kubernetes/
Transaction Layer - CockroachDB - Cockroach Labs, https://docs.cockroachlabs.com/docs/stable/architecture/transaction-layer
cockroach/docs/design.md at master · cockroachdb ... - GitHub, https://github.com/cockroachdb/cockroach/blob/master/docs/design.md
Spanner - First Principles Engineering, https://firstprinciplesengineering.tech/01-fundamentals/03-technologies/07-spanner
Summary of Spanner: Google's Globally-Distributed Database, https://salemal.medium.com/summary-of-spanner-googles-globally-distributed-database-f66a6ea25a81
Amazon Aurora DSQL: Performance, Limits & Architecture, https://andrewbaker.ninja/2025/11/19/amazon-aurora-dsql-performance-limits-architecture/
LeaseGuard: Raft Leases Done Right - arXiv, https://arxiv.org/html/2512.15659v1
Build Modern Applications with Amazon Aurora DSQL - Slideshare, https://www.slideshare.net/slideshow/build-modern-applications-with-amazon-aurora-dsql-aws-community-day-athens-2026/287317522
리듬의 마술사가 나타났다? - ACRYL, https://acryl.ai/resources/blog/aurora-dsql-aws
CockroachDB Architecture: A Deep Systems-Level Deep Dive, https://medium.com/@aman.kohli1/cockroachdb-architecture-a-deep-systems-level-deep-dive-fb0498813e1b
CockroachDB Architecture Overview | PDF | Replication (Computing), https://www.scribd.com/document/805339432/Cockroach-Labs-Ben-Darnell-CockroachDB-Architecture-Presentation-RoachFest-2022
CockroachDB ReadWithinUncertaintyInterval restarts: the near, https://www.netdata.cloud/guides/cockroachdb/cockroachdb-readwithinuncertainty-restarts/
Consistent Hashing vs. Rendezvous Hashing: A Comparative Analysis, https://dzone.com/articles/consistent-hashing-vs-rendezvous-hashing-a-compara
Understanding Rendezvous Hashing: A Scalable and Balanced, https://medium.com/@communicate.saksham/understanding-rendezvous-hashing-a-scalable-and-balanced-approach-b5dbff84d0d9
Principles of Operation - bcachefs, https://bcachefs.org/bcachefs-principles-of-operation.pdf
Algorithm & Systems Visualizations, https://giaicuuthegioi.com/
(PDF) CockroachDB: The Resilient Geo-Distributed SQL Database, https://www.researchgate.net/publication/341744190_CockroachDB_The_Resilient_Geo-Distributed_SQL_Database
Optimizing Geo-replicated Deterministic Databases - arXiv, https://arxiv.org/html/2611.05882
Use of Time in Distributed Databases (part 4): Synchronized clocks, http://muratbuffalo.blogspot.com/2025/01/use-of-time-in-distributed-databases.html
a High-performance and General Distributed Transaction Protocol, https://arxiv.org/html/2302.12517v2
Optimizing Distributed Transactions via Modern AI, Storage and, https://academiccommons.columbia.edu/doi/10.7916/q0vz-1e26/download
