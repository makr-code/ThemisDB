# Security Audit Report

**Stand:** 6. April 2026  
**Version:** v1.3.0  
**Kategorie:** 🔒 Security  
**Status:** 📝 Draft

---

## 📑 Inhaltsverzeichnis

- [1. Scope](#1-scope)
- [2. Methodik](#2-methodik)
- [3. Zusammenfassung der Ergebnisse](#3-zusammenfassung-der-ergebnisse)
- [4. Detaillierte Findings](#4-detaillierte-findings)
- [5. Empfehlungen](#5-empfehlungen)
- [6. Remediation-Plan](#6-remediation-plan)
- [7. Evidenz & Artefakte](#7-evidenz--artefakte)
- [8. Changelog](#8-changelog)

---

## 1. Scope

Beschreibung des Prüfungsumfangs (Systeme, Services, Versionen, Zeitfenster, Prüfumgebung).

| Komponente | Version | In-Scope | Hinweise |
|------------|---------|----------|----------|
| ThemisDB Server | x.y.z | ✅ | Release/Commit-Hash |
| Admin Tools | x.y.z | ✅ | WPF Suite |
| APIs/SDKs | x.y.z | ⚠️ | Fokus: Auth/RBAC |
| Infrastruktur | n/a | ❌ | Separater Audit |

## 2. Methodik

Beschreibt angewandte Standards und Verfahren.

- Referenzen: OWASP ASVS, NIST 800-53, ISO 27001/27002, BSI C5
- Methoden: Code Review, Konfigurationsanalyse, Black-/Grey-/Whitebox Tests
- Tools: OWASP ZAP/Burp, testssl.sh, SAST/DAST, SBOM-Analyse

## 3. Zusammenfassung der Ergebnisse

Kurze, managementtaugliche Zusammenfassung mit Risikoklassifizierung.

| Kategorie | Kritisch | Hoch | Mittel | Niedrig |
|-----------|---------:|-----:|------:|--------:|
| Auth/RBAC | 0 | 1 | 2 | 3 |
| API | 0 | 0 | 3 | 4 |
| Krypto/TLS | 0 | 0 | 1 | 2 |
| Logging/Audit | 0 | 0 | 1 | 1 |

## 4. Detaillierte Findings

Jedes Finding mit eindeutiger ID, Evidenz, Risiko, Impact, Reproduktion, Empfehlung, Status.

### FIND-001: Beispiel Finding Titel

- Schweregrad: Mittel  
- Kategorie: API Security  
- Betroffene Komponenten: /api/v1/...  
- Beschreibung: …  
- Evidenz/PoC: …  
- Risiko/Impact: …  
- Reproduktion: Schritte 1..n  
- Empfehlung: …  
- Status: Offen / In Bearbeitung / Behoben (Commit/Version)

## 5. Empfehlungen

Priorisierte Maßnahmenliste mit grober Aufwands-/Wirkungsschätzung.

| Maßnahme | Priorität | Wirkung | Aufwand |
|----------|-----------|---------|---------|
| Rate Limiting härten | Hoch | Hoch | Mittel |
| Secrets Rotation | Hoch | Mittel | Mittel |
| TLS Harden | Mittel | Mittel | Gering |

## 6. Remediation-Plan

Zeitplan, Verantwortliche, Meilensteine, Abnahme.

| Aufgabe | Owner | Zieltermin | Status |
|---------|-------|-----------:|--------|
| Fix FIND-001 | Security | 2026-01-15 | Offen |
| DAST erweitern | DevOps | 2026-01-22 | Offen |

## 7. Evidenz & Artefakte

Links/Hashes/Anhänge: Logs, Screenshots, Reports, SBOMs, Testskripte.

## 8. Changelog

- 2025-12-22: Initialer Entwurf (v1.3.0 Template)

