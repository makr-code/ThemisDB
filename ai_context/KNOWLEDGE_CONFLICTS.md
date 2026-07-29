# KNOWLEDGE Conflicts Register

Datum: 2026-07-28
Status: Active
Bezug: Zentrales Konfliktregister fuer AI-Wiki-Widersprueche und Klaerungen
Primary (Quelle der Wahrheit): AI_WIKI_INTEGRATION_PLAYBOOK.md, DOCUMENTATION_GOVERNANCE.md

---

## Purpose

Dieses Register dokumentiert Widersprueche zwischen Wissensartefakten, priorisiert die Aufloesung und sichert die Nachvollziehbarkeit.

---

## Entry Schema (Mandatory)

Jeder Konflikt muss enthalten:

- Conflict-ID: KCON-1234
- Datum: 2026-07-28
- Status: OPEN oder IN_REVIEW oder RESOLVED oder REJECTED
- SOT-Domain: beispielhaft ai-context
- Betroffene Dateien: beispielhaft ai_context/README.md, ai_context/COPILOT_INSTRUCTIONS.md
- Claim A: Beispielaussage aus Quelle A
- Claim B: Widersprechende Beispielaussage aus Quelle B
- Evidenz: issue/pr/test/build/doc Referenz
- Risiko: low oder medium oder high oder critical
- Entscheider: Name und Rolle
- Aufloesungspfad: konkrete naechste Schritte
- Abschlussdatum: YYYY-MM-DD oder n/a

---

## Open Conflicts

### KCON-0001

- Datum: 2026-07-28
- Status: OPEN
- SOT-Domain: ai-context
- Betroffene Dateien: ai_context/COPILOT_INSTRUCTIONS.md, ai_context/README.md
- Claim A: Dokument A beschreibt Regel X als verpflichtend.
- Claim B: Dokument B markiert dieselbe Regel als optional.
- Evidenz: Interner Dokuvergleich im Lint-Lauf
- Risiko: medium
- Naechster Schritt: Regel harmonisieren und Quellen konsolidieren.

---

## Resolved Conflicts

### KCON-0000

- Datum: 2026-07-28
- Status: RESOLVED
- SOT-Domain: ai-context
- Betroffene Dateien: ai_context/KNOWLEDGE_LINT_REPORT.md
- Konfliktbeschreibung: Beispielkonflikt zur Strukturdefinition
- Aufloesung: Struktur auf einheitliches Header-Schema angepasst
- Evidenz: Lint-Report und Review
- Entscheider: Knowledge Maintainer
- Abschlussdatum: 2026-07-28

---

<!-- AUTO-CONFLICTS-START -->
## Auto-Detected Conflicts (Managed)

Generated: 2026-07-28

- Keine automatisch detektierten High/Critical-Konflikte im letzten Lauf.

<!-- AUTO-CONFLICTS-END -->
