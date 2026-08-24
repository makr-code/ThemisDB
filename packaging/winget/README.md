# WinGet Packaging

Dieser Ordner enthält die WinGet-Manifeste für veröffentlichte Community-Releases von ThemisDB.

## Ziel

Die Manifeste in `packaging/winget/manifests/` sind für eine Einreichung in `microsoft/winget-pkgs` vorbereitet. Sie werden aus echten Release-Artefakten und deren SHA256 erzeugt, damit Version, Download-URL und Hash konsistent bleiben. Unterstützt werden portable ZIP-Pakete und MSI-Artefakte.

## Generierung

Ein bestehendes Release-ZIP kann direkt in Manifeste umgesetzt werden:

```powershell
pwsh ./scripts/release/new-winget-manifest.ps1 `
	-Version 1.0.0 `
	-InstallerUrl https://github.com/makr-code/ThemisDB/releases/download/v1.0.0/ThemisDB-COMMUNITY-1.0.0-windows-x64.zip `
	-InstallerSha256 0123456789ABCDEF0123456789ABCDEF0123456789ABCDEF0123456789ABCDEF
```

Ein MSI-Release oder MSI-Release-Candidate wird analog erzeugt:

```powershell
pwsh ./scripts/release/new-winget-manifest.ps1 `
	-Version 1.9.0-rc1 `
	-InstallerType msi `
	-InstallerUrl https://github.com/makr-code/ThemisDB/releases/download/v1.9.0-rc1/ThemisDB-COMMUNITY-1.9.0-rc1-windows-x64.msi `
	-InstallerSha256 0123456789ABCDEF0123456789ABCDEF0123456789ABCDEF0123456789ABCDEF
```

Der lokale Release-Flow kann die Manifeste direkt nach dem ZIP-Build erzeugen:

```powershell
pwsh ./scripts/release/publish-local-release.ps1 `
	-Tag v1.0.0 `
	-GenerateWingetManifest `
	-IncludeGermanWingetLocale
```

Portable ZIP-Releases deklarieren im Winget-Manifest eine VC++-Runtime-Abhängigkeit, damit die Installationsvalidierung auf einer sauberen Maschine die benötigten MSVC-Laufzeit-DLLs mitnachzieht. Der lokale Release-Flow uebergibt diese Abhaengigkeit automatisch.

Fuer MSI-Artefakte im lokalen Release-Flow:

```powershell
pwsh ./scripts/release/publish-local-release.ps1 `
	-Tag v1.9.0-rc1 `
	-AllowPreRelease `
	-GenerateWingetManifest `
	-WingetInstallerType msi `
	-IncludeGermanWingetLocale
```

Die Ausgabe landet unter `packaging/winget/manifests/t/ThemisDB/ThemisDB/<version>/`.

> Hinweis: Pre-Release-Versionen werden durch die Paketversion selbst codiert (z. B. `2.4.0-alpha`). Das `IsPreRelease`-Feld ist im aktuellen WinGet-Schema ungültig und wird nicht verwendet.

## Inhalt

- `ThemisDB.ThemisDB.yaml`: Versionsmanifest
- `ThemisDB.ThemisDB.installer.yaml`: Installer-Metadaten für ZIP oder MSI
- `ThemisDB.ThemisDB.locale.en-US.yaml`: Publisher-, Lizenz- und Release-Notizen
- `ThemisDB.ThemisDB.locale.de-DE.yaml`: optionale deutsche Lokalisierung

## Einreichung in winget-pkgs

1. GitHub-Release mit dem referenzierten ZIP veröffentlichen.
2. Manifeste lokal erzeugen oder aktualisieren.
3. Optional lokal validieren, z. B. mit `winget validate` oder `wingetcreate validate`.
4. Die drei Manifestdateien in einen Fork von `microsoft/winget-pkgs` unter `manifests/t/ThemisDB/ThemisDB/<version>/` übernehmen.
5. Pull Request gegen `microsoft/winget-pkgs` öffnen.

## Hinweise

- Bei ZIP-Artefakten referenziert die WinGet-Definition `bin\themis_server.exe` als portables Nested-Executable.
- Bei ZIP-Artefakten wird eine Runtime-Abhaengigkeit fuer die VC++-Laufzeit mit eingetragen, damit `themis_server.exe` auf einer frischen Windows-VM startet.
- Bei MSI-Artefakten wird ein natives MSI-Installer-Manifest ohne Nested-Portable-Eintrag erzeugt.
- `LicenseUrl` und `ReleaseNotesUrl` werden auf den jeweiligen GitHub-Release-Tag gesetzt statt auf einen Branch-Namen.
- Hash-Platzhalter dürfen nicht committed oder eingereicht werden; die Manifeste müssen immer aus einem konkreten Artefakt erzeugt werden.
