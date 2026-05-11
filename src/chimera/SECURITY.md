> **Sicherheitshinweis:** Security-Angaben gegen aktuelle Build-Flags, Codepfade und Tests validieren.

# SECURITY

## Scope
- Modul/Ordner: `src/chimera`
- Sicherheitsrelevante Funktionen in `src/chimera/themisdb_adapter.cpp` und `include/chimera/themisdb_adapter.hpp`.

## Threat Model

| Threat | Mitigation |
|--------|-----------|
| Connection string credential leakage | Credential masking (`user:pass@` → `***:***@`) in stored/logged connection strings |
| Unvalidated connection state | All operation methods check `connected_` flag and return `CONNECTION_ERROR` if not connected (`src/chimera/themisdb_adapter.cpp`) |
| Engine-backed path NOT_IMPLEMENTED silent failure | Engine dispatch paths return structured `ErrorCode::NOT_IMPLEMENTED`; callers must check `Result<T>` |
| Unvalidated query parameters | Parameters passed through to `execute_query`; input validation is the caller's responsibility at the API layer |

## Security Controls
- Connection-string parsing with credential masking in `ThemisDBAdapter::connect()`
- `Result<T>` error propagation: no silent failures on connection or operation errors
- Engine injection constructor (`ThemisDBAdapter(QueryEngine*, VectorIndexManager*, GraphIndexManager*)`) limits injection surface to trusted callers

## Known Limitations
- No rate limiting in `ThemisDBAdapter`; must be enforced at the API layer
- Connection pooling is conditional: `has_capability(CONNECTION_POOLING)` returns `true` only when a pool provider has been injected via `setConnectionPool(std::function<void*()>)`; without injection, pooling is disabled and the capability is not reported
- No SSL/TLS configuration in the adapter connection interface

## Incident & Meldung
- Sicherheitsfunde gemäß Root-`SECURITY.md` melden und behandeln.
