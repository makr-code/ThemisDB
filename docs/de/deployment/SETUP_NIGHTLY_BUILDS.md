# Setup Guide: Nightly Builds mit DockerHub Push

## Voraussetzungen

Um die nächtlichen Builds mit DockerHub Push zu aktivieren, müssen die folgenden Secrets in GitHub konfiguriert werden.

## Schritt-für-Schritt Anleitung

### 1. DockerHub Access Token erstellen

1. Bei DockerHub einloggen: https://hub.docker.com
2. Auf das Benutzerprofil oben rechts klicken
3. "Account Settings" auswählen
4. Zu "Security" navigieren
5. "New Access Token" klicken
6. Token-Details eingeben:
   - **Description**: "ThemisDB Nightly Builds"
   - **Access permissions**: "Read, Write, Delete" wählen
7. "Generate" klicken
8. Token **sofort kopieren** (wird nur einmal angezeigt!)

### 2. GitHub Repository Secrets konfigurieren

1. Zum ThemisDB Repository gehen: https://github.com/makr-code/ThemisDB
2. "Settings" Tab öffnen
3. Links im Menü "Secrets and variables" → "Actions" auswählen
4. "New repository secret" klicken

#### Secret 1: DOCKER_USERNAME

- **Name**: `DOCKER_USERNAME`
- **Value**: Ihr DockerHub Benutzername (z.B. `themisdb`)
- "Add secret" klicken

#### Secret 2: DOCKER_TOKEN

- **Name**: `DOCKER_TOKEN`
- **Value**: Das in Schritt 1 generierte Access Token einfügen
- "Add secret" klicken

### 3. Workflow-Status überprüfen

1. Zum "Actions" Tab gehen
2. "Nightly Build & DockerHub Push" Workflow auswählen
3. Der Workflow wird automatisch um 2:00 Uhr UTC ausgeführt

### 4. Manuellen Test durchführen (Optional)

Vor dem ersten automatischen Lauf empfohlen:

1. Im "Actions" Tab zu "Nightly Build & DockerHub Push" gehen
2. "Run workflow" Button klicken (rechts oben)
3. Optionen auswählen:
   - **Push to DockerHub**: ✅ aktivieren
   - **Build platforms**: `linux/amd64` wählen (schneller)
4. "Run workflow" klicken
5. Workflow-Ausführung beobachten (~30-60 Minuten)
6. Bei Erfolg: Image auf DockerHub prüfen

## Überprüfung

### DockerHub Images prüfen

Nach erfolgreichem Build:

```bash
# Image herunterladen
docker pull themisdb/server:nightly

# Image Info anzeigen
docker images themisdb/server:nightly

# Container starten (Test)
docker run -d --name test-nightly -p 18765:18765 themisdb/server:nightly

# Logs prüfen
docker logs test-nightly

# Aufräumen
docker stop test-nightly && docker rm test-nightly
```

### DockerHub Web Interface

1. Zu https://hub.docker.com/r/themisdb/server gehen
2. "Tags" Tab öffnen
3. Folgende Tags sollten sichtbar sein:
   - `nightly`
   - `nightly-YYYYMMDD` (aktuelles Datum)
   - `1.3.0-nightly` (aktuelle Version)

## Zeitplan

Der Nightly Build läuft automatisch:
- **Zeitpunkt**: 2:00 AM UTC (täglich)
- **Dauer**: ~30-60 Minuten
- **Zeitzone-Umrechnung**:
  - CET (Berlin): 3:00 AM
  - CEST (Sommerzeit): 4:00 AM
  - EST (New York): 9:00 PM (Vorabend)
  - PST (Los Angeles): 6:00 PM (Vorabend)

## Troubleshooting

### Problem: "Error: Cannot find DOCKER_USERNAME secret"

**Lösung**: Secret wurde nicht korrekt hinzugefügt
1. Repository Settings → Secrets and variables → Actions überprüfen
2. Sicherstellen, dass beide Secrets (`DOCKER_USERNAME` und `DOCKER_TOKEN`) existieren
3. Secret-Namen müssen **exakt** übereinstimmen (Groß-/Kleinschreibung beachten)

### Problem: "Error: Cannot push to DockerHub"

**Mögliche Ursachen**:
1. **Falsches Access Token**: Neues Token in DockerHub generieren
2. **Token-Berechtigungen**: Token muss "Read, Write, Delete" Rechte haben
3. **Falscher Benutzername**: `DOCKER_USERNAME` überprüfen
4. **DockerHub Repository existiert nicht**: Repository `themisdb/server` in DockerHub erstellen

### Problem: "Build failed - Out of disk space"

**Lösung**: Automatische Bereinigung sollte ausreichen, aber:
1. Workflow erneut ausführen
2. Cache in GitHub Actions leeren (Settings → Actions → Caches)
3. Eventuell auf Self-Hosted Runner wechseln

### Problem: "vcpkg dependency download failed"

**Lösung**:
1. Workflow erneut ausführen (temporäre Netzwerkprobleme)
2. vcpkg-Cache leeren und neu bauen
3. vcpkg Baseline Version in `vcpkg-configuration.json` prüfen

## Workflow deaktivieren

Falls die nächtlichen Builds temporär deaktiviert werden sollen:

**Option 1: Workflow deaktivieren**
1. Actions Tab → "Nightly Build & DockerHub Push"
2. "..." Menü (rechts oben) → "Disable workflow"

**Option 2: Cron-Schedule auskommentieren**
```yaml
# In .github/workflows/nightly-build.yml
on:
  # schedule:
  #   - cron: '0 2 * * *'
  workflow_dispatch:
    # ...
```

## Best Practices

### Sicherheit

✅ **DO**:
- Access Token mit minimalen erforderlichen Rechten erstellen
- Tokens regelmäßig rotieren (alle 3-6 Monate)
- Secrets niemals in Code oder Logs speichern

❌ **DON'T**:
- Niemals DockerHub Passwort als Secret verwenden (immer Access Token)
- Tokens nicht in öffentlichen Repositories oder Issues teilen
- Keine "Full Access" Tokens verwenden

### Monitoring

- GitHub Actions Email-Benachrichtigungen aktivieren (Settings → Notifications)
- DockerHub Webhook für Build-Notifications einrichten (optional)
- Regelmäßig Workflow-Logs überprüfen (wöchentlich)

## Weiterführende Dokumentation

- [Nightly Builds Übersicht (Deutsch)](deployment_nightly_builds_de.md)
- [Nightly Builds Documentation (English)](deployment_nightly_builds.md)
- [GitHub Actions Workflows README](../README.md)
- [DockerHub Access Tokens Guide](https://docs.docker.com/docker-hub/access-tokens/)

## Support

Bei Fragen oder Problemen:
- GitHub Issue erstellen: https://github.com/makr-code/ThemisDB/issues
- Workflow-Logs mit Issue verknüpfen
- DockerHub Account-Status überprüfen
