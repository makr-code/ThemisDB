# Multi-Level Encrypted User Storage Plugin

## Übersicht

Das Multi-Level Encrypted User Storage Plugin bietet sichere, klassifizierungsbasierte Speicherung für Benutzer- und Gruppendaten mit Verschlüsselung auf Dateisystemebene. Es implementiert das vierstufige Sicherheitsklassifizierungssystem von ThemisDB mit separaten verschlüsselten Containern pro Stufe.

## Funktionen

- **4 Sicherheitsstufen**: offen, vs-nfd, geheim, streng-geheim
- **Dateisystem-Verschlüsselung**: gocryptfs (AES-256-GCM)
- **Schlüsselverwaltung**: HashiCorp Vault und HSM-Integration
- **Automatische Schlüsselrotation**: Zero-Downtime-Rotation mit konfigurierbaren Intervallen
- **Plattformübergreifend**: Linux (primär), macOS über macFUSE, Windows experimentell
- **RBAC/ABAC**: Apache Ranger Integration bereit

## Sicherheitsklassifizierungsstufen

| Stufe | Beschreibung | Verschlüsselung | Schlüsselanbieter | Rotationsintervall |
|-------|--------------|-----------------|-------------------|--------------------|
| **offen** | Öffentliche Daten | Keine | N/A | N/A |
| **vs-nfd** | Verschlusssache - Nur für den Dienstgebrauch | AES-256-GCM | Vault | 90 Tage |
| **geheim** | Geheim | AES-256-GCM | Vault | 60 Tage |
| **streng-geheim** | Streng geheim | AES-256-GCM | HSM | 30 Tage |

## Systemanforderungen

### Minimale Anforderungen
- Linux (Ubuntu 20.04+, Debian 11+, RHEL 8+)
- gocryptfs >= 2.0
- FUSE-Unterstützung
- 500MB+ freier Speicherplatz pro Stufe

### Optionale Anforderungen
- HashiCorp Vault (für vs-nfd, geheim, streng-geheim)
- HSM mit PKCS#11-Unterstützung (für streng-geheim)
- Apache Ranger (für erweitertes RBAC)

## Installation

### 1. gocryptfs installieren

```bash
# Ubuntu/Debian
sudo apt-get install gocryptfs fuse

# RHEL/CentOS
sudo yum install fuse gocryptfs

# macOS
brew install --cask macfuse
brew install gocryptfs
```

### 2. Plugin konfigurieren

Beispielkonfiguration kopieren:

```bash
cp config/storage_config.yaml.example /etc/themisdb/storage_config.yaml
```

`/etc/themisdb/storage_config.yaml` bearbeiten und folgendes festlegen:
- Vault-Adresse und Zugangsdaten
- HSM-Bibliothekspfad und Slot
- Verschlüsselungs-Schlüssel-IDs
- Rotationsintervalle

### 3. Vault-Schlüssel initialisieren

```bash
# Verschlüsselungsschlüssel in Vault erstellen
vault kv put themis/keys/user_storage_vs_nfd \
  key=$(openssl rand -base64 32) \
  algorithm="AES-256-GCM" \
  version=1

vault kv put themis/keys/user_storage_geheim \
  key=$(openssl rand -base64 32) \
  algorithm="AES-256-GCM" \
  version=1
```

### 4. Plugin laden

```cpp
// In Ihrer ThemisDB-Anwendung
auto plugin_manager = std::make_shared<PluginManager>();
plugin_manager->loadPlugin("user_storage_encrypted");

// Plugin-Instanz abrufen
auto storage = plugin_manager->getPlugin<MultiLevelEncryptedStorage>(
    "user_storage_encrypted"
);

// Mit Konfiguration initialisieren
std::ifstream config_file("/etc/themisdb/storage_config.yaml");
std::string config((std::istreambuf_iterator<char>(config_file)),
                   std::istreambuf_iterator<char>());
storage->initialize(config.c_str());
```

## Verwendung

### Benutzer erstellen

```cpp
User user;
user.user_id = "user_001";
user.username = "max.mustermann";
user.email = "max.mustermann@firma.de";
user.full_name = "Max Mustermann";
user.roles = {"admin", "developer"};
user.classification = SecurityLevel::VS_NFD;
user.created_at_ms = getCurrentTimeMs();
user.updated_at_ms = user.created_at_ms;

auto result = storage->createUser(user, SecurityLevel::VS_NFD);
if (result.isSuccess()) {
    std::cout << "Benutzer erfolgreich erstellt" << std::endl;
} else {
    std::cerr << "Fehler: " << result.error() << std::endl;
}
```

### Benutzer abrufen

```cpp
auto result = storage->getUser("user_001", SecurityLevel::VS_NFD);
if (result.isSuccess()) {
    const User& user = result.value();
    std::cout << "Benutzer: " << user.username << std::endl;
    std::cout << "E-Mail: " << user.email << std::endl;
} else {
    std::cerr << "Fehler: " << result.error() << std::endl;
}
```

### Gruppe erstellen

