<!--
  META-INFORMATIONEN
  WordPress-Slug:    /militaer-verteidigung
  Meta-Title:        ThemisDB Military Edition – Air-Gap & KI für Militär und Verteidigung
  Meta-Description:  ThemisDB Military Edition für Bundeswehr, NATO und Verteidigungsbehörden: Air-Gap zwingend, RAID-Sharding für Battlefield-Resilienz, lokale KI, Virtual SCIF. 68 % TCO-Vorteil vs. Legacy DefTech.
  OG-Title:          ThemisDB Military Edition – Datenplattform für DDIL-Umgebungen
  OG-Description:    Multi-Model-Datenplattform für SIGINT, Logistik, C2 und Sensorik ohne Cloud. Air-Gap, RAID-Sharding, Virtual SCIF, LoRA Field Adapters. Für Bundeswehr, BAAINBw, NATO-Behörden.
  Quelle:            docs/de/THEMISDB_MONETARY_VALUATION_ANALYSIS.confidential.md (v3.0, 08.03.2026)
-->

<!-- Gutenberg: Cover Block (Fullwidth Hero, dunkles Design) -->
# Military Edition: Air-Gap, Resilienz und lokale KI in DDIL-Umgebungen

**Multi-Model-Datenplattform für SIGINT, Logistik, C2, Sensorik und verteilte Einheiten –
vollständig ohne Cloud-Abhängigkeit.**

Für Bundeswehr, BAAINBw, NATO-Streitkräfte, Nachrichtendienste und Verteidigungsministerien der
EU-Staaten sowie Defense Prime Contractors.

<!-- Gutenberg: Buttons Block -->
[Sales kontaktieren](/kontakt) | [Defense-Anforderungen besprechen](/kontakt?intent=defense)

> Antwort innerhalb von 24 Stunden (Werktage). Vertraulichkeitsvereinbarung auf Wunsch vorab.
> Auf Wunsch: KRITIS/Defense-NDA, Sicherheitsabklärung, Beschaffungsberatung.

---

<!-- Gutenberg: Columns Block (4 Key Claims) -->
## Military Edition – Kernaussagen

| Air-Gap zwingend | RAID-Sharding | Lokale KI | 68 % TCO-Vorteil |
|---|---|---|---|
| **Vollständig offline – keine Cloud-Abhängigkeit** | **Battlefield-Resilienz bei Teilausfall** | **Keine externen APIs, keine Datenübertragung** | **€ 1,3 Mio. vs. € 4,1 Mio. (Legacy DefTech)** |

---

<!-- Gutenberg: Heading Block -->
## Warum ThemisDB im Verteidigungskontext?

Moderne Verteidigungsszenarien stellen besondere Anforderungen an Datensysteme:
Disconnected/Degraded/Intermittent/Limited (DDIL) Netzwerke, Resilienz bei Teilausfall,
vollständige Datensouveränität und lokale KI-Verarbeitung ohne externe Verbindungen.

ThemisDB adressiert diese Anforderungen direkt:

- **Air-Gap zwingend unterstützt:** Das System funktioniert vollständig ohne Netzwerk – alle
  Features inkl. LLM, STT/TTS und Bildanalyse laufen lokal
- **RAID-Sharding für Battlefield-Resilienz:** RAID 0/1/5/6 auf Datenbankebene; Ausfall einzelner
  Shard-Einheiten (z. B. durch Zerstörung) lässt das Gesamtsystem operabel
- **Lokale KI ohne API:** Zielerkennung, Sprachnotizen (STT/TTS), Dokumentenanalyse und
  semantische Suche laufen komplett lokal – kein Byte verlässt die sichere Umgebung
- **Federated Learning:** Gradienten statt Rohdaten werden synchronisiert – Bandbreiten-
  optimierung für taktische Netze
- **Virtual SCIF:** Software-basierte Schutzumgebung mit Hash-Chain-Audit-Trails
- **LoRA Field Adapters:** KI-Modelle werden für domänenspezifische Anpassungen im Feld trainiert

---

<!-- Gutenberg: Heading Block -->
## 5-Jahres-TCO: ThemisDB Military Edition vs. Alternativen

