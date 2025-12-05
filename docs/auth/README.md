# Authentication Documentation

**Stand:** 5. Dezember 2025  
**Version:** 1.0.0  
**Kategorie:** Auth

---

## Übersicht

ThemisDB unterstützt JWT-basierte Authentifizierung mit JWKS-Validierung.

## Features

| Feature | Status |
|---------|--------|
| JWT Authentication | ✅ Production |
| JWKS Key Rotation | ✅ Production |
| RBAC Authorization | ✅ Production |
| mTLS | ✅ Production |

## Source-Code Referenz

| Komponente | Header | Source |
|------------|--------|--------|
| AuthMiddleware | `include/server/auth_middleware.h` | `src/server/auth_middleware.cpp` |
| RBAC | `include/security/rbac.h` | `src/security/rbac.cpp` |

## JWT Token Format

```json
{
  "sub": "user-123",
  "iss": "themisdb",
  "exp": 1733400000,
  "roles": ["admin", "reader"],
  "permissions": ["read", "write", "delete"]
}
```

## Dokumentation in diesem Ordner

| Datei | Beschreibung |
|-------|--------------|
| [jwt.md](jwt.md) | JWT Authentication |
| [jwks_example.json](jwks_example.json) | JWKS Example |

## Verwandte Dokumentation

- [Security Module](../security/README.md) - Security Implementation
- [Server Module](../server/README.md) - Auth Middleware
