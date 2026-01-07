# WordPress Plugin Beispiel-Screenshots

## Admin-Panel

Das Admin-Panel zeigt:
- GitHub Repository-Konfiguration
- API-Token-Einstellung (optional)
- Cache-Verwaltung
- Shortcode-Verwendungsbeispiele
- Aktuelle Release-Informationen

**Zugriff:** WordPress Admin → Einstellungen → ThemisDB Downloads

## Frontend-Anzeige (Standard-Stil)

```
┌─────────────────────────────────────────────────────────────┐
│  ThemisDB v1.4.0-alpha                          [v1.4.0-alpha]│
│  Veröffentlicht: 7. Januar 2026                                │
├─────────────────────────────────────────────────────────────┤
│  Release Notes:                                                │
│  • Neue Features für v1.4.0                                   │
│  • Performance-Verbesserungen                                 │
│                                                                │
├─────────────────────────────────────────────────────────────┤
│  Downloads:                                                    │
│                                                                │
│  🪟  themisdb-1.4.0-windows-x64.zip                           │
│      25 MB  ↓ 1,234                                           │
│      SHA256: a1b2c3d4e5f6g7h8i9j0k1l2m3n4o5p6...             │
│      [📋 Kopieren]                                            │
│                                                                │
│  🐧  themisdb-1.4.0-linux-x64.tar.gz                          │
│      28 MB  ↓ 2,345                                           │
│      SHA256: b2c3d4e5f6g7h8i9j0k1l2m3n4o5p6q7...             │
│      [📋 Kopieren]                                            │
│                                                                │
│  🐳  Docker: themisdb/themisdb:1.4.0                          │
│      Docker Hub Link                                           │
│                                                                │
│  ▼ Alle SHA256 Checksums anzeigen                            │
└─────────────────────────────────────────────────────────────┘
```

## Kompakte Ansicht

```
ThemisDB v1.4.0-alpha (7. Januar 2026)
  • themisdb-1.4.0-windows-x64.zip
  • themisdb-1.4.0-linux-x64.tar.gz
  • themisdb-1.4.0-arm64.tar.gz

ThemisDB v1.3.4 (15. Dezember 2025)
  • themisdb-1.3.4-windows-x64.zip
  • themisdb-1.3.4-linux-x64.tar.gz
```

## Tabellen-Ansicht

```
┌─────────┬────────────┬─────────────────────────┬────────┬──────────┬──────────┐
│ Version │ Datum      │ Datei                   │ Größe  │ SHA256   │ Download │
├─────────┼────────────┼─────────────────────────┼────────┼──────────┼──────────┤
│ 1.4.0   │ 2026-01-07 │ themisdb-...win-x64.zip │ 25 MB  │ a1b2c... │ [Button] │
│ 1.4.0   │ 2026-01-07 │ themisdb-...linux.tgz   │ 28 MB  │ b2c3d... │ [Button] │
│ 1.3.4   │ 2025-12-15 │ themisdb-...win-x64.zip │ 23 MB  │ c3d4e... │ [Button] │
└─────────┴────────────┴─────────────────────────┴────────┴──────────┴──────────┘
```

## Verifizierungs-Tool

```
┌─────────────────────────────────────────────────────────────┐
│  Download-Verifizierung                                        │
│                                                                │
│  Überprüfen Sie die Integrität Ihrer heruntergeladenen Datei:│
│                                                                │
│  Datei auswählen:                                             │
│  [Datei wählen...] Keine Datei ausgewählt                     │
│                                                                │
│  Erwarteter SHA256-Hash:                                      │
│  [a1b2c3d4e5f6g7h8i9j0k1l2m3n4o5p6q7r8s9t0u1v2w3x4y5z6...]   │
│                                                                │
│  [Verifizieren]                                               │
│                                                                │
│  ✅ Verifizierung erfolgreich!                                │
│     Die Datei ist authentisch und wurde nicht manipuliert.    │
│     SHA256: a1b2c3d4e5f6g7h8...                              │
│                                                                │
├─────────────────────────────────────────────────────────────┤
│  Manuelle Verifizierung (Kommandozeile):                      │
│                                                                │
│  Windows (PowerShell):                                         │
│  > Get-FileHash -Algorithm SHA256 themis-*.zip | Format-List │
│                                                                │
│  Linux/macOS:                                                 │
│  $ sha256sum themis-*.tar.gz                                  │
└─────────────────────────────────────────────────────────────┘
```

## Plattform-Filter Beispiele

**Nur Windows:**
```
[themisdb_downloads platform="windows"]
```

Zeigt nur:
- themisdb-1.4.0-windows-x64.zip
- themisdb-1.3.4-windows-x64.zip

**Nur Linux:**
```
[themisdb_downloads platform="linux"]
```

Zeigt nur:
- themisdb-1.4.0-linux-x64.tar.gz
- themisdb-1.4.0-arm64.tar.gz
- themisdb-1.3.4-linux-x64.tar.gz

## Responsive Design

### Desktop (> 768px)
- Download-Items in Grid-Layout (2-3 Spalten)
- Vollständige SHA256-Hashes sichtbar
- Alle Metadaten angezeigt

### Tablet (768px - 1024px)
- Download-Items in 2 Spalten
- SHA256-Hashes scrollbar
- Kompakte Metadaten

### Mobile (< 768px)
- Download-Items in 1 Spalte (vertikal gestapelt)
- Icons größer für bessere Touch-Bedienung
- SHA256-Hashes in Details-Element versteckt
- Copy-Buttons prominenter

## Farbschema

Das Plugin passt sich automatisch an das WordPress-Theme an:

- **Primärfarbe:** #0073aa (WordPress-Blau)
- **Erfolg:** #46b450 (Grün)
- **Fehler:** #dc3545 (Rot)
- **Warnung:** #ffc107 (Gelb)
- **Hintergrund:** #f7f7f7 (Hellgrau)
- **Text:** #23282d (WordPress-Dunkelgrau)

## Barrierefreiheit

- ✅ WCAG 2.1 Level AA konform
- ✅ Keyboard-Navigation unterstützt
- ✅ Screen-Reader freundlich
- ✅ Ausreichende Farbkontraste
- ✅ Focus-Indikatoren
- ✅ Alt-Texte für alle Icons