<!-- Gutenberg: Table Block -->
| Kostenart | ThemisDB Military Ed. | Legacy DefTech (ISAF-Ära) | Open-Source + Integrator | Bemerkung |
|-----------|----------------------|--------------------------|--------------------------|-----------|
| **Lizenzen (5 Jahre)** | € 250 k (€ 50 k/Jahr) | € 1.500 k (proprietär) | € 300 k (Support) | Military Edition inkl. SCIF-Features |
| **Hardware (gehärtet)** | € 300 k | € 600 k | € 400 k | ThemisDB läuft auf COTS-Hardware |
| **Betrieb & Wartung** | € 200 k | € 500 k | € 650 k | Reduzierter Aufwand durch Integration |
| **Personal (Security Engineers)** | € 400 k | € 1.000 k | € 700 k | Einheitliche Plattform senkt Aufwand |
| **Integration & Zertifizierung** | € 150 k | N/A (fertig) | € 500 k | BSI VS-NfD / STANAG-Audit |
| **Gesamt (5 Jahre)** | **€ 1.300 k** | **€ 4.100 k** | **€ 2.650 k** | |
| **Einsparung vs. ThemisDB** | – | **− € 2.800 k (− 68 %)** | **− € 1.350 k (− 51 %)** | |

> **ROI ThemisDB Military Edition:** € 1,35 Mio. – € 2,8 Mio. Einsparung über 5 Jahre.

---

<!-- Gutenberg: Heading Block -->
## Military-Feature-Matrix

<!-- Gutenberg: Table Block -->
| Feature | ThemisDB Military Edition | Legacy DefTech | Open-Source Stack |
|---------|--------------------------|----------------|-------------------|
| **Air-Gap (vollständig offline)** | ✅ Zwingend | ⚠️ Teils Cloud-abhängig | ⚠️ Komponenten ggf. online |
| **RAID-Sharding (DB-Ebene)** | ✅ RAID 0/1/5/6 | ❌ Externes Storage | ❌ Separat konfigurieren |
| **Virtual SCIF** | ✅ Native | ❌ Nicht verfügbar | ❌ Separat entwickeln |
| **Native LLM (lokal)** | ✅ llama.cpp eingebettet | ❌ API-only | ⚠️ Separat integrieren |
| **STT/TTS (lokal)** | ✅ Whisper.cpp + Piper | ❌ Cloud-Dienst | ⚠️ Separat integrieren |
| **Bildanalyse (lokal)** | ✅ ONNX/OpenCV | ❌ Cloud-Dienst | ⚠️ Separat integrieren |
| **LoRA Field Adapters** | ✅ Native | ❌ Nicht verfügbar | ❌ Separat entwickeln |
| **MQTT Broker (IoT/Sensorik)** | ✅ Native | ❌ Separat | ❌ Separat |
| **Multi-Model ACID** | ✅ Nativ | ⚠️ Teils | ❌ Separat |
| **Federated Learning** | ✅ Gradienten-Sync | ❌ Nicht verfügbar | ⚠️ Aufwändig |
| **Hash-Chain-Audit-Log** | ✅ Native | ⚠️ Zusatz | ⚠️ Zusatz |
| **COTS-Hardware-Unterstützung** | ✅ Standard x86/ARM | ❌ Proprietäre HW | ✅ Ja |

---

<!-- Gutenberg: Heading Block -->
## Zertifizierungs-Roadmap (geplant)

> **Hinweis:** Die nachfolgende Roadmap beschreibt geplante Zertifizierungsstufen. Aktuelle
> Zertifizierungsstände und detaillierte Zeitpläne werden im Sales-Gespräch mitgeteilt.

<!-- Gutenberg: Table Block -->
| Phase | Zeitraum (geplant) | Zertifizierung | Zielgruppe |
|-------|--------------------|----------------|-----------|
| **Phase 1** | Q3 2026 – Q1 2027 | VS-NfD (BSI Grundschutz) | Bundesbehörden, niedrige Geheimhaltung |
| **Phase 2** | Q2 2027 – Q4 2027 | NATO RESTRICTED + STANAG 4586 | NATO-Streitkräfte, alliierte Behörden |
| **Phase 3** | 2028 | VS-Vertraulich (BSI) | Höherer Geheimhaltungsgrad |
| **Phase 4** | 2029–2030 | Common Criteria EAL4+ | Vollständige Militär-Beschaffungsfähigkeit |

---

<!-- Gutenberg: Heading Block -->
## Typische Einsatzszenarien

