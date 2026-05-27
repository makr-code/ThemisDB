## AI Update Blueprint

### Dateien, die konkret aktualisiert werden müssen
- src/acceleration/README.md
- src/acceleration/ROADMAP.md
- src/acceleration/FUTURE_ENHANCEMENTS.md
- src/acceleration/ARCHITECTURE.md
- src/acceleration/AUDIT.md
- src/acceleration/SECURITY.md
- src/acceleration/CHANGELOG.md
- src/acceleration/PERFORMANCE_EXPECTATIONS.md
- src/acceleration/backend_registry.cpp
- src/acceleration/compute_backend.cpp
- src/acceleration/device_manager.cpp
- src/acceleration/plugin_loader.cpp
- src/acceleration/shader_integrity.cpp
- src/acceleration/graphics_backends.cpp
- src/acceleration/geo_acceleration_bridge.cpp
- src/acceleration/ai_hardware_dispatcher.cpp

### Was genau zu ändern ist
- Public API, Backend-Übergänge, Laufzeitverhalten, Sicherheitsgrenzen und Cross-References für das acceleration-Modul präzisieren.
- Die Dokumentation so schreiben, dass die AI den Zusammenhang zwischen Backends, Shadern und Hardware-Dispatch direkt aus dem Ticket ableiten kann.
