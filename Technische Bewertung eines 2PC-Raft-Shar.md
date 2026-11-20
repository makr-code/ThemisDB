Technische Bewertung eines 2PC-Raft-Shard-Konsensmodells für dezentrale Datenbanklandschaften
Ein dezentrales Konsensmodell, das Two-Phase Commit (2PC) über per Shard organisierte Raft-Konsensgruppen legt, erfüllt grundsätzlich das Ziel maximaler Ausfallsicherheit bei begrenztem Koordinationsaufwand: Indem sowohl der 2PC-Koordinator als auch jeder Teilnehmer nicht aus Einzelknoten, sondern aus fehlertoleranten Raft-Gruppen bestehen, entfällt das klassische Blockierungsrisiko bei Koordinatorausfällen, ohne die Konsistenzrisiken eines Three-Phase-Commit-Protokolls (3PC) bei Netzwerkpartitionen einzugehen1. Gleichzeitig vermeidet eine verteilte Koordinatorzuordnung – bei der etwa die Raft-Gruppe des objektbezogenen Home-Shards per Rendezvous Hashing die Transaktionssteuerung übernimmt – den Aufbau und Betrieb eines globalen zentralen Koordinationsclusters1.
Für die bewerteten ThemisDB-Architekturspezifikationen (Stand November 2026) gilt diese Eignung jedoch nur unter einem deutlichen Reife- und Umsetzungsvorbehalt: Alle vier Kern-Spezifikationen tragen den Status review, sind KI-gestützt entstanden und weisen kein abgeschlossenes Human Review auf (Human_Review: false)1. Das im Zielentwurf skizzierte Raft-gestützte 2PC-Protokoll ist bislang nicht als durchgängig verdrahtete Produktionsimplementierung nachgewiesen1. Statt eines einheitlichen Konsenskernes existieren im Quellcode derzeit fünf unterschiedliche Koordinatorklassen, wobei zumindest die drei näher dokumentierten Implementierungen inkompatible Write-Ahead-Log-Formate (WAL) schreiben und CrossShardTransactionCoordinator::recoverFromWAL() nach Abstürzen keine Phase-2-RPCs automatisch neu antreibt8. Da zudem eine verknüpfte Notebook-Rohquelle (notebook://notebooks/9550fd1e-83b5-49c9-9cc8-455b93646edf) technisch nicht auslesbar war9, stützt sich diese Bewertung auf die vorliegenden Projektdokumente, mathematische Modellrechnungen sowie etablierte Vergleichsarchitekturen (CockroachDB, TiKV, Google Spanner, Amazon Aurora DSQL und Morph)1.
Ob ein 2PC-Raft-Modell in einer dezentralen Landschaft tatsächlich hohe Resilienz mit geringem Aufwand verbindet, entscheidet sich an drei architektonischen Stellschrauben. Erstens sinkt die Verfügbarkeit bei Cross-Shard-Transaktionen multiplikativ mit der Anzahl beteiligter Raft-Gruppen, weshalb verteilte Schreibzugriffe auf möglichst wenige Shards begrenzt werden müssen1. Zweitens verursacht ein naives synchrones 2PC über Raft vier sequenzielle Konsens-Schreibzyklen samt fsync, die sich erst durch Parallel Commits oder Async Commit auf einen Konsens-Round-Trip halbieren lassen11. Drittens verlangt eine wirtschaftliche Speicherarchitektur die strikte Trennung zwischen 3-fach-Raft-Replikation für aktive Transaktionsdaten und nachgelagertem Erasure Coding (z. B. Reed-Solomon 6+3) für kalte Blöcke, da Erasure Coding auf dem heißen Schreibpfad massive Tail-Latenz-Einbrüche auslöst10.
Architektonische Einordnung und Protokollmechanik
Klassisches Two-Phase Commit auf Einzelknotenbasis scheitert in verteilten Systemen am Blocking-Problem: Fällt der Koordinator aus, nachdem die Teilnehmer in Phase 1 PREPARED gemeldet und lokale Sperren gesetzt haben, verharren diese Ressourcen im ungewissen Zustand (in-doubt), bis der Koordinator wiederhergestellt ist2. Das 2PC-Raft-Modell löst dieses Problem nicht über ein drittes Protokollstadium, sondern durch Zustandsmaschinen-Replikation der Protokollteilnehmer1. Jeder logische Shard bildet eine eigenständige Raft-Gruppe – typischerweise mit drei oder fünf Replikaten über getrennte Fehlerdomänen hinweg1.
Im Zielentwurf für dezentrale Cross-Shard-Blocktransaktionen verzichtet ThemisDB konsequent auf einen monolithischen globalen Koordinator1. Stattdessen ermittelt Rendezvous Hashing (Highest Random Weight) über Mandanten- und Objekt-ID einen stabilen Home-Shard1. Der jeweilige Raft-Leader dieser Home-Gruppe fungiert als Transaktionskoordinator, protokolliert Transaktions-ID, erwartete Vorversion, beteiligte Shards, Platzierungsepoche sowie die unveränderliche COMMIT- oder ABORT-Entscheidung im eigenen Raft-Log1. Fällt dieser Leader aus, wählt die Home-Raft-Gruppe innerhalb der konfigurierten Election-Timeouts (bei ThemisDB standardmäßig 150–300 ms bei 50 ms Heartbeat-Intervall) einen neuen Leader, der ausstehende Phase-2-Zustellungen idempotent fortsetzt1.
Bemerkenswert ist die klare Abgrenzung des Zielentwurfs gegen 3PC1. Obwohl der bestehende CrossShardTransactionCoordinator noch eine 3PC-Variante mit PreCommit-Phase bereitstellt8, belegen theoretische und praktische Analysen, dass 3PC ein synchrones Netzwerk beziehungsweise ein reines Fail-Stop-Fehlermodell voraussetzt4. Bei realen Netzwerkpartitionen (Asynchronie) können isolierte Teilgruppen oder mehrere Koordinatoren widersprüchliche Commit- und Abort-Entscheidungen treffen und damit die Konsistenz brechen3. Das 2PC-Raft-Modell ordnet sich nach dem PACELC-Theorem hingegen als PC/EC-System ein: Bei Partitionen priorisiert es Konsistenz vor Schreibverfügbarkeit (Minderheiten-Partitionen werden schreibgeschützt oder verweigern Commits), und im Normalbetrieb garantiert es strikte Konsistenz vor minimaler Latenz1.
In der gegenwärtigen Codebasis von ThemisDB besteht jedoch noch eine erhebliche Lücke zwischen diesem Entwurf und der betrieblichen Realität1. Die interne Governance-Dokumentation (DIS-05 / CC-5) weist fünf Klassen nach, die das Recovery-Interface IRecoverableTwoPhaseCoordinator implementieren (TwoPhaseCommitCoordinator, CrossShardTransactionCoordinator, DistributedTransactionCoordinator, GlobalTransactionManager und DistributedTransactionManager), ohne dass ein kanonischer Standardkoordinator festgelegt wäre8. Eine unter DIS-05 erfolgte Angleichung betrifft lediglich die Phasenordnung zwischen TwoPhaseCommitCoordinator und CrossShardTransactionCoordinator, nicht jedoch deren Transporte, WAL-Formate oder Recovery-Verhalten8. Da eine mit Koordinator A begonnene Transaktion nicht durch Koordinator B wiederhergestellt werden kann8, entstünde beim heutigen Produktiveinsatz ein erheblicher operativer Wartungsaufwand. Hinzu kommen Hinweise aus einem zusammenfassenden Evaluierungsdokument auf unbegrenztes WAL-Wachstum unter Raft und Race-Conditions bei Gossip-Topologie-Updates10, deren detaillierte Ursachen mangels Zugriff auf die zugehörige Notebook-Quelle nicht unabhängig verifiziert werden konnten9.
Bewertung der Ausfallsicherheit und Verfügbarkeitsgrenzen
Die Fehlertoleranz eines 2PC-Raft-Modells stützt sich auf das Mehrheitsquorum innerhalb jeder Shard-Gruppe: Eine Gruppe mit 2f + 1 Replikaten verkraftet bis zu f gleichzeitige Knotenausfälle ohne Datenverlust und bei fortlaufendem Schreibbetrieb1. Da bei einer Cross-Shard-Transaktion jedoch alle beteiligten Participant-Gruppen sowie die Home-Koordinator-Gruppe im PREPARED-Schritt ihr jeweiliges Quorum erreichen müssen – eine Mehrheit der betroffenen Gruppen genügt ausdrücklich nicht –, sinkt die Gesamtverfügbarkeit multiplikativ mit der Zahl der beteiligten Shards1.
Unter der Annahme unabhängiger Knoten-Ausfallwahrscheinlichkeit p in getrennten Fehlerdomänen folgt die Verfügbarkeit einer einzelnen 3er-Raft-Gruppe der Gleichung  und die einer 5er-Raft-Gruppe der Gleichung 15. Für eine Transaktion über S unabhängige Shard-Gruppen ergibt sich die Gesamtverfügbarkeit zu 15:
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
In der streng sequenziellen Ausprägung des ThemisDB-Zielentwurfs schreibt der Client zunächst die Transaktionsankündigung in den Home-Shard, danach loggen alle Teilnehmer ihre Blöcke plus PREPARED-Status über ihr jeweiliges Raft-Quorum, anschließend persistiert der Home-Shard COMMIT, verteilt die Entscheidung in Phase 2 zur erneuten Raft-Persistierung an alle Teilnehmer und veröffentlicht erst nach Erhalt sämtlicher Bestätigungen den neuen Manifest-Zeiger durch einen weiteren Raft-Schreibvorgang1. Zwar laufen parallele Block-Uploads zwischen Client und Shard-Leadern nebenläufig, auf dem kritischen Pfad addieren sich dennoch zwei Inter-Shard-RTTs und vier sequenzielle Intra-Shard-Raft-Zyklen inklusive fsync1.
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

Schließlich reduziert die im Block-Zielentwurf gewählte Rendezvous-Hashing-Platzierung den Routing-Aufwand gegenüber dem an anderer Stelle in ThemisDB beschriebenen Consistent-Hash-Ring mit 150 virtuellen Knoten je Shard1. Rendezvous Hashing benötigt keine Ring-Metadatenstruktur, liefert gleichmäßige Verteilung ohne Hotspot-Neigung und verschiebt bei Knotenausfällen nur minimal viele Zuordnungen5. Der Rechenaufwand von O(N) Hash-Berechnungen pro Routing-Vorgang bleibt bis in den Bereich von rund 100 Shard-Gruppen problemlos beherrschbar und entfällt beim Lesen ganz, sobald die ausgewählten Shard-IDs im Objekt-Manifest festgehalten sind1.
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

Deterministische Systeme wie Calvin sparen zwar die zweite Commit-Phase vollständig ein, schließen jedoch interaktive Multi-Statement-Transaktionen aus11. Serverlose Cloud-Dienste wie Aurora DSQL reduzieren den Betriebsaufwand nahezu auf null, binden das System jedoch an einen einzelnen Cloud-Anbieter und schränken gemäß einer unabhängigen Analyse der Preview-Version Datenmodell sowie Transaktionsgröße erheblich ein14. Ein optimiertes 2PC über Per-Shard-Raft mit Parallel Commits und HLC vermeidet beides: Es bewahrt volle ACID-Fähigkeit und On-Premise-Souveränität, ohne Spezialuhren vorauszusetzen10.
Handlungsempfehlungen für die Umsetzung
Um das bewertete 2PC-Raft-Modell aus dem gegenwärtigen Entwurfs- und Fragmentierungsstatus1 in eine belastbare, betriebseffiziente Produktionsarchitektur zu überführen, sollten sieben architektonische Maßnahmen priorisiert werden:
Konsolidierung auf einen kanonischen Koordinator und ein einheitliches WAL: Alle verteilten Schreibpfade müssen auf ein gemeinsames ITransactionCoordinator-Interface mit einer einheitlichen UnifiedTransactionWAL zusammengeführt werden8. Ein absturzbedingter Koordinatorwechsel muss offene Phase-2-Entscheidungen automatisiert und idempotent neu zustellen (re-drive), anstatt sie wie im aktuellen CrossShardTransactionCoordinator lediglich als Wiederaufnahmekandidaten zu melden1.
Umstellung von naivem 2PC auf Parallel Commits: Statt vier sequenzieller Raft-Schreibzyklen und zweier Inter-Shard-RTTs sollte die Home-Shard-Gruppe einen STAGING-Transaktionseintrag parallel zu den PREPARE-Writes der Teilnehmer-Shards replizieren11. Dies halbiert die Cross-Shard-Commit-Latenz im Multi-Region- wie im Cross-AZ-Betrieb ohne Abstriche bei der Atomarität11.
Ersatz dauerhafter Reader Pins durch Leader Leases und Closed Timestamps: Da ein Raft-Quorum-Write für jeden Lesezugriff den Leseaufwand vervielfacht und abgestürzte Clients verwaiste Pins hinterlassen1, sollten streng konsistente Lesevorgänge über zeitlich begrenzte Leader Leases (0 Konsens-RTTs) und leicht zeitversetzte Lesevorgänge über Closed Timestamps auf lokalen Followern bedient werden15.
Ablösung zentraler Sequencer durch Hybrid Logical Clocks (HLC): Um keinen zentralen Zeitgeber-Flaschenhals wie beim TSO-Modell zu schaffen10, sollte die transaktionale Ordnung vollständig über HLC erfolgen30. Betriebsseitig genügt ein einheitliches chrony-Setup über konsistente Zeitquellen bei deaktivierter VM-Live-Migration sowie eine Überwachung der Clock-Offset-Metriken deutlich unterhalb der max-offset-Grenze27.
Begrenzung des Cross-Shard-Fan-outs und RF=5 für Home-/Katalog-Shards: Weil die Transaktionsverfügbarkeit exponentiell mit der Zahl beteiligter Shards sinkt ()15, müssen Objekte und zugehörige Metadaten nach Möglichkeit lokal auf demselben Shard gruppiert werden. Kritische Home- und Katalog-Shards sollten mit RF=5 über fünf Fehlerdomänen laufen, während normale Datenshards mit RF=3 ein günstigeres Verhältnis aus Aufwand und Ausfallsicherheit erzielen7.
Striktes Storage-Tiering (Hot Raft auf NVMe, Cold RS 6+3): Aktive Raft-Logs und heiße RocksDB-Daten gehören zwingend auf lokale NVMe-Laufwerke mit 3-fach-Raft-Replikation, um fsync-Ausreißer zu vermeiden10. Erst unveränderliche, abgeschlossene 256-KiB-Blöcke werden im Hintergrund auf Reed-Solomon 6+3 transkodiert, wodurch 45 % bis 63 % physischer Speicherplatz gespart werden, ohne den transaktionalen Schreibpfad mit Read-Modify-Write-Latenzen zu belasten1.
Verbindliche Chaos- und Recovery-Abnahmetests vor Produktivfreigabe: Vor jedem produktiven Rollout sind die im Zielentwurf definierten realen Netzwerk- und Storage-Tests vollständig nachzuweisen – insbesondere Quorum-Verlust während PREPARE und Phase 2, Leader-Wechsel unter Last, WAL-Compaction-Grenzen, Block-Korruption sowie die Bereinigung der dokumentierten Risiken bei WAL-Wachstum und Gossip-Topologie-Updates1.
Referenzen
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