- **SIGINT & Aufklärung:** Relationaler Datenspeicher + Vektor-Suche + semantische Analyse lokal
- **Logistik & Nachschub:** Zeitreihendaten, Geodaten, Graph-Routenoptimierung ohne Cloud
- **C2 (Command & Control):** Echtzeit-Lagebild mit CDC (~0 ms), RAID-Resilienz bei Netzausfall
- **Sensorik & IoT:** Nativer MQTT-Broker für Sensordaten; keine externe Broker-Infrastruktur
- **Sprachaufzeichnung & Analyse:** STT lokal (Whisper.cpp), Transkription und semantische Suche
- **Bildauswertung (IMINT):** ONNX/OpenCV eingebettet; Bild- und Videoanalyse offline
- **Feldausbildung & Training:** LoRA Field Adapters trainieren KI-Modelle domänenspezifisch im Feld

---

<!-- Gutenberg: Heading Block -->
## Preisrahmen Military Edition

<!-- Gutenberg: Table Block -->
| Tier | Preis/Jahr | Umfang |
|------|-----------|--------|
| **Military Edition Entry** | € 50.000 | Bis 10 Nodes, Air-Gap, RAID-Sharding, Virtual SCIF |
| **Military Edition Standard** | € 100.000 | Bis 25 Nodes, + LoRA Field Adapters, STANAG-Readiness |
| **Military Edition Enterprise** | € 250.000 | Unbegrenzte Nodes, + Klassifizierter Support (VS-NfD+), BAAINBw-Beschaffungsberatung |

*Detaillierte Preise, kundenindividuelle Konfigurationen und Rahmenverträge auf Anfrage.*

---

<!-- Gutenberg: Heading Block -->
## Häufig gestellte Fragen

<!-- Gutenberg: FAQ Block -->

### Welche Zertifizierungen hat ThemisDB aktuell?
ThemisDB ist aktuell nicht nach VS-NfD oder NATO RESTRICTED zertifiziert. Die
Zertifizierungs-Roadmap ist definiert. Details und Timing werden im Sales-Gespräch besprochen.

### Wie lange dauert eine typische Defense-Beschaffung?
Defense-Beschaffungen variieren stark je nach Behörde und Beschaffungsweg (Direktbeschaffung,
Rahmenvertrag, BAAINBw-Verfahren). Wir beraten Sie gerne zum optimalen Beschaffungspfad.

### Unterstützt ThemisDB den Betrieb auf gehärteter Hardware (Ruggedized)?
Ja. ThemisDB läuft auf Standard-COTS-Hardware (x86-64, ARM) – auch auf gehärteten Rüstungsgeräten.
Spezifische Hardware-Anforderungen werden im Solutions-Gespräch ermittelt.

### Was ist ein Virtual SCIF?
Ein Virtual SCIF (Sensitive Compartmented Information Facility) ist eine software-basierte
Schutzumgebung, die klassifizierte Daten innerhalb des Systems isoliert und mit Hash-Chain-Audit-
Trails vollständig protokolliert.

### Wie funktioniert RAID-Sharding im Gefechtsfeldkontext?
ThemisDB implementiert RAID 0/1/5/6 auf Datenbankebene. Wenn ein Shard-Knoten (z. B. eine
Einheit/ein Fahrzeug) ausfällt, bleibt das Gesamtsystem operabel – analog zu RAID auf
Festplattenebene, aber für verteilte Datenbanken im Feld.

### Können wir mit einem Pilotprojekt starten?
Ja. Wir empfehlen ein strukturiertes Pilotprojekt (4–8 Wochen) mit definiertem Usecase, Testdaten
und Abnahmekriterien. Unser Defense Solutions Team begleitet Sie von Anfang bis Ende.

---

<!-- Gutenberg: Cover Block (CTA-Sektion, dunkles Design) -->
## Defense-Anforderungen & Beschaffungsprozess abstimmen

ThemisDB ist für Verteidigungsszenarien gebaut – Air-Gap, Resilienz und lokale KI ohne Kompromisse.

<!-- Gutenberg: Buttons Block -->
[Sales kontaktieren](/kontakt) | [Defense-Briefing anfragen](/kontakt?intent=defense-briefing)

> Auf Wunsch: NDA vorab, Sicherheitsabklärung, Vertraulichkeitsstufe VS-NfD.
> Antwort innerhalb von 24 Stunden (Werktage).
