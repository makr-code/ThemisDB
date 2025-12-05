# retention_api_handler.cpp

**Stand:** 5. Dezember 2025  
**Version:** 1.0.0  
**Kategorie:** Src

---


Path: `src/server/retention_api_handler.cpp`

Purpose: Handlers for data retention APIs (delete/expire data according to policies).

Public functions / symbols:
- `if (!retention_manager_) {`
- ``
- `if (policy.classification_level != filter.classification_filter) {`
- `if (start < total) {`
- `for (int i = start; i < end; ++i) {`
- `catch (const std::exception& e) {`
- `for (const auto& action : actions) {`
- `localtime_s(&tm, &timestamp_t);`
- `localtime_r(&timestamp_t, &tm);`

