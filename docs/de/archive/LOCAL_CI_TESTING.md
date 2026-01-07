# Lokales CI-Testing - Zusammenfassung

## Lokales CI-Testing - Zusammenfassung

### Problem
- GitHub Actions Workflows schlagen regelmäßig fehl
- Kosten entstehen durch fehlerhafte Builds
- Lokales Testen mit `act` hat Einschränkungen (vcpkg-Timeouts, große Repository-Checkouts)

### Lösungen

#### 1. **Workflow-Optimierungen (Empfohlen)**
✅ Umgesetzt in `ci-fast.yml`:
- `timeout-minutes: 30` (statt 60) - Fail-fast bei Hängern
- `if: "!contains(github.event.head_commit.message, '[skip ci]')"` - Skip bei [skip ci] in Commit
- `concurrency` mit `cancel-in-progress: true` - Abbr uch alter Runs

❌ TODO (erfordert weitere Anpassung):
- vcpkg Caching mit `actions/cache@v4`
- Kürzere vcpkg-Install-Timeouts
- Path-Filter für Dokumentationsänderungen

#### 2. **Lokaler Test-Workflow erstellt**
📄 `.github/workflows/ci-local-test.yml`
- Entfernt vcpkg (nur System-Libraries)
- Schneller für grundlegende Compile-Tests
- Funktioniert mit `act`, aber langsam bei großen Repos

Verwendung:
```powershell
act -W .github/workflows/ci-local-test.yml
```

#### 3. **PowerShell Build-Test-Script**
📄 `scripts/test-build-local.ps1`
- Direkter Build-Test ohne GitHub Actions
- Schnell für lokale Iterationen
- Encoding-Probleme müssen noch behoben werden

### Empfohlener Workflow

1. **Lokale Entwicklung:**
   ```powershell
   # Schneller Build-Test
   cmake -S . -B build-quick -G Ninja
   cmake --build build-quick
   ```

2. **Vor dem Push:**
   - Code-Review durchführen
   - Lokalen Build testen (wenn möglich)
   - Commit-Message prüfen (ggf. `[skip ci]` nutzen für Docs)

3. **GitHub Push:**
   - Optimierter `ci-fast.yml` läuft (30min Timeout, Cancel-in-Progress)
   - Bei Fehlern: Logs prüfen, lokal fixen, erneut pushen

4. **Kosten sparen:**
   - `[skip ci]` in Commit-Message für Dokumentationsänderungen
   - Feature-Branches nutzen (nicht direkt auf main)
   - Draft PRs für Work-in-Progress

### act Einschränkungen

**Was funktioniert:**
- Docker-Image pull
- System-Dependencies installieren
- Basis-Workflow-Syntax validieren

**Was nicht funktioniert:**
- Große Repository-Checkouts (Timeout bei docker cp)
- vcpkg Downloads (Netzwerk-Timeouts)
- Komplexe Multi-Job-Workflows

**Fazit:** `act` ist nützlich für Syntax-Tests, aber nicht für vollständige CI-Simulation.

### Weitere Optimierungsmöglichkeiten

1. **GitHub Self-Hosted Runner** (langfristig):
   - Eigener Build-Server
   - Keine Kosten pro Minute
   - Volle Kontrolle über Cache/Dependencies

2. **Matrix-Builds reduzieren:**
   - Nur Ubuntu (kein Windows/macOS) für schnelles Feedback
   - Vollständige Matrix nur vor Release

3. **Dependency Pre-Building:**
   - Docker-Image mit vorinstallierten Dependencies
   - Reduziert Build-Zeit von 30min auf <5min

### Nächste Schritte

1. ✅ Lokalen Test-Workflow erstellt (`ci-local-test.yml`)
2. ✅ Timeout auf 30min reduziert
3. ✅ [skip ci] Support aktiviert
4. ❌ vcpkg Caching implementieren
5. ❌ Path-Filter für Docs hinzufügen
6. ❌ PowerShell-Script Encoding fixen

Möchten Sie, dass ich die noch offenen Optimierungen umsetze?
