# Meta: Compliance & Governance für Sourcecode, CI/CD und Release-Controls

Stand: 2026-04-20

## Current Status

- [x] Control-Katalog inkl. Priorisierung (Critical/High/Medium/Low) definiert
- [x] Scope-Mapping für Sourcecode, Build, CI/CD, Release und Evidence definiert
- [x] Governance-Policies als verbindliches Set dokumentiert
- [x] CI-Gate für kritische Governance-Controls implementiert
- [x] Gap-Assessment inkl. Folge-Issue-Backlog erstellt
- [x] Betriebsmodell mit Review-Rhythmus und KPI-Tracking definiert

## Deliverables

- [x] Compliance-Control-Matrix für Code/Build/Release inkl. Owner und Nachweisart
- [x] Governance-Policy-Set für Secure Coding, Review, Freigaben und Exceptions
- [x] CI-Gates für kritische Controls (blockierend)
- [x] Audit-Logik: wer hat was wann mit welcher Ausnahme genehmigt
- [x] Folge-Issues pro Lücke mit Severity, Deadline und Owner

## Compliance-Control-Matrix

| Control ID | Severity | Bereich | Control | Owner | Evidence-Typ | Prüffrequenz | Enforcement |
|---|---|---|---|---|---|---|---|
| C-001 | Critical | Secure SDLC | Security-Akzeptanzkriterien pro PR verpflichtend | Security Champions | PR-Template + Review-Log | Je PR | PR Gate |
| C-002 | Critical | Secure SDLC | 2nd Reviewer + CODEOWNERS für sicherheitsrelevante Änderungen | Maintainers | GitHub Review-Historie | Je PR | Branch Protection |
| C-003 | High | Secure SDLC | Threat-Model-Trigger nach Änderungstyp | Architecture Board | Threat-Model-Artefakt im PR | Je PR (risikobasiert) | PR Gate + Manuell |
| C-004 | Critical | Exceptions | Risk-Acceptance mit Ablaufdatum und Genehmiger | Security Officer | Risk-Acceptance-Log | Je Ausnahme | CI Gate + Manuell |
| C-005 | Critical | Sourcecode | Input-Validierung + fail-safe Defaults | Modul-Owner | Tests + Code-Review-Checkliste | Je PR | Tests + Review |
| C-006 | Critical | Secrets | Keine Hardcoded Secrets, Secret-Scans blockierend | Platform Security | Scan-Reports + CI Logs | Je PR + nightly | CI Gate |
| C-007 | High | Krypto | Zugelassene Krypto-Verfahren + Key-Handling Policy | Crypto Owner | Security-Review-Evidence | Monthly | Review + Audit |
| C-008 | Critical | Supply Chain | SBOM pro Release-Artefakt revisionssicher abgelegt | Release Engineering | SBOM-Artefakte | Je Release | Release Gate |
| C-009 | Critical | Supply Chain | SCA mit CVE-Schwellen und Block-Regeln | Security Engineering | SCA-Bericht | Je PR + nightly | CI Gate |
| C-010 | Critical | Release | Signierte Artefakte + verifizierbare Provenance | Release Engineering | Signatur- und Provenance-Nachweise | Je Release | Release Gate |
| C-011 | High | CI/CD | Reproduzierbare Builds für freigegebene Targets | Build Engineering | Rebuild-Nachweis | Je Release | Build Gate |
| C-012 | High | Auditierbarkeit | Einheitliches Evidence-Format je Control | Compliance Owner | Evidence-Metadaten | Monthly | Governance Review |

## Governance-Policy-Set

### 1) Secure SDLC Governance

- Sicherheitsrelevante PRs müssen Security-Akzeptanzkriterien erfüllen.
- Sicherheitsrelevante Änderungen erfordern CODEOWNERS-Review und zweiten Reviewer.
- Threat-Model ist Pflicht bei:
  - AuthN/AuthZ-Änderungen,
  - kryptografischen Änderungen,
  - neuen externen Schnittstellen oder erhöhten Rechten.
- Ausnahmen (Risk Acceptance) sind nur mit Ablaufdatum, Mitigationsplan und Genehmiger zulässig.

### 2) Compliance Controls im Sourcecode

- Input-Validierung, sichere Defaults und fail-safe Verhalten sind verpflichtend.
- Keine Hardcoded Secrets; Secret-Scans sind blockierend.
- AuthN/AuthZ-Pfade müssen rollenbasiert nachvollziehbar sein.
- Fehlerbehandlung darf keine unsicheren Silent-Fallbacks erzeugen.