```cpp
Group group;
group.group_id = "group_001";
group.name = "Entwickler";
group.description = "Entwicklungsteam";
group.member_ids = {"user_001", "user_002"};
group.classification = SecurityLevel::VS_NFD;
group.created_at_ms = getCurrentTimeMs();

auto result = storage->createGroup(group, SecurityLevel::VS_NFD);
```

### Gesundheitsprüfung

```cpp
auto health = storage->checkHealth();
if (health.isSuccess() && health.value().healthy) {
    std::cout << "Alle Speicherstufen gesund" << std::endl;
} else {
    std::cerr << "Gesundheitsprüfung fehlgeschlagen: " 
              << health.value().message << std::endl;
    for (const auto& error : health.value().errors) {
        std::cerr << "  - " << error << std::endl;
    }
}
```

## Container-Verwaltung

### Alle Container mounten

```cpp
auto result = storage->mountAll();
if (result.isError()) {
    std::cerr << "Container-Mount fehlgeschlagen: " << result.error() << std::endl;
}
```

### Alle Container unmounten

```cpp
storage->unmountAll();
```

### Manuelle Schlüsselrotation

```cpp
auto result = storage->rotateKey(SecurityLevel::VS_NFD);
if (result.isSuccess()) {
    std::cout << "Schlüssel erfolgreich rotiert" << std::endl;
} else {
    std::cerr << "Rotation fehlgeschlagen: " << result.error() << std::endl;
}
```

## Docker-Unterstützung

### Dockerfile

```dockerfile
FROM ubuntu:22.04

# gocryptfs und FUSE installieren
RUN apt-get update && apt-get install -y \
    gocryptfs \
    fuse \
    libsodium23 \
    && rm -rf /var/lib/apt/lists/*

# FUSE im Container erlauben
RUN echo "user_allow_other" >> /etc/fuse.conf

# ThemisDB kopieren
COPY . /opt/themisdb
WORKDIR /opt/themisdb

# Mit FUSE-Unterstützung ausführen
# docker run --cap-add SYS_ADMIN --device /dev/fuse ...
```

### docker-compose.yml

```yaml
version: '3.8'
services:
  themisdb:
    image: themisdb:latest
    cap_add:
      - SYS_ADMIN
    devices:
      - /dev/fuse
    volumes:
      - ./encrypted:/var/lib/themisdb/encrypted
      - ./config:/etc/themisdb
    environment:
      - VAULT_ADDR=https://vault:8200
      - VAULT_TOKEN_FILE=/run/secrets/vault_token
    secrets:
      - vault_token

secrets:
  vault_token:
    file: ./secrets/vault_token
```

## Sicherheitsüberlegungen

1. **Schlüsselspeicherung**: Niemals Schlüssel im Klartext speichern. Verwenden Sie Vault oder HSM.
2. **Dateisystemberechtigungen**: Verschlüsselte Verzeichnisse verwenden 0700 (nur Besitzer).
3. **Speichersicherheit**: Schlüssel werden niemals in den Swap geschrieben (mlock).
4. **Prozessisolation**: gocryptfs läuft als separater Prozess.
5. **Audit-Protokollierung**: Alle Operationen werden mit Klassifizierungsstufe protokolliert.

## Fehlerbehebung

### Container lässt sich nicht mounten

```bash
# Prüfen, ob gocryptfs installiert ist
which gocryptfs

# FUSE-Verfügbarkeit prüfen
ls -l /dev/fuse

# Prüfen, ob bereits gemountet
mount | grep themisdb

# Manueller Mount-Test
gocryptfs /var/lib/themisdb/encrypted/vs-nfd /var/lib/themisdb/mnt/vs-nfd
```

### Zugriff verweigert

```bash
# Sicherstellen, dass Benutzer in fuse-Gruppe ist
sudo usermod -a -G fuse $USER

# Gruppen neu laden
newgrp fuse
```

### Schlüsselanbieter-Fehler

```bash
# Vault-Verbindung testen
vault status

# Vault-Token prüfen
vault token lookup

# Schlüssel-Existenz prüfen
vault kv get themis/keys/user_storage_vs_nfd
```

## Leistung

| Operation | Latenz | Hinweise |
|-----------|--------|----------|
| Container-Mount | < 500ms | Kaltstart |
| Benutzer lesen | < 5ms | Gecacht |
| Benutzer schreiben | < 10ms | Inklusive Verschlüsselung |
| Schlüsselrotation | < 30min | 10.000 Benutzer |

## Einschränkungen

- Windows-Unterstützung experimentell (WinFsp erforderlich)
- macOS erfordert macFUSE-Installation
- HSM-Operationen langsamer als Vault (10-50ms vs 1-5ms)
- Schlüsselrotation verursacht vorübergehenden Read-Only-Modus

## Zukünftige Erweiterungen

- [ ] Backup/Wiederherstellung pro Stufe
- [ ] Migration von unverschlüsselt zu verschlüsselt
- [ ] Multi-Region-Replikation
- [ ] Container-Gesundheitsüberwachung
- [ ] Automatische Fehlerwiederherstellung

## Lizenz

Siehe ThemisDB-Hauptlizenz.

## Support

Für Probleme und Fragen:
- GitHub Issues: https://github.com/makr-code/ThemisDB/issues
- Dokumentation: https://themisdb.org/docs/plugins/user-storage-encrypted
