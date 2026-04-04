<!--
  META-INFORMATIONEN
  WordPress-Slug:    /kritis-airgap
  Meta-Title:        ThemisDB für KRITIS & Air-Gap – Datensouveränität ohne Cloud
  Meta-Description:  ThemisDB ist die einzige Multi-Model-Datenbank mit nativer KI, die vollständig offline (Air-Gap) und KRITIS-konform betrieben werden kann. 62 % TCO-Vorteil vs. Open-Source-Patchwork.
  OG-Title:          KRITIS-ready Datenplattform: Air-Gap, Datensouveränität, lokale KI
  OG-Description:    Wenn Cloud nicht erlaubt ist – ThemisDB liefert Multi-Model + KI vollständig offline. NIS2-konform, BSI-orientiert, für Behörden und Blaulicht-Organisationen.
  Quelle:            docs/de/THEMISDB_MONETARY_VALUATION_ANALYSIS.confidential.md (v3.0, 08.03.2026)
-->

<!-- Gutenberg: Cover Block (Fullwidth Hero) -->
# KRITIS-ready Datenplattform: Air-Gap, Datensouveränität, lokale KI

**Wenn Cloud nicht erlaubt ist – ThemisDB liefert Multi-Model + KI vollständig offline.**

Für Behörden, Rettungsdienste, Polizei, Energieversorger und alle Organisationen, die unter die
NIS2-Richtlinie fallen oder eigene Datensouveränitätsanforderungen haben.

<!-- Gutenberg: Buttons Block -->
[Sales kontaktieren](/kontakt) | [Sicherheitsanforderungen besprechen](/kontakt?intent=security)

> Antwort innerhalb von 24 Stunden (Werktage). Auf Wunsch NDA oder Vertraulichkeitsvereinbarung vorab.

---

<!-- Gutenberg: Columns Block (3 Key Claims) -->
## Auf einen Blick

| Air-Gap zwingend unterstützt | Lokale KI – keine Datenübertragung | 62 % TCO-Vorteil |
|---|---|---|
| Kein einziger Byte verlässt Ihre Infrastruktur | LLM, STT/TTS, Bildanalyse laufen vollständig lokal | vs. Open-Source-Patchwork über 5 Jahre |

---

<!-- Gutenberg: Heading Block -->
## Das Problem: Patchwork-Architekturen unter KRITIS-Bedingungen

Viele Behörden und KRITIS-Organisationen betreiben heute 5 oder mehr getrennte Systeme:
Relationale Datenbank, Suchindex, Dokumentenstore, Vektor-Datenbank, KI-Microservice.

Das Ergebnis:

- **Keine ACID-Konsistenz** über Systemgrenzen hinweg
- **Hohe Integrationskosten** – jede Schnittstelle ist ein Risikopunkt
- **Komplexer Betrieb** – 5 Systeme, 5 Update-Zyklen, 5 Monitoring-Konfigurationen
- **Latenz und Datensynchronisationsprobleme** zwischen den Komponenten
- **Air-Gap oft nicht vollständig** – Cloud-Abhängigkeiten einzelner Komponenten

<!-- Gutenberg: Heading Block -->
## Die Lösung: Ein System für alle Anforderungen

ThemisDB ersetzt das Patchwork durch eine einzige, kohärente Datenplattform:

- **Multi-Model in einer Engine:** Relational, Document, Graph, Vector, Full-Text, Time-Series
- **ACID über alle Modelle:** Konsistenz auch bei komplexen, modellübergreifenden Operationen
- **Native KI:** LLM (llama.cpp), STT/TTS (Whisper.cpp + Piper), Bildanalyse (ONNX/OpenCV)
- **Air-Gap-first:** alle Komponenten offline betreibbar, keine externen Abhängigkeiten
- **Observability:** OTLP/Prometheus nativ integriert – kein zusätzliches Tool nötig

---

<!-- Gutenberg: Heading Block -->
## 5-Jahres-TCO: ThemisDB vs. Open-Source-Patchwork

<!-- Gutenberg: Table Block -->
| Kostenart | ThemisDB On-Prem | PostgreSQL Patchwork (5 Systeme) | Bemerkung |
|-----------|-----------------|----------------------------------|-----------|
| **Lizenzen** | € 50 k | € 200 k (Enterprise Support) | ThemisDB: ein Lizenzvertrag |
| **Hardware** | € 150 k | € 300 k (5 separate Cluster) | Konsolidierung spart Hardware |
| **Betrieb & Wartung** | € 100 k | € 250 k | Reduzierter Aufwand durch Integration |
| **Integration** | € 0 (Single System) | € 400 k (5 Systeme synchronisieren) | Kein Inter-System-Aufwand |
| **Training** | € 50 k | € 100 k | Einheitliche Schulung |
| **Support** | € 200 k | € 200 k | Vergleichbar |
| **Gesamt (5 Jahre)** | **€ 550 k** | **€ 1.450 k** | **Einsparung: € 900 k (– 62 %)** |

*Cloud-Lösungen (AWS, Azure) sind in KRITIS-Szenarien nicht nutzbar (N/A). Kein Vergleich möglich.*

---

<!-- Gutenberg: Heading Block -->
## Security & Compliance: Was ThemisDB bietet

