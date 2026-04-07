# ThemisDB Modularisierung - Entscheidung und Zeitplan

**Stand:** 6. April 2026  
**Version:** v1.3.0  
**Kategorie:** 🧩 Architecture

---

## 📑 Inhaltsverzeichnis

- [Zusammenfassung](#zusammenfassung)
- [Hintergrund](#hintergrund)
- [Vorteile](#vorteile)
- [Herausforderungen](#herausforderungen)
- [Zeitplan](#zeitplan)
- [Aktueller Status](#aktueller-status-v120)

## Zusammenfassung

Die Modularisierung von `themis_core` in separate Bibliotheken ist eine wichtige architektonische Verbesserung, die **erst nach der v1.3.0 Release** implementiert wird.

## Hintergrund

### Das Problem
- **Aktueller Stand**: `themis_core` hat über 69.000 exportierte Symbole
- **Windows COFF-Limit**: Maximum 65.535 Symbole pro Objektdatei
- **Aktueller Workaround**: `THEMIS_CORE_SHARED=ON` (DLL-Build)

### Die Lösung (Post-v1.3.0)
Aufteilung von `themis_core` in 11 thematisch organisierte Bibliotheken:

| Modul | Symbole | Beschreibung |
|-------|---------|--------------|
| themis_base | ~3.000 | Gemeinsame Typen, Interfaces, Status-Codes |
| themis_storage | ~15.000 | RocksDB Wrapper, Indizes |
| themis_query | ~12.000 | AQL Parser, Query Engine |
| themis_security | ~8.000 | Verschlüsselung, PKI, RBAC, JWT |
| themis_sharding | ~10.000 | Shard Router, Raft, Gossip Protocol |
| themis_llm | ~6.000 | Model Inference, LoRA, KV Cache |
| themis_content | ~5.000 | Content Management, MIME |
| themis_timeseries | ~4.000 | TSStore, Gorilla Compression |
| themis_network | ~5.000 | HTTP/Wire Protocol Server |
| themis_geo | ~2.000 | Geospatial Index |
| themis_graph | ~2.000 | Graph Analytics |

**Gesamt**: ~72.000 Symbole → Jedes Modul unter 65.535 ✓

## Vorteile

✅ **Technisch**:
- Löst COFF-Limit Problem auf Windows
- Schnellere inkrementelle Builds (nur geänderte Module neu kompilieren)
- Bessere Code-Organisation
- Optionale Features (LLM/Geo können deaktiviert werden)

✅ **Architektonisch**:
- Klare Trennung der Verantwortlichkeiten
- Einfacheres Testen einzelner Module
- Bessere Wartbarkeit
- Parallele Kompilierung möglich

## Herausforderungen

⚠️ **Bekannte Herausforderungen**:
1. **Zirkuläre Abhängigkeiten**: Query ↔ Storage ↔ Security
   - Lösung: Dependency Inversion Principle, Interfaces in `themis_base`
   
2. **Export-Management**: Jedes Modul braucht eigene Export-Makros
   - Lösung: Zentrale Export-Header-Datei erstellt (siehe `include/themis/base/export.h`)
   
3. **Header-Organisation**: Shared vs. Private Headers trennen
   - Lösung: Klare Verzeichnisstruktur `include/` vs. `src/`
   
4. **CMake-Komplexität**: 11+ Targets statt 1
   - Lösung: Helper-Funktionen und Makros (siehe `cmake/ModularBuild.cmake`)

## Zeitplan

**Voraussetzung**: v1.3.0 muss erst released werden

| Zeitraum | Meilenstein |
|----------|-------------|
| Woche 1 | Foundation (base-Modul, CMake-Struktur) |
| Woche 2 | Core-Module (storage, query, security, network) |
| Woche 3 | Feature-Module (timeseries, geo, graph, content, llm) |
| Woche 4 | Distributed System (sharding), Testing, Validierung |
| Woche 5 | Puffer für Probleme, Dokumentation, Review |

**Geschätzter Aufwand**: 2-4 Wochen

## Aktueller Status (v1.2.0)

✅ **Was wurde vorbereitet**:
- Detaillierter Modularisierungsplan erstellt (`docs/architecture/MODULARIZATION_PLAN.md`)
- CMake-Konfiguration vorbereitet (`cmake/ModularBuild.cmake`)
- Export-Header-Templates erstellt (`include/themis/base/export.h`)
- Version-Check eingebaut (verhindert Aktivierung vor v1.3.0)
- Dokumentation im README aktualisiert

✅ **Sicherheitsmechanismen**:
```cmake
# Automatischer Version-Check
if(THEMIS_BUILD_MODULAR)
    if(PROJECT_VERSION VERSION_LESS "1.3.0")
        message(WARNING "Modular build requires v1.3.0 or later")
        set(THEMIS_BUILD_MODULAR OFF CACHE BOOL "Disabled" FORCE)
    endif()
endif()
```

❌ **Was noch NICHT gemacht wurde** (erst nach v1.3.0):
- Keine Code-Änderungen an bestehenden Dateien
- Keine Aufteilung der Source-Dateien
- Keine Build-System-Änderungen
- Kein Refactoring

## Nächste Schritte

1. ✅ **Jetzt (v1.2.0)**: Planung und Vorbereitung abgeschlossen
2. ⏳ **v1.3.0 Release**: Abwarten (Q1 2026)
3. 🚀 **Nach v1.3.0**: Modularisierung implementieren (v1.4.0)

## Verwendung (Post-v1.3.0)

Nach dem v1.3.0 Release kann die Modularisierung aktiviert werden:

```bash
# Modular build (v1.3.0+)
cmake -DTHEMIS_BUILD_MODULAR=ON \
      -DTHEMIS_MODULE_LLM=ON \
      -DTHEMIS_MODULE_GEO=OFF \
      ..

# Legacy monolithic build (default)
cmake -DTHEMIS_BUILD_MODULAR=OFF ..
```

## Referenzen

- **Hauptdokument**: [MODULARIZATION_PLAN.md](MODULARIZATION_PLAN.md)
- **CMake-Konfiguration**: [cmake/ModularBuild.cmake](../../cmake/ModularBuild.cmake)
- **Export-Header**: [include/themis/base/export.h](../../include/themis/base/export.h)
- **Windows COFF-Format**: https://learn.microsoft.com/en-us/windows/win32/debug/pe-format

## Fazit

Die Modularisierung ist eine sinnvolle und notwendige architektonische Verbesserung, die:
- ✅ Technische Probleme löst (COFF-Limit)
- ✅ Developer Experience verbessert (schnellere Builds)
- ✅ Zukünftiges Wachstum ermöglicht (optionale Module)

**Entscheidung**: Implementation erfolgt **nach v1.3.0 Release** in v1.4.0+

---

**Status**: Planung abgeschlossen, wartet auf v1.3.0 Release  
**Version**: 1.2.0 → 1.3.0 → 1.4.0 (Modularisierung)  
**Geschätzter Zeitaufwand**: 2-4 Wochen nach v1.3.0
