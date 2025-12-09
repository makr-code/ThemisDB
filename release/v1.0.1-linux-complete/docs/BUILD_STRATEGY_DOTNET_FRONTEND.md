# ThemisDB Document Manager - Build Strategy

## 📋 Übersicht

**Projekt:** Themis.DocumentManager (C# WPF, .NET 8.0)
**Typ:** Desktop-Application (Windows)
**Plattformen:** x64, x86 (optional ARM64)
**Framework:** .NET 8.0-windows + WPF

## 🎯 Build-Strategien

### 1. **CLI-basierter Build (dotnet CLI)**

#### Quick Build (Debug)
```powershell
.\build_dotnet_frontend.ps1 -Configuration Debug
```

#### Production Build (Release)
```powershell
.\build_dotnet_frontend.ps1 -Configuration Release -Version 1.0.1
```

#### Mit Packaging
```powershell
.\build_dotnet_frontend.ps1 -Configuration Release -Version 1.0.1 -Pack
```

#### Mit Code Signing
```powershell
# Zertifikat-Pfad muss gesetzt werden
$env:CERT_PASSWORD = "your-password"
.\build_dotnet_frontend.ps1 -Configuration Release -Sign -CertPath "C:\path\to\cert.pfx"
```

### 2. **Visual Studio Solution Build**

#### Voraussetzungen
- Visual Studio 2022 (17.8+)
- .NET 8.0 SDK
- Windows Desktop Development Workload

#### Schritt-für-Schritt in Visual Studio

1. **Solution öffnen:**
   ```
   C:\VCC\themis\projects\Themis.sln
   ```

2. **NuGet Packages wiederherstellen:**
   - Build → Rebuild Solution
   - Oder: Right-click Solution → Restore NuGet Packages

3. **Konfiguration wählen:**
   - Configuration: **Release** (für Produktion)
   - Platform: **x64** (empfohlen)

4. **Build durchführen:**
   - Build → Build Solution (Ctrl+Shift+B)
   - Oder: Build → Publish Themis.DocumentManager

5. **Output überprüfen:**
   ```
   C:\VCC\themis\projects\Themis.DocumentManager\bin\Release\net8.0-windows\
   ```

### 3. **Docker-Containerisierter Build (Windows Container)**

#### Multi-Stage Build für .NET Frontend

```dockerfile
# Stage 1: Build
FROM mcr.microsoft.com/dotnet/sdk:8.0-windowsservercore-ltsc2022 as build
WORKDIR /src

# Copy project files
COPY ["projects/Themis.DocumentManager/", "Themis.DocumentManager/"]
COPY ["projects/Themis.AdminTools.Shared/", "Themis.AdminTools.Shared/"]

# Restore & Build
RUN dotnet restore "Themis.DocumentManager/Themis.DocumentManager.csproj"
RUN dotnet build "Themis.DocumentManager/Themis.DocumentManager.csproj" -c Release -o /app/build

# Publish
RUN dotnet publish "Themis.DocumentManager/Themis.DocumentManager.csproj" \
    -c Release -o /app/publish \
    --self-contained \
    --runtime win-x64

# Stage 2: Runtime
FROM mcr.microsoft.com/windows/servercore:ltsc2022
WORKDIR /app
COPY --from=build /app/publish .

# Windows Desktop Runtime
RUN powershell -Command \
    $ProgressPreference = 'SilentlyContinue'; \
    Invoke-WebRequest -Uri 'https://aka.ms/dotnet/8.0/windowsdesktop-runtime-win-x64.exe' \
    -OutFile 'runtime.exe'; \
    .\runtime.exe /install /quiet /norestart; \
    Remove-Item runtime.exe

ENTRYPOINT ["Themis.DocumentManager.exe"]
```

### 4. **CI/CD Pipeline (GitHub Actions)**

```yaml
name: .NET Frontend Build

on: [push, pull_request]

jobs:
  build:
    runs-on: windows-latest
    
    steps:
    - uses: actions/checkout@v3
    
    - name: Setup .NET
      uses: actions/setup-dotnet@v3
      with:
        dotnet-version: '8.0.x'
    
    - name: Restore
      run: dotnet restore projects/Themis.DocumentManager/
    
    - name: Build
      run: dotnet build projects/Themis.DocumentManager/ -c Release
    
    - name: Publish
      run: |
        dotnet publish projects/Themis.DocumentManager/ \
          -c Release \
          -o publish \
          --self-contained \
          --runtime win-x64
    
    - name: Create Package
      run: |
        Compress-Archive -Path publish -DestinationPath ThemisDB-DocumentManager-1.0.1-x64.zip
      shell: powershell
    
    - name: Upload Artifact
      uses: actions/upload-artifact@v3
      with:
        name: themis-document-manager
        path: ThemisDB-DocumentManager-1.0.1-x64.zip
```

## 🔧 Build-Artefakte

### Debug Build Output
```
projects/Themis.DocumentManager/bin/Debug/net8.0-windows/
├── Themis.DocumentManager.exe
├── Themis.DocumentManager.dll
├── *.deps.json
└── *.runtimeconfig.json
```

### Release Build Output
```
build-dotnet/
├── net8.0-windows/
│   ├── Themis.DocumentManager.exe (signed)
│   ├── dependencies/
│   └── resources/
├── publish/
│   └── (self-contained deployment)
└── ThemisDB-DocumentManager-1.0.1-x64.zip
```

## 📊 Abhängigkeiten

### NuGet Packages
```xml
<PackageReference Include="CommunityToolkit.Mvvm" Version="8.2.2" />
<PackageReference Include="Microsoft.Extensions.DependencyInjection" Version="8.0.0" />
<PackageReference Include="Microsoft.Extensions.Http" Version="8.0.0" />
<PackageReference Include="Microsoft.Extensions.Logging" Version="8.0.0" />
<PackageReference Include="ModernWpfUI" Version="0.9.6" />
<PackageReference Include="Newtonsoft.Json" Version="13.0.3" />
<PackageReference Include="System.Text.Json" Version="8.0.4" />
```

### COM References (Office Integration)
- Microsoft.Office.Interop.Word 8.7
- Microsoft.Office.Interop.Excel 1.9
- Microsoft.Office.Interop.PowerPoint 2.12
- Microsoft.Office.Interop.Outlook 9.6

## ✅ Qualitätssicherung

### Code-Analyse
```powershell
# Style-Check
dotnet format projects/Themis.DocumentManager/ --verify-no-changes

# Security scanning
dotnet run --project security-scanner/ -- projects/Themis.DocumentManager/
```

### Testing
```powershell
dotnet test tests/Themis.DocumentManager.Tests/ -c Release
```

### Packaging-Validierung
```powershell
# ZIP-Integrität prüfen
Test-Path "ThemisDB-DocumentManager-1.0.1-x64.zip"

# Signatur verifizieren (nur wenn signed)
# Erfordert: SignTool.exe (Windows SDK)
```

## 🚀 Deployment-Strategien

### 1. **Direct Installation**
```powershell
# Benutzer führt .exe direkt aus
.\ThemisDB-DocumentManager-1.0.1-x64.exe
```

### 2. **MSI Installer** (optional mit WiX)
```powershell
# Installer-Erstellung
candle.exe Themis.DocumentManager.wxs -o Themis.wixobj
light.exe Themis.wixobj -o ThemisDB-DocumentManager-1.0.1.msi
```

### 3. **MSIX App Package** (Microsoft Store)
```powershell
# Für Windows 11+ distribution
```

### 4. **Self-Contained Distribution**
```powershell
# Zip mit Runtime mitgeliefert
7z a -r "ThemisDB-DocumentManager-1.0.1-portable.zip" publish\*
```

## 📈 Performance-Tipps

- **Trimming:** `<TrimMode>partial</TrimMode>` zur Reduzierung der Größe
- **Compilation:** Verwende `<PublishReadyToRun>true</PublishReadyToRun>` für faster startup
- **Caching:** NuGet-Cache lokal speichern für schnellere Builds
- **Parallel Builds:** `dotnet build -maxcpucount` nutzen

## 🔐 Security Best Practices

1. ✅ Code Signing (AuthentiCode zertifikat)
2. ✅ NuGet-Paket-Signierung prüfen
3. ✅ Abhängigkeiten regelmäßig aktualisieren
4. ✅ SBOM (Software Bill of Materials) erstellen
5. ✅ Vulnerability Scanning (dependabot, snyk)

## 📝 Versionsverwaltung

Versions-Schema: **MAJOR.MINOR.PATCH**
- Update automatisch in csproj durch `build_dotnet_frontend.ps1`
- Git Tag: `v1.0.1`
- Release Notes in `CHANGELOG.md`

## 🎯 Häufige Probleme & Lösungen

| Problem | Lösung |
|---------|--------|
| "NuGet restore failed" | `dotnet nuget locals all --clear` |
| "WPF designer not loading" | VS 2022 neu starten, .NET 8 SDK überprüfen |
| "COM reference errors" | Office installieren oder aus GAC referenzieren |
| "Build timeout" | `dotnet build --verbosity quiet` oder cache clearing |