<!-- Gutenberg: List Block -->
- **Datenresidenz:** 100 % lokal – kein Datenabfluss, kein Cloud-Egress
- **Air-Gap:** vollständig offline betreibbar; alle Features inkl. KI ohne Netzwerk
- **Auditierbarkeit:** OTLP/Prometheus nativ; Hash-Chain-Audit-Log für lückenlose Nachverfolgung
- **Zugriffssteuerung:** rollenbasierte Zugriffsrechte, multi-tenantfähig
- **Hochverfügbarkeit:** Multi-Region Replication, automatisches Failover (Enterprise Edition)
- **HSM-Integration:** Hardware Security Module Support für Schlüsselverwaltung (Enterprise Edition)
- **NIS2-Orientierung:** Architektur entspricht den Grundprinzipien der NIS2-Richtlinie (Resilienz, Datenschutz, Monitoring)

> **Hinweis:** Spezifische BSI-Zertifizierungen (IT-Grundschutz, C5) sind in Planung. Details zur
> Zertifizierungs-Roadmap werden im Sales-Gespräch besprochen.

---

<!-- Gutenberg: Heading Block -->
## Feature-Vergleich: ThemisDB vs. typisches KRITIS-Patchwork

<!-- Gutenberg: Table Block -->
| Feature | ThemisDB | PostgreSQL + Elastic + Milvus + … |
|---------|----------|------------------------------------|
| **Multi-Model (ACID)** | ✅ Nativ | ❌ Separate Systeme, kein gemeinsames ACID |
| **Air-Gap** | ✅ Vollständig | ⚠️ Einzelne Komponenten ggf. Cloud-abhängig |
| **Native KI (LLM/STT/Vision)** | ✅ Eingebettet | ❌ Externe API oder separate Microservices |
| **Vektor-Suche** | ✅ Nativ + Embedding Cache | ⚠️ Separates Milvus/Weaviate nötig |
| **Full-Text-Suche** | ✅ Nativ | ⚠️ Separates Elasticsearch nötig |
| **OTLP/Prometheus** | ✅ Nativ | ❌ Separate Konfiguration je System |
| **Kubernetes Operator** | ✅ Nativ (Enterprise) | ⚠️ Separates Helm-Chart je System |
| **Betriebskomplexität** | ✅ Ein System | ❌ 5+ Systeme, 5+ Update-Zyklen |
| **Lizenzvertrag** | ✅ Einer | ❌ 5+ Lizenzen und Supportverträge |

---

<!-- Gutenberg: Heading Block -->
## Typische Einsatzszenarien im KRITIS-Umfeld

- **Rettungsdienste & Feuerwehr:** Einsatzdaten, Geodaten, Sprachaufnahmen und Bildanalyse in einem System – ohne Cloud
- **Polizei & Ermittlungsbehörden:** Relationale Falldaten, Dokumentensuche und Bildanalyse (ONNX) lokal
- **Energieversorger:** Zeitreihendaten, IoT (MQTT) und Graph-Topologien für Netzmanagementsysteme
- **Gesundheitswesen / Kliniken:** Patientendaten, Bildgebung und Sprachnotizen ohne Datenabfluss
- **Kommunalverwaltungen:** Geodaten, Dokumentenmanagement und semantische Suche On-Prem

---

<!-- Gutenberg: Heading Block -->
## Häufig gestellte Fragen

<!-- Gutenberg: FAQ Block -->

### Ist ThemisDB BSI-zertifiziert?
ThemisDB ist aktuell nicht BSI-zertifiziert. Eine BSI-Grundschutz- und VS-NfD-orientierte
Zertifizierungsroadmap ist vorhanden. Details und aktuellen Stand besprechen wir gerne im
persönlichen Gespräch.

### Welche KRITIS-Sektoren werden adressiert?
ThemisDB ist für alle BSI-KRITIS-Sektoren geeignet: Energie, Wasser, Ernährung, IT/TK,
Gesundheit, Finanz, Transport, Staat/Verwaltung sowie den Bereich Blaulicht/Sicherheitsbehörden.

### Läuft ThemisDB auf bestehender On-Prem-Hardware?
Ja. ThemisDB läuft auf Standard-COTS-Hardware (x86-64, ARM) unter Linux. Spezielle Hardware
ist nicht erforderlich. Mindestanforderungen und empfohlene Konfigurationen werden im
Erstgespräch ermittelt.

### Wie ist die Hochverfügbarkeit realisiert?
Die Enterprise Edition bietet native Multi-Region Replication und automatisches Failover.
RAID Sharding auf Datenbankebene (RAID 0/1/5/6) ermöglicht zusätzliche Resilienz ohne
externe Storage-Systeme.

### Können wir mit einem PoC starten?
Ja. Typischerweise genügen 2–4 Wochen für einen aussagekräftigen PoC auf Ihrer Hardware.
Unser Solutions-Team begleitet Sie von der Anforderungsaufnahme bis zum Ergebnisbericht.

### Was passiert, wenn das Netzwerk ausfällt?
ThemisDB ist Air-Gap-first: alle Funktionen inkl. KI, Suche und Observability laufen ohne
Netzwerk. Ein Netzwerkausfall hat keinen Einfluss auf den laufenden Betrieb.

---

<!-- Gutenberg: Cover Block (CTA-Sektion, Kontrastfarbe) -->
## Datensouveränität ohne Kompromisse

ThemisDB schließt die Lücke zwischen Datensicherheitsanforderungen und moderner KI-Nutzung –
vollständig On-Prem und Air-Gap-fähig.

<!-- Gutenberg: Buttons Block -->
[Sales kontaktieren](/kontakt) | [Sicherheitsanforderungen besprechen](/kontakt?intent=security)

> Antwort innerhalb von 24 Stunden (Werktage). Auf Wunsch Vertraulichkeitsvereinbarung vorab.
