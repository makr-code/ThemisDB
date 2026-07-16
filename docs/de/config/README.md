# Config-Modul
<!-- status: current | validated: 2026-04-06 | primary: src/config/ -->

**Modul:** `src/config/`  
**Stand:** 6. April 2026  
**Version:** 0.0.34  
**Status:** ✅ Production-Ready

---

## Übersicht

Das Config-Modul stellt die rückwärtskompatible Konfigurationspfad-Auflösung für ThemisDB bereit.
Es bildet legacy-flache Konfigurationspfade auf die neue hierarchische Verzeichnisstruktur ab und
ermöglicht so ein nahtloses Migrationsfenster, in dem sowohl alte als auch neue Pfade gleichzeitig
gültig sind.

**Kernfunktionen:**
- **Pfadauflösung** — 60+ Legacy-zu-Neu-Pfad-Mappings mit Dateisystem-Fallback
- **LRU-Cache** — aufgelöste Pfade werden gecacht (Kapazität und TTL via Umgebungsvariablen konfigurierbar)
- **Schema-Validierung** — YAML/JSON-Konfigurationsdateien gegen JSON Schema (Draft-7-Subset) validieren
- **Audit-Trail** — opt-in, bounded Ring-Buffer mit allen Pfadzugriffen
- **Prometheus-Metriken** — Auflösungsrate, Cache-Trefferquote, Legacy-Fallbacks je Kategorie
- **Migrations-Scanner** — CLI-Tool zum Auffinden und Ersetzen veralteter Pfade im Deployment

---

## Komponenten

| Datei | Rolle |
|-------|-------|
| `config_path_resolver.h` / `.cpp` | Kern-Logik: Pfadauflösung, LRU-Cache, Metriken, Multi-Env-Overlay |
| `config_schema_validator.h` / `.cpp` | JSON-Schema-Validierung von YAML/JSON-Konfigurationsdateien |
| `config_audit_log.h` / `.cpp` | Bounded In-Memory-Audit-Trail für Konfigurationspfadzugriffe |
| `config_metrics_exporter.h` / `.cpp` | Prometheus-Metriken-Exporter (Text-Format, `/metrics`-Endpunkt) |
| `lru_cache.h` | Generischer LRU-Cache mit TTL-Ablauf |
| `path_mapping_metadata.h` | Deprecation-Datum, Entfernungsdatum und Migrations-URL je Pfad |
| `config_errors.h` | Typisierte Exception-Hierarchie für konfigurationsbezogene Fehler |
| `config_migration_scanner_impl.h` | Testbare Inline-Implementierung des `config_migration_scanner` CLI-Tools |

---

## Schnellstart

### Pfad auflösen

```cpp
#include "config/config_path_resolver.h"
using namespace themis::config;

// Wirft ConfigNotFoundException wenn nicht gefunden
std::string path = ConfigPathResolver::resolve("config/lora_training_config.yaml");
// → "config/ai_ml/lora_training_config.yaml" (neuer Pfad)
// → "config/lora_training_config.yaml" + Deprecation-Warnung (Legacy-Fallback)

// Nicht-werfende Variante
auto opt = ConfigPathResolver::tryResolve("config/pii_patterns.yaml");
if (opt) {
    // *opt enthält den aufgelösten Pfad
}
```

### YAML/JSON-Konfiguration validieren

```cpp
#include "config/config_schema_validator.h"
using namespace themis::config;

nlohmann::json schema = R"({
    "type": "object",
    "required": ["host", "port"],
    "properties": {
        "host": { "type": "string" },
        "port": { "type": "integer", "minimum": 1, "maximum": 65535 }
    }
})"_json;

auto result = ConfigSchemaValidator::validate("config/server.yaml", schema);
if (!result.valid) {
    spdlog::error("Validierung fehlgeschlagen:\n{}", result.formatErrors());
}
```

### Audit-Trail aktivieren

```cpp
ConfigPathResolver::setAuditLogEnabled(true);
std::string path = ConfigPathResolver::resolve("config/pii_patterns.yaml");

for (const auto& entry : ConfigPathResolver::auditLog()) {
    // entry.requested_path, entry.resolved_path, entry.timestamp,
    // entry.is_legacy, entry.is_cache_hit
}
```

---

## Umgebungsvariablen

| Variable | Standard | Gültiger Bereich | Beschreibung |
|----------|----------|-----------------|--------------|
| `THEMIS_CONFIG_CACHE_SIZE` | `1000` | `[10, 100000]` | Maximale Anzahl gecachter Pfadauflösungen |
| `THEMIS_CONFIG_CACHE_TTL` | `300` | `[1, 86400]` | Cache-TTL in Sekunden |
| `THEMIS_CONFIG_ENV` | `prod` | `dev` \| `staging` \| `prod` | Aktive Deployment-Umgebung für Overlay-Auflösung |