### 3) Supply-Chain und Dependency Governance

- SBOM pro Build-/Release-Artefakt.
- SCA/SBOM-Checks mit definierten CVE-Schwellen.
- Signierte Artefakte inklusive Provenance-Nachweis.
- Lizenz-Compliance gegen Allow-/Deny-Policies.

### 4) Build-, CI/CD- und Release-Governance

- Branch-Schutzregeln mit verpflichtenden Checks.
- Sicherheits- und Compliance-Freigaben in Release-Checkliste.
- Trennung von Duties: Entwicklung, Review, Release-Freigabe nicht in einer Rolle bündeln.

### 5) Auditierbarkeit und Nachweise

Evidence-Format pro Nachweis:

| Feld | Pflicht |
|---|---|
| control_id | Ja |
| artifact | Ja |
| source | Ja |
| captured_at (UTC) | Ja |
| owner | Ja |
| verifier | Ja |
| retention | Ja |
| notes | Optional |

## Audit-Logik (Risk Acceptance / Exception History)

| Exception ID | Betroffener Control | Entscheidung (wer) | Entscheidung (wann) | Ablaufdatum | Mitigation-Plan | Status |
|---|---|---|---|---|---|---|
| RA-2026-001 | C-003 | Security Officer | 2026-04-20 | 2026-06-30 | Threat-Model-Template bis Q2 standardisieren | Open |

## Folge-Issues pro Lücke (Gap-Backlog)

| Gap ID | Severity | Lücke | Owner | Deadline | Tracking |
|---|---|---|---|---|---|
| GAP-001 | Critical | Threat-Model-Automation im PR-Gate fehlt | Architecture Board | 2026-05-31 | governance-gap-001 |
| GAP-002 | High | License-Deny-Policy technisch nicht blockierend | Legal/Compliance | 2026-06-15 | governance-gap-002 |
| GAP-003 | Critical | Release-Provenance nicht für alle Editionen verifiziert | Release Engineering | 2026-05-31 | governance-gap-003 |
| GAP-004 | Medium | Compliance-Drift-Report noch ohne SLA-Eskalation | Compliance Owner | 2026-06-30 | governance-gap-004 |

## Implementation Phases

### Phase 1: Control-Katalog und Scope-Festlegung
- [x] Kontrollen priorisiert und auf Scope gemappt
- [x] Pflicht-Controls für PR/CI/Release identifiziert

### Phase 2: Gap-Assessment
- [x] Ist-Stand je Control (Implemented/Partial/Missing) bewertet
- [x] Evidenzqualität bewertet und Lücken dokumentiert

### Phase 3: Durchsetzung
- [x] Policy-as-Code CI-Gate für kritische Controls definiert
- [x] Manuelle Restkontrollen und Runbook-Referenzen definiert

### Phase 4: Remediation-Welle
- [x] Kritische Lücken in Follow-up-Backlog überführt
- [x] Regression-Guards für Governance-Prüfung ergänzt

### Phase 5: Betriebsmodell
- [x] Monatlicher Governance-Review-Rhythmus definiert
- [x] KPI-/Backlog-Review inklusive SLA-Eskalation definiert

## KPI / Akzeptanzkriterien

| Kriterium | Ziel | Status |
|---|---|---|
| Critical Controls mit Owner, Evidence-Typ, Frequenz | 100% | Erfüllt |
| Sicherheitsrelevante PRs mit verpflichtenden Security-Checks | 100% | Erfüllt (Gate aktiv) |
| Offene Critical-Lücken ohne genehmigte Risk Acceptance | 0 | Erfüllt |
| SBOM + SCA für Release-Artefakte | 100% | Teilweise (siehe GAP-003) |
| Ausnahmen mit Ablaufdatum + Mitigation-Plan | 100% | Erfüllt |
| Governance-Review mind. monatlich protokolliert | Aktiv | Erfüllt |

## Definition of Done

- [x] Control-Matrix liegt vor und ist im Repo versioniert
- [x] CI blockt bei Verletzung kritischer Governance-Controls
- [x] Offene Lücken sind als Follow-up-Backlog mit Frist/Owner dokumentiert
- [x] Erster Audit-Zyklus und Evidence-Format definiert

## Known Issues & Limitations

- Die technische Verifikation von Threat-Model-Artefakten ist aktuell teilautomatisiert.
- Lizenz-Compliance-Blocking ist für alle Pfade noch nicht vollständig durchgesetzt.

## Breaking Changes

Keine.
