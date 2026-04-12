$ErrorActionPreference = "Stop"
function GetModuleIssueIds {
  $q = @'
query {
  repository(owner:"makr-code", name:"ThemisDB") {
    issue(number:3491) {
      subIssues(first:100) { nodes { number title state } }
    }
  }
}
'@
  $nodes = (gh api graphql -f query="$q" | ConvertFrom-Json).data.repository.issue.subIssues.nodes
  return @($nodes | Where-Object { $_.state -eq 'OPEN' -and $_.title -like '[MODULE]*' } | ForEach-Object { [int]$_.number })
}

$ids = GetModuleIssueIds

function ModuleKey([string]$title){
  $m = ($title -replace '^\[MODULE\]\s*','').Trim()
  if($m -eq 'AQL'){ return 'aql' }
  return $m.ToLowerInvariant()
}

function ScopeLines([string]$key){
  $paths = @()
  if($key -like 'include_*'){
    $paths += "include/" + $key.Substring(8)
  } else {
    if(Test-Path "src/$key"){ $paths += "src/$key" }
    if(Test-Path "include/$key"){ $paths += "include/$key" }
    if($paths.Count -eq 0){ $paths += "src/$key" }
  }
  return ($paths | ForEach-Object { "- $_/" }) -join "`n"
}

$template = @'
## Kontext
Dieses Issue ist ein Teilauftrag von #3491 und kombiniert den ursprünglichen Standardauftrag mit dem erweiterten, agent-tauglichen Arbeitsauftrag.

## Scope
### Primary:
__SCOPE__
- optional: examples/** falls <module>-relevant

### Secondary:
- docs/de/<domain>/<module>/ (+ optional docs/en/**)

## Nicht-Ziele
- Keine Feature-Implementierung / großen Refactors (außer minimal nötige Fixes für Doku/Links/Lint/Build)
- Keine große Text-Neuschreibung ohne Bezug zum Ist-Code

## Tasks
### 1) Primary Inventory
Welche README*, ARCHITECTURE*, ROADMAP*, FUTURE_ENHANCEMENTS* usw. existieren im Modul?

Output:
- Liste im Issue (oder optional docs/de/<module>/inventory.md)

### 2) Primary: Abgleich & Standard (MUSS zuerst)
#### 2.1 Reality-Check (Doku ↔ Sourcecode)
Als erstes die Entwickler-Dokumentation in src/<module> und/oder include/<module> gegen den aktuellen Sourcecode aktualisieren (Synchronisierung/Reality-Check).

Mindestprüfung:
- Entry-Points/APIs/Headers stimmen
- Features sind "wired up" vs. nur Stub/Scaffold (klar markieren)
- Config-Keys/Flags/Build-Optionen stimmen
- Test-/Benchmark-/CI-Claims stimmen (falls behauptet)

Zusatz (Research / wissenschaftlicher Stand):
Falls FUTURE_ENHANCEMENTS* (oder vergleichbare Datei) existiert:
- Suche nach aktuellen wissenschaftlichen Papern (Stand: heute) zu den im Modul genannten geplanten/fehlenden Features
- Ergänze/aktualisiere das Quellenverzeichnis in FUTURE_ENHANCEMENTS* als IEEE-Referenzen (nummeriert, konsistentes Format)
- Jede "Future"-Behauptung, die auf Forschung basiert, soll mindestens 1–3 passende Quellen haben

#### 2.2 Besonderheiten (acceleration) — Pflicht
- ROADMAP vorhanden und verpflichtend zu verifizieren:
  - Primary: src/<module>/ROADMAP.md
  - URL: https://github.com/makr-code/ThemisDB/blob/develop/src/<module>/ROADMAP.md
- Regel: ROADMAP [x] nur, wenn Merge/Commit/Code/Test-Evidence vorhanden ist; sonst Status auf [~]/[?] + Begründung
- ROADMAP-Referenzen (Issues/PRs/Commits) prüfen und Links/Evidence ergänzen, wo möglich
- Output im Issue: kurzer Abschnitt Besonderheiten: ... (erledigt / n/a)

Hinweis für dieses Modul:
- __SPECIAL__

#### 2.3 Primary Standard
Nach dem Reality-Check: Primary-Standard sicherstellen (Header/Links/Status):
- Status (current|drifting|stale) + validated: YYYY-MM-DD oder Commit
- Links: Primary ↔ Secondary ↔ Root-Doku
- konsistente Terminologie + klare Stabilität/Produktionsreife-Hinweise

### 3) Secondary Ableitung (erst nach 2)
Kurze erklärende Docs in docs/de/<domain>/<module> (optional docs/en/**):
- Links auf Primary (Quelle der Details)
- Überblick + Einstieg
- Beispiele/HowTo nur wenn vorhanden und verifiziert
- Navigierbarkeit (Index/Backlinks)

### 4) Report "fehlende Implementierungen" (kein Auto-Issue)
Wenn beim Reality-Check (inkl. ROADMAP-Verifikation) fehlende oder nur teilweise umgesetzte Punkte auffallen:

Erzeuge/aktualisiere:
- docs/de/<module>/missing-implementations.md
- docs/de/<module>/missing-implementations.json

Jeder Eintrag enthält mind.:
- Claim-Quelle (Datei/Abschnitt, ggf. Zeile)
- Erwartet vs. Beobachtet
- Evidence (geprüfte Pfade/Symbole/Tests, ggf. PR/Commit Links)
- Issue-Titelvorschlag + Label-Vorschläge (nur als Vorschlag)

## DoD
- [ ] Primary Docs sind gegen Sourcecode geprüft und aktuell
- [ ] ROADMAP (src/<module>/ROADMAP.md) ist gegen Code/PR-Evidence verifiziert und konsistent (oder bewusst als drift/unclear markiert)
- [ ] Primary Standard (Header/Links/Status) ist erfüllt
- [ ] Secondary Docs existieren, sind navigierbar und verlinken korrekt auf Primary
- [ ] Missing-Implementations Report (md + json) ist erstellt/aktualisiert (falls Findings)
- [ ] Link-check & lint sind grün

## Abschlussformat
1. Geprüfte Dateien
2. Geänderte Aussagen (vorher -> nachher)
3. Verifizierte Pfade/Tests/Evidence
4. Offene Punkte/Risiken
'@

foreach($id in $ids){
  $issue = gh issue view $id --json title | ConvertFrom-Json
  $k = ModuleKey $issue.title
  $scope = ScopeLines $k
  $special = if($k -eq 'acceleration'){ 'ROADMAP-Verifikation ist Pflicht (erledigt / n/a im Abschluss explizit angeben).' } else { 'n/a' }

  $body = $template.Replace('__SCOPE__', $scope).Replace('__SPECIAL__', $special)
  $tmp = New-TemporaryFile
  Set-Content -Path $tmp -Value $body -Encoding UTF8
  gh issue edit $id --body-file $tmp | Out-Null
  Remove-Item $tmp -Force
  Write-Output "updated #$id"
}