---

## Multi-Environment-Overlay

Der Resolver unterstützt umgebungsspezifische Konfigurationsüberschreibungen über ein
Overlay-Verzeichnis (nur für `dev` und `staging`):

| Umgebung | Overlay-Root | Aktivierung |
|----------|-------------|-------------|
| `DEV` | `config/dev/` | `THEMIS_CONFIG_ENV=dev` |
| `STAGING` | `config/staging/` | `THEMIS_CONFIG_ENV=staging` |
| `PROD` | *(kein Overlay)* | Standard |

**Auflösungsreihenfolge (Beispiel: DEV):**
1. `config/dev/ai_ml/lora_training_config.yaml` — Overlay
2. `config/ai_ml/lora_training_config.yaml` — kanonischer neuer Pfad
3. `config/lora_training_config.yaml` — Legacy-Fallback (mit Deprecation-Warnung)

---

## Prometheus-Metriken

| Metrik | Typ | Beschreibung |
|--------|-----|--------------|
| `themis_config_resolution_hits_total` | Counter | Erfolgreiche Pfadauflösungen |
| `themis_config_resolution_misses_total` | Counter | Fehlgeschlagene Auflösungen |
| `themis_config_legacy_fallbacks_total` | Counter | Legacy-Fallbacks gesamt |
| `themis_config_new_path_hits_total` | Counter | Direkte Treffer auf neuen Pfaden |
| `themis_config_cache_hits_total` | Counter | LRU-Cache-Treffer |
| `themis_config_cache_misses_total` | Counter | LRU-Cache-Fehlschläge |
| `themis_config_cache_hit_ratio` | Gauge | Trefferquote 0,0–1,0 |
| `themis_config_legacy_fallbacks_by_category_total{category}` | Counter | Legacy-Fallbacks je Kategorie |

---

## Migrations-Scanner

```bash
# Textbericht (Standard)
config_migration_scanner --root /srv/themis

# JSON-Bericht
config_migration_scanner --root /srv/themis --output json

# Trockenlauf: zeigt, was --fix ändern würde
config_migration_scanner --root /srv/themis --dry-run --fix

# Dateien in-place umschreiben (erstellt .bak-Backups)
config_migration_scanner --root /srv/themis --fix
```

**Exit-Codes:** `0` = keine überfälligen Pfade · `1` = mind. ein Pfad nach `removal_date` · `2` = Argument-Fehler

---

## Exception-Hierarchie

| Exception | Ausgelöst wenn |
|-----------|---------------|
| `ConfigNotFoundException` | Weder neuer noch Legacy-Pfad existiert |
| `MappingNotFoundException` | Kein Mapping für den angegebenen Legacy-Pfad vorhanden |
| `InvalidPathException` | Pfad enthält `..` (Traversal-Angriff) oder Null-Bytes |
| `ConfigPermissionException` | Dateisystem verweigert den Zugriff |
| `SchemaValidationException` | Konfigurations- oder Schema-Datei kann nicht gelesen/geparst werden |

---

## Dokumentation in diesem Ordner

| Datei | Beschreibung |
|-------|--------------|
| [README.md](README.md) | Diese Seite — Einstieg und Schnellübersicht |
| [MISSING_IMPLEMENTATIONS.md](MISSING_IMPLEMENTATIONS.md) | Fehlende Implementierungen: Claim vs. Ist-Stand |

---

## Weiterführende Dokumentation (Primary)

| Dokument | Pfad | Beschreibung |
|----------|------|--------------|
| Modul-README | [`src/config/README.md`](../../../src/config/README.md) | Vollständige Komponentenbeschreibung, Interfaces, Konfiguration, Beispiele |
| Architektur | [`src/config/ARCHITECTURE.md`](../../../src/config/ARCHITECTURE.md) | Komponentendiagramme, Datenflüsse, Threading-Modell |
| Roadmap | [`src/config/ROADMAP.md`](../../../src/config/ROADMAP.md) | Implementierungsstatus und geplante Features |
| Future Enhancements | [`src/config/FUTURE_ENHANCEMENTS.md`](../../../src/config/FUTURE_ENHANCEMENTS.md) | Detaillierte Planung zukünftiger Features (inkl. wissenschaftliche Quellen) |
| Migrations-Leitfaden | [`docs/config_migration_guide.md`](../../migration/config_migration_guide.md) | Operator-Leitfaden: Legacy-Pfade auf neue Hierarchie migrieren |

---

## Verwandte Module

- [Security-Modul](../security/README.md) — Verschlüsselung, TLS, Key Management
- [Observability-Modul](../observability/README.md) — Prometheus, Grafana, Tracing
