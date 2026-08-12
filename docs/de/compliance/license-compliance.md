# Lizenz-Compliance-Prozess

## Übersicht

ThemisDB implementiert einen automatisierten Lizenz-Compliance-Prozess, um sicherzustellen, dass alle Abhängigkeiten mit der Projektlizenz (MIT mit Government Clause) kompatibel sind und keine rechtlichen Risiken darstellen.

## Automatisierte Lizenz-Prüfung

### GitHub Actions Workflow

Der Lizenz-Compliance-Workflow (`.github/workflows/license-compliance.yml`) wird automatisch ausgeführt bei:

- **Pull Requests**: Prüft alle Änderungen an Abhängigkeiten
- **Push auf permanente Branches**: Validiert Lizenz-Compliance auf `develop`, `community`, `enterprise`, `hyperscaler`, `military` und `minimal`
- **Monatlicher Audit**: Automatische Prüfung am ersten Tag jedes Monats
- **Manuelle Auslösung**: Kann jederzeit über GitHub Actions UI gestartet werden

### Geprüfte Dateien

Der Workflow überwacht Änderungen an:

- `vcpkg.json` - C++ Abhängigkeiten (vcpkg)
- `package.json` / `package-lock.json` - Node.js/JavaScript Abhängigkeiten
- `pom.xml` - Java Maven Abhängigkeiten
- `build.gradle` - Java Gradle Abhängigkeiten
- `Cargo.toml` - Rust Abhängigkeiten
- `go.mod` - Go Abhängigkeiten
- `requirements.txt` / `Pipfile` - Python Abhängigkeiten
- `composer.json` - PHP Abhängigkeiten
- `.license-policy.json` - Lizenz-Policy-Änderungen

## Lizenz-Policy

Die Lizenz-Policy ist in `.license-policy.json` definiert und kategorisiert Lizenzen in drei Gruppen:

### 1. Erlaubte Lizenzen (Allowed) ✅

Diese Lizenzen sind ohne Einschränkungen erlaubt:

- **MIT** - Maximale Freiheit, minimale Einschränkungen
- **Apache-2.0** - Permissiv mit Patent-Schutz
- **BSD-2-Clause / BSD-3-Clause** - Permissiv mit Namensnennung
- **ISC** - Funktional identisch mit MIT
- **Unlicense / 0BSD / CC0-1.0** - Public Domain ähnlich
- **BSL-1.0** - Boost Software License
- **MPL-2.0** - Mozilla Public License (datei-basiertes Copyleft)

### 2. Eingeschränkte Lizenzen

#### Starkes Copyleft (Blocked) ⛔

Diese Lizenzen sind **blockiert**, da sie verlangen, dass abgeleitete Werke unter derselben Lizenz veröffentlicht werden:

- **GPL-2.0 / GPL-3.0** - GNU General Public License
- **AGPL-3.0** - GNU Affero General Public License

**Aktion**: Pull Request wird blockiert, Abhängigkeit muss ersetzt werden.

#### Schwaches Copyleft (Warning) ⚠️

Diese Lizenzen sind mit **Warnung** akzeptiert, wenn dynamic linking verwendet wird:

- **LGPL-2.1 / LGPL-3.0** - GNU Lesser General Public License
- **EPL-1.0 / EPL-2.0** - Eclipse Public License
- **CDDL-1.0 / CDDL-1.1** - Common Development and Distribution License

**Aktion**: Warnung wird angezeigt, aber Pull Request wird nicht blockiert.

**Beispiel**: FFmpeg verwendet LGPL-2.1, was akzeptabel ist, da ThemisDB FFmpeg als dynamische Bibliothek einbindet.

#### Proprietäre Lizenzen (Blocked) ⛔

- **Proprietary / Commercial / UNLICENSED**

**Aktion**: Pull Request wird blockiert.

### 3. Unbekannte Lizenzen (Unknown) ❓

Lizenzen, die nicht in der Policy definiert sind, gelten als nicht whitelisted und erfordern eine manuelle Überprüfung.

**Aktion**: Pull Request wird blockiert, bis eine Freigabe oder ein erlaubter Ersatz vorliegt.

## Workflow-Integration

### Pull Request Checks

Bei jedem Pull Request wird automatisch:

1. **Lizenz-Scan durchgeführt**: Alle in `vcpkg.json` deklarierten direkten und transitiven Pakete werden auf ihre Lizenzen geprüft
2. **Policy-Validierung**: SPDX-Lizenz-Ausdrücke werden gegen `.license-policy.json` ausgewertet
3. **Lizenz-SBOM erzeugt**: `vcpkg-license-sbom.json` und `license-summary.md` werden als Artefakte abgelegt
4. **Status-Check**: PR-Status wird auf "failed" gesetzt bei blockierten oder nicht whitelisted Lizenzen

### Beispiel Workflow-Summary

```markdown
## 📋 License Compliance Check

### ✅ Check Passed

Alle Abhängigkeiten entsprechen der Lizenz-Policy.

**Summary:**
- 🔴 Blocked: 0
- 🟡 Warnings: 1

[Lizenz-SBOM und Summary als Artefakte herunterladen](...)
```

### Blockierung von PRs bei Verletzungen

Wenn eine Lizenz-Verletzung erkannt wird:

1. ❌ Der PR-Check schlägt fehl
2. 📝 `license-summary.md` zeigt problematische Pakete, SPDX-Ausdrücke und Policy-Entscheidungen
3. 🚫 Der PR darf nicht gemerged werden, bis der Status-Check wieder grün ist
4. 📋 `vcpkg-license-sbom.json` dokumentiert die vollständige Paketliste für Audit und Release

## Prozess bei Lizenz-Verletzungen

### Für Entwickler

Wenn der Lizenz-Check fehlschlägt:

1. **Überprüfen Sie die Verletzung**:
   - Öffnen Sie den Workflow-Run und prüfen Sie die Details
   - Identifizieren Sie die problematische Abhängigkeit

2. **Maßnahmen**:
   
   **Option A: Abhängigkeit ersetzen** (bevorzugt)
   - Suchen Sie nach einer Alternative mit kompatibler Lizenz
   - Aktualisieren Sie die Abhängigkeiten
   - Pushen Sie die Änderungen

   **Option B: Ausnahme beantragen**
   - Erstellen Sie ein Issue mit dem Label `license-exception`
   - Begründen Sie, warum die Abhängigkeit notwendig ist
   - Warten Sie auf Review durch das Compliance-Team

3. **Re-Check**:
   - Der Workflow wird automatisch bei jedem Push erneut ausgeführt
   - Überprüfen Sie, ob die Verletzung behoben wurde

### Für Maintainer

Bei Ausnahme-Anfragen:

1. **Review**: Prüfen Sie die Begründung und rechtliche Risiken
2. **Entscheidung**: Genehmigen oder ablehnen
3. **Dokumentation**: Bei Genehmigung:
   - Fügen Sie die Ausnahme zu `.license-policy.json` unter `exceptions.list` hinzu
   - Hinterlegen Sie Paketname, exakten Lizenz-Ausdruck, Override-Aktion, Begründung, Freigabedatum und Genehmiger
   - Dokumentieren Sie die Begründung
   - Aktualisieren Sie die Compliance-Dokumentation

## Lizenz-Audit

### Monatlicher Audit

Am ersten Tag jedes Monats (3:00 UTC) wird automatisch ein vollständiger Lizenz-Audit durchgeführt:

- Scannt alle Abhängigkeiten
- Erstellt einen detaillierten Bericht
- Speichert Artefakte für 90 Tage
- Benachrichtigt bei Verletzungen

### Manueller Audit

Führen Sie jederzeit einen manuellen Audit durch:

```bash
# Via GitHub Actions UI
# 1. Gehen Sie zu Actions → Compliance — License Policy Gate
# 2. Klicken Sie auf "Run workflow"
# 3. Wählen Sie den Branch
# 4. Klicken Sie auf "Run workflow"
```

### Audit-Berichte

Alle Audit-Berichte werden als Artefakte gespeichert:

- `license-summary.md` - Review- und Audit-Zusammenfassung
- `vcpkg-license-sbom.json` - Lizenz-SBOM für direkte und transitive vcpkg-Abhängigkeiten

**Aufbewahrungsdauer**: 90 Tage

## Best Practices für Entwickler

### Vor dem Hinzufügen neuer Abhängigkeiten

1. **Lizenz prüfen**: Überprüfen Sie die Lizenz der Abhängigkeit
2. **Policy konsultieren**: Stellen Sie sicher, dass die Lizenz erlaubt ist
3. **Dokumentation**: Fügen Sie die Lizenz-Information zu Ihrem PR hinzu

### Bevorzugte Lizenzen

Bevorzugen Sie Abhängigkeiten mit diesen Lizenzen:

