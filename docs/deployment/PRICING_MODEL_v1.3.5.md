# ThemisDB v1.3.5 — Abo- und Preismodell

Ziel: Realistisches, klar strukturiertes Preismodell mit fairen Rabatten für Reseller und Großkunden, abgestimmt auf die drei Editionen (Community, Enterprise, Hyperscaler).

## Editionsüberblick
- Community (kostenlos, Open-Source): 24 GB GPU-VRAM Limit, Single-Node, keine Enterprise-Plugins
- Enterprise (Abo, kommerziell): 256 GB GPU-VRAM, bis 100 Knoten, Enterprise-Plugins
- Hyperscaler (OEM/Custom): Unbegrenzter GPU-VRAM, unbegrenzte Knoten, OEM-Funktionen

## Preisstrategie (Grundsätze)
- Transparente pro-Node-Lizenzierung: Preis pro produktivem Knoten (physisch oder VM/Container, ≥ 4 vCPU)
- Jahres-Abos (Standard): 12 Monate mit Option auf Mehrjahresrabatte
- Monatliche Abos: +15% Aufpreis gegenüber Jahrespreisen (Flexibilität)
- Preisstufen nach Funktionsumfang und SLA

## Enterprise Preise (pro Knoten, pro Jahr)
- Standard: 2.000 € / Node / Jahr
  - SLA: 99,9% | Support: Werktage, 8×5 | Replikation: Ja | Plugins: Ja
- Pro: 5.000 € / Node / Jahr
  - SLA: 99,95% | Support: 24×7 | Geo-Replikation: Ja | Security (RBAC/Field-Enc/HSM): Ja
- Plus: 12.000 € / Node / Jahr
  - SLA: 99,99% | Support: 24×7/P1 < 30 min | Compliance/Audit: Ja | Advanced CDC | Priorisierte Roadmap

Hinweise:
- Mindestabnahme: 3 Nodes (Standard), 5 Nodes (Pro), 10 Nodes (Plus)
- Staging/Preprod: bis zu 50% der Prod-Knoten kostenlos (max. 10 Nodes)
- Edu/Non-Profit: -40% auf Standard/Pro (Nachweis erforderlich)

## Hyperscaler Preise (OEM/Custom)
- Listenpreise werden nicht veröffentlicht. Richtwerte:
  - OEM Cluster-Lizenz: ab 250.000 € / Jahr (inkl. unbegrenzte Nodes, OEM-Support)
  - Dedicated Engineering: 2.000 € / Person-Tag
  - Premium SLA (99,995%): Zuschlag 10–20% auf Vertragssumme
- Abrechnung: All-inclusive Paket mit individuellen SLAs, Hardwareprofile und Rollout-Plan

## Add-on Module (Enterprise/Hyperscaler)
- HSM Provider: 10.000 € / Jahr (pro Mandant)
- Compliance & Audit Suite: 5.000 € / Jahr (pro Mandant)
- Advanced Observability (OTLP/Tracing Dashboards): 3.000 € / Jahr (pro Deployment)
- LLM Integration (llama.cpp enablement): 4.000 € / Jahr (pro Deployment)

## Rabatte
### Mengenrabatte (Enterprise)
- 10–24 Nodes: -10%
- 25–49 Nodes: -15%
- 50–99 Nodes: -20%
- ≥100 Nodes: -30%
- ≥250 Nodes: -35%

Mehrjahresrabatt (Enterprise/Hyperscaler)
- 24 Monate: -5% zusätzlich
- 36 Monate: -10% zusätzlich

Vorabzahlung (Upfront)
- Jahrespreis im Voraus: -3%
- 3 Jahre im Voraus: -7%

### Reseller-Rabatte
- Basis-Marge: 25% auf Listenpreise (Enterprise)
- Zielerreichung (Quartal): +5% Bonus-Marge ab 100 Nodes Umsatz
- Deal-Registrierung: +5% (bei frühzeitiger Registrierung und aktiver Begleitung)
- Services-Bündelung: Reseller behält 100% Marge auf eigene Services (Implementierung, Migration, Betrieb)

### Großkunden-/Rahmenverträge
- Volumen > 500 Nodes oder Multi-Region: individuelle Konditionen
- Typischer Rahmen: Listenpreis -30% bis -45% + Services-Paket
- Preisgleitklauseln: Jährliche Anpassung ≤ 3% (Inflationskorridor), gedeckelt

