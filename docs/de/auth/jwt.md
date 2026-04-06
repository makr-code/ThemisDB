# JWT / OpenID Connect Authentifizierung

**Stand:** 6. April 2026
**Version:** 2.0.0
**Kategorie:** Auth

---

## Übersicht

- `auth::JWTValidator` verifiziert RS256/ES256/EdDSA-signierte JWTs gegen einen JWKS-Endpoint
  oder ein injiziertes JWKS-Dokument. Es werden Signatur (`kid` → JWK) und Standard-Claims
  validiert: `exp`, `nbf`, `iss`, `aud`.
- `AuthMiddleware` (in `src/server/`) nutzt `JWTValidator`, um HTTP-Endpunkte zu schützen
  und die geparsten Claims an nachgelagerte Handler weiterzugeben.

Für die vollständige API-Referenz siehe [`include/auth/README.md`](../../../include/auth/README.md).

---

## Beispiel-JWKS

Eine minimale JWKS-Datei (siehe [`jwks_example.json`](jwks_example.json)):

```json
{
  "keys": [
    {
      "kty": "RSA",
      "kid": "example-key-1",
      "use": "sig",
      "alg": "RS256",
      "n": "...base64url modulus...",
      "e": "AQAB"
    }
  ]
}
```

`n` muss durch den base64url-kodierten RSA-Modulus des Public Keys ersetzt werden.
`e` ist üblicherweise `AQAB`.

---

## Konfiguration (C++)

Die JWT-Konfiguration erfolgt über `JWTValidatorConfig` (siehe `include/auth/jwt_validator.h`):

| Parameter | Typ | Beschreibung |
|-----------|-----|--------------|
| `jwks_url` | `string` | JWKS-Endpoint URL (optional bei Test-Injection) |
| `expected_issuer` | `string` | Erwarteter `iss`-Claim |
| `expected_audience` | `string` | Erwarteter `aud`-Claim |
| `jwks_cache_ttl` | `seconds` | Cache-Lebensdauer für JWKS (Standard: 300 s) |
| `clock_skew` | `seconds` | Erlaubte Zeitabweichung (Standard: 60 s) |

Beispiel:

```cpp
#include "auth/jwt_validator.h"
using namespace themis::auth;

JWTValidatorConfig cfg;
cfg.jwks_url          = "https://pki.example.com/.well-known/jwks.json";
cfg.expected_issuer   = "https://auth.example.com";
cfg.expected_audience = "themis-api";
cfg.jwks_cache_ttl    = std::chrono::seconds(300);
cfg.clock_skew        = std::chrono::seconds(60);

JWTValidator validator(cfg);
auto claims = validator.parseAndValidate(bearer_token);
// claims.sub, claims.email, claims.roles, claims.groups verfügbar
```

---

## Lokales Testen

### Unit-Tests

Das Repository enthält `tests/test_jwt_validator.cpp` mit Unit-Tests, die RSA-Keys on-the-fly
generieren und JWKS via `setJWKSForTesting(...)` injizieren.

Build und Ausführung (Linux/macOS):

```bash
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --target themis_tests -j$(nproc)
./build/themis_tests --gtest_filter=JWTValidatorTest.*
```

Build und Ausführung (Windows):

```powershell
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release --target themis_tests
.\build\Release\themis_tests.exe --gtest_filter=JWTValidatorTest.*
```

### Manueller curl-Test

1. HTTP-Server starten, der `docs/de/auth/jwks_example.json` ausliefert:

   ```bash
   python3 -m http.server 8000 --directory docs/de/auth/
   ```

2. Request an geschützten Endpunkt:

   ```bash
   curl -H "Authorization: Bearer <JWT>" http://localhost:8080/api/protected
   ```

   Bei gültigem Token wird der Request weitergeleitet; sonst wird 401 zurückgegeben.

---

## Häufige Fehler

| Problem | Ursache | Lösung |
|---------|---------|--------|
| `kid not found in JWKS` | JWKS enthält nicht den `kid` aus dem JWT-Header | JWKS aktualisieren oder `kid` im JWT korrigieren |
| `Token expired` | `exp`-Claim überschritten | Neues Token anfordern; `clock_skew` prüfen |
| `Invalid signature` | Falscher Signing-Key oder `n`/`e` falsch base64url-kodiert | Key-Material prüfen; kein Padding in base64url |
| `Issuer mismatch` | `iss`-Claim stimmt nicht mit `expected_issuer` überein | Konfiguration anpassen |
| `JWKS HTTP error` | JWKS-Endpoint nicht erreichbar | Netzwerk/URL prüfen; Retry-Logik nutzen |

---

## Weiterführende Dokumentation

- [Auth-Modul Übersicht](README.md) — Alle Authentifizierungsverfahren im Überblick
- [src/auth/README.md](../../../src/auth/README.md) — Vollständige Implementierungsdokumentation
- [include/auth/README.md](../../../include/auth/README.md) — Vollständige API-Referenz
