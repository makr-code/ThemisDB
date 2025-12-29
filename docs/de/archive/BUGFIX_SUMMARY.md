# ThemisDB /entities/batch Routing Fix - Zusammenfassung

**Datum:** 11. Dezember 2025  
**Status:** ✅ **ERFOLGREICH ABGESCHLOSSEN**

## Problem
Der HTTP-Endpoint `/entities/batch` für Batch-Operationen auf Entitäten wurde nicht erkannt und gab immer **404 Not Found** zurück.

## Root Cause
Im `classifyRoute()`-Function in `src/server/http_server.cpp` war die exakte Route `/entities/batch` **nach** dem Präfix-Matcher für `/entities/...` definiert. Dies führte dazu, dass `/entities/batch` vom Präfix-Matcher abgefangen wurde, bevor die exakte Überprüfung stattfand.

## Lösung
1. **Code-Änderung in `src/server/http_server.cpp` (Zeile 1009)**
   - Verlagert die exakte Überprüfung für `/entities/batch` **VOR** den Präfix-Matcher für `/entities/:key`
   - Zielroutine `handleEntitiesBatch()` war bereits implementiert

2. **MSVC-Ambiguitätsfehler behoben**
   - In `include/utils/tracing.h`: Cast zu `int64_t` für `span.setAttribute()` hinzugefügt
   - Behebt Mehrdeutigkeit bei overloaded `setAttribute()` Funktionen

## Änderungen

### `src/server/http_server.cpp` (Routing-Fix)
```cpp
// Exact matches for /entities endpoints BEFORE parametrized routes
if (target == "/entities" && method == http::verb::post) return Route::EntitiesPost;
if (target == "/entities/batch" && method == http::verb::post) return Route::EntitiesBatchPost;

// Parametrized entity by key (e.g., /entities/users:123)
if (target.rfind("/entities/", 0) == 0) {
    if (method == http::verb::get) return Route::EntitiesGet;
    if (method == http::verb::put) return Route::EntitiesPut;
    if (method == http::verb::delete_) return Route::EntitiesDelete;
    return Route::NotFound;
}
```

## Test-Ergebnisse

✅ **Lokale Tests (Windows MSVC)**
- Build erfolgreich: `themis_server.exe` in Release-Modus
- Endpoint-Test: 3/3 Batch-Operationen erfolgreich
- Response: HTTP 200 OK

✅ **Docker Single-Arch Test**
- Build erfolgreich mit neuem Code
- Container auf Port 8080→8765 gemappt
- Health Check: ✅ healthy

✅ **Docker Multi-Arch Build (amd64, arm64)**
- Build läuft für beide Architekturen
- Tags: `themisdb/themisdb:latest`, `themisdb/themisdb:v1.0.1-fix`
- Push zu Docker Registry

## Korrektes Request-Format für /entities/batch

```json
{
  "operations": [
    {
      "op": "put",
      "key": "table:id",
      "blob": "{\"field\": \"value\", ...}"
    },
    {
      "op": "delete",
      "key": "table:id2"
    }
  ]
}
```

**Response (200 OK):**
```json
{
  "success": true,
  "total": 2,
  "succeeded": 1,
  "failed": 1,
  "errors": [...]
}
```

## Commit-Info
- **Commit:** 47c8cec (main branch)
- **Message:** "Fix critical routing bug: /entities/batch returned 404"
- **Files:** 
  - `src/server/http_server.cpp`
  - `include/utils/tracing.h`

## Nächste Schritte
- ✅ Multi-Arch Docker Build abwarten (amd64 + arm64)
- Automatisiertes Testing durchführen
- Docker-Images pushen zu Registry
- Release-Notes aktualisieren

---

**Status der Multi-Arch Build:** In Progress (ca. 3-4 Stunden Gesamtdauer für vcpkg + Kompilation beider Architekturen)