## SLA- und Support-Modelle
- Standard (inkl. in Enterprise Standard): 99,9%, 8×5, P1 Reaktion < 4h
- Premium (Enterprise Pro): 99,95%, 24×7, P1 < 1h, Eskalationspfad
- Elite (Enterprise Plus/Hyperscaler): 99,99%+, 24×7, P1 < 30 min, dedizierter TAM

SLA Credits (bei Nichteinhaltung):
- 99,9% verfehlt: 5% Gutschrift
- 99,95% verfehlt: 10% Gutschrift
- 99,99% verfehlt: 15% Gutschrift

## Zahlungsbedingungen & Währung
- Zahlungsziel: Net 30 Tage
- Währungen: EUR (Standard), USD (auf Anfrage)
- Wechselkurs-Fixierung: Für 30 Tage ab Angebot
- Steuern/Abgaben: Exklusive, gemäß Land/Region

## Lizenzmetriken & Definitionen
- Node: Produktionsinstanz mit ≥4 vCPU (VM/Container/Physisch)
- Cold Standby: kostenlos
- Hot Standby: zählt als 0,5 Node
- Dev/Test: bis zu 50% der Prod-Nodes kostenlos (max. 10)

## Upgrade-/Downgrade-Regeln
- Community → Enterprise: pro-rata ab Upgrade-Datum
- Enterprise Tierwechsel (Standard/Pro/Plus): monatsgenau, pro-rata
- Enterprise → Hyperscaler: neue OEM-Vertragskonditionen
- Downgrade nur nach Vertragsende/Mindestlaufzeit möglich

## Beispiel-Szenarien
1) Startup SaaS (8 Nodes, Enterprise Standard, 12 Monate)
- Listenpreis: 8 × 2.000 € = 16.000 €
- Mengenrabatt (≥10? nein): 0%
- Vorabzahlung: -3% → 15.520 €
- Dev/Test (gratis bis 4 Nodes): inkl.
- Total: 15.520 € / Jahr

2) Mid-Market Analytics (40 Nodes, Enterprise Pro, 12 Monate)
- Listenpreis: 40 × 5.000 € = 200.000 €
- Mengenrabatt (25–49): -15% → 170.000 €
- Mehrjahr (24 Monate): -5% → 161.500 € / Jahr
- Vorabzahlung: -3% → 156.655 € / Jahr
- Total 2 Jahre: 313.310 €

3) Global Enterprise (120 Nodes, Enterprise Plus, 36 Monate)
- Listenpreis: 120 × 12.000 € = 1.440.000 € / Jahr
- Mengenrabatt (≥100): -30% → 1.008.000 € / Jahr
- Mehrjahr (36 Monate): -10% → 907.200 € / Jahr
- SLA Elite inkl.
- Total 3 Jahre: 2.721.600 €

4) Hyperscaler OEM (Unlimited, 3 Regionen)
- OEM Basis: 250.000 € / Jahr
- Premium SLA Zuschlag: +15% → 287.500 € / Jahr
- Dedicated Engineering (40 PT): 80.000 €
- Total: 367.500 € / Jahr (exkl. Steuern)

## Preisannahmen & Fairness
- Community bleibt kostenlos: schützt 80% der typischen Deployments
- Enterprise deckt Unternehmensanforderungen mit klarer Skalierung
- Hyperscaler ist individuell, wertbasiert
- Rabatte sind fair, transparent und volumen-/leistungsbezogen

## Governance & Änderungen
- Preise gültig ab: 01.01.2026 (v1.3.5)
- Jährliche Überprüfung basierend auf Nutzung, Markt, Kosten
- Änderungen nur nach Vorankündigung (60 Tage)

## Kurzfassung (Cheat-Sheet)
- Enterprise Standard: 2.000 € / Node / Jahr
- Enterprise Pro: 5.000 € / Node / Jahr
- Enterprise Plus: 12.000 € / Node / Jahr
- Hyperscaler OEM: ab 250.000 € / Jahr (custom)
- Mengenrabatte bis -35%, Reseller Marge 25–35%, Mehrjahresrabatte bis -10%
- SLA: 99,9% / 99,95% / 99,99%+ je nach Tier

—
Status: Vorschlag. Zur Freigabe durch Vertrieb/Finanzen.
