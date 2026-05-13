# ThemisDB Website – Marketing-Inhalte

Dieser Ordner enthält die Inhalte (Copy, Struktur, Block-Anweisungen) für die öffentliche
Marketing-Website von ThemisDB. Alle Dateien sind als **Gutenberg-kompatible Vorlagen**
aufgebaut und können direkt als Grundlage für WordPress-Pages genutzt werden.

## Pflegeverantwortung
- **Owner:** Product Marketing / Documentation Team
- **Review:** fachliche Freigabe durch Produktverantwortliche, QA-Freigabe über Doku-Checklisten

## Abgrenzung: Generiert vs. manuell
- **Manuell (hier):** redaktionelle Webseitentexte und Seitenstruktur
- **Generiert (nicht hier):** Exporte, Build-Outputs und abgeleitete Artefakte in `docs/_generated/`
- **Review-/Report-Artefakte (nicht hier):** nach `docs/reviews/` und `docs/reports/`

## Struktur

```
docs/website/
├── README.md          # Diese Übersicht
└── pages/             # Einzelne Landingpages (je eine .md-Datei pro Seite)
    ├── themisdb.md              # /themisdb – Haupt-Landingpage
    ├── kritis-airgap.md         # /kritis-airgap – KRITIS, Behörden, Blaulicht
    ├── enterprise.md            # /enterprise – Enterprise / Plattform-Teams
    ├── militaer-verteidigung.md # /militaer-verteidigung – Militär & Verteidigung
    └── developer.md             # /developer – Entwickler & PoC
```

## Verwendung

> Hinweis zur Toolchain: Inhalte unter `docs/website/**` sind Marketing-Quellen und
> nicht Teil des MkDocs-Navigationsbaums aus `mkdocs.yml`.
> Der konsolidierte Docs-Build-/Publish-Flow ist in `docs/README-DOCUMENTATION.md` beschrieben.

### WordPress / Gutenberg

Jede `.md`-Datei enthält:

- **Meta-Informationen** (WordPress-Slug, Meta-Title/Description, OG-Tags)
- **Block-Anweisungen** als HTML-Kommentare (z. B. `<!-- Gutenberg: Cover Block -->`)
- Den eigentlichen **Seiteninhalt** in Markdown (Überschriften, Absätze, Listen, Tabellen)

Um eine Seite in WordPress anzulegen:

1. Neue Seite erstellen, Slug aus den Meta-Infos übernehmen.
2. Gutenberg-Code-Editor (`</>`) öffnen und den Inhalt abschnittweise als passende Blöcke einfügen.
3. Buttons/CTAs als **Button-Block** mit Link zu `/kontakt` (oder dem jeweiligen Kontaktformular) anlegen.
4. Tabellen als **Table-Block** übernehmen.
5. FAQ-Abschnitte als **FAQ-Block** oder **Accordion-Block** (je nach Theme) umsetzen.

### Datenpflege

Alle Zahlen, Claims und Feature-Beschreibungen basieren auf Version 3.0 (8. März 2026) der
internen Quelle `docs/de/THEMISDB_MONETARY_VALUATION_ANALYSIS.confidential.md`.
Bei Updates an Features oder Preisen bitte diese Quelle zuerst prüfen und dann die betroffenen
Landingpages synchronisieren.

## CTA-Definition (global)

| CTA | Text | Ziel |
|-----|------|------|
| Primär | **Sales kontaktieren** | `/kontakt` |
| Sekundär | **Demo anfragen** | `/kontakt?intent=demo` |
| Tertiär | **Kurzes Gespräch buchen** | `/kontakt?intent=call` |

Kontakt-Teaser unterhalb jedes CTA-Buttons:
> „Antwort innerhalb von 24 Stunden (Werktage). Auf Wunsch NDA / Sicherheitsanforderungen (KRITIS/Defense)."

## Terminologie

| Begriff | Verwendung |
|---------|-----------|
| ThemisDB | Produktname (immer so, nie „Themis" allein) |
| Multi-Model | Relational, Document, Graph, Vector, Full-Text, Time-Series |
| Air-Gap | Vollständig offline betreibbar, keine Cloud-Verbindung |
| KRITIS | Kritische Infrastrukturen (BSI-Definition) |
| Production-Ready | v1.5.0 inkl. aller v1.7.0 Features |
| Enterprise Edition | Hyperscaler-Ready Features (Kubernetes Operator, HSM, Multi-Region, OTLP) |
| Military Edition | Air-Gap Mandatory, RAID-Sharding, Virtual SCIF, LoRA Field Adapters |

## QA-/Review-Anbindung
- `docs/PR_DOCUMENTATION_CHECKLIST.md`
- `docs/DOCUMENTATION_REVIEW_GUIDELINES.md`
- `docs/DOCUMENTATION_MERGE_PROTOCOL.md`