1. MIT
2. Apache-2.0
3. BSD-3-Clause
4. ISC

### Zu vermeidende Lizenzen

Vermeiden Sie Abhängigkeiten mit:

- GPL (alle Versionen) - außer bei dynamic linking
- AGPL - immer vermeiden
- Proprietäre Lizenzen
- Unklare oder fehlende Lizenz-Information

## Integration mit anderen Compliance-Prozessen

### SBOM (Software Bill of Materials)

Der Lizenz-Compliance-Workflow arbeitet mit dem SBOM-Workflow zusammen:

- SBOM enthält Lizenz-Informationen für alle Abhängigkeiten
- Wird bei jedem Release generiert
- Siehe `.github/workflows/sbom-ci.yml`

### Security Scanning

Der Security-Scan-Workflow (`.github/workflows/security-scan.yml`) enthält einen Basis-Lizenz-Check:

- Validiert Vorhandensein von Lizenz-Dateien
- Verweist auf den detaillierten Lizenz-Compliance-Workflow
- Teil der Gesamt-Sicherheitsstrategie

### Code Review

Bei Code Reviews:

1. Prüfen Sie den Status von `Compliance — License Policy Gate`
2. Laden Sie bei Bedarf `license-summary.md` oder `vcpkg-license-sbom.json` aus dem Workflow herunter
3. Überprüfen Sie Warnungen und Ausnahmen manuell
4. Dokumentieren Sie Lizenz-Entscheidungen im PR

## Compliance-Standards

Dieser Prozess erfüllt folgende Standards und Regulierungen:

- **BSI C5 (SSO-02)**: Software-Supply-Chain-Sicherheit
- **NIS2**: Netz- und Informationssicherheit
- **DSGVO**: Datenschutz-Grundverordnung
- **Executive Order 14028**: Improving the Nation's Cybersecurity (SBOM)
- **ISO 27001**: Informationssicherheits-Managementsystem

## Häufig gestellte Fragen (FAQ)

### F: Warum wird mein PR blockiert?

**A**: Der Lizenz-Check hat eine Abhängigkeit mit einer nicht-kompatiblen Lizenz gefunden. Überprüfen Sie den Workflow-Bericht für Details und ersetzen Sie die Abhängigkeit oder beantragen Sie eine Ausnahme.

### F: Was bedeutet "schwaches Copyleft"?

**A**: Schwaches Copyleft (z.B. LGPL) erfordert, dass Änderungen an der Bibliothek selbst unter derselben Lizenz veröffentlicht werden. Bei dynamic linking ist dies jedoch akzeptabel, da Ihr Code nicht als abgeleitetes Werk gilt.

### F: Kann ich GPL-Bibliotheken verwenden?

**A**: GPL-Bibliotheken sind nur bei dynamic linking akzeptabel und erzeugen eine Warnung. Static linking oder Einbettung von GPL-Code ist nicht erlaubt, da dies ThemisDB als abgeleitetes Werk unter GPL stellen würde.

### F: Wie beantrage ich eine Ausnahme?

**A**: Erstellen Sie ein Issue mit:
- Titel: "License Exception Request: [Dependency Name]"
- Label: `license-exception`
- Beschreibung: Begründung und rechtliche Analyse
- Warten Sie auf Review durch das Compliance-Team

### F: Wie aktualisiere ich die Lizenz-Policy?

**A**: Bearbeiten Sie `.license-policy.json` und erstellen Sie einen PR. Änderungen an der Policy erfordern eine ausführliche Begründung und Review durch Maintainer.

## Kontakt und Support

Bei Fragen zur Lizenz-Compliance:

- **Issues**: Erstellen Sie ein Issue mit Label `license-compliance`
- **Discussions**: Starten Sie eine Diskussion im "Compliance" Bereich
- **E-Mail**: Kontaktieren Sie das Compliance-Team (siehe SUPPORT.md)

## Weiterführende Ressourcen

- [Lizenz-Policy (.license-policy.json)](../../../.license-policy.json)
- [License Compliance Workflow](../../../.github/workflows/license-compliance.yml)
- [Security Scanning Workflow](../../../.github/workflows/security-scan.yml)
- [SBOM Workflow](../../../.github/workflows/sbom.yml)
- [CONTRIBUTING.md](../../../CONTRIBUTING.md)
- [SECURITY.md](../../../SECURITY.md)

---

**Version**: 1.0.0  
**Letzte Aktualisierung**: 2026-02-02  
**Status**: ✅ Aktiv
