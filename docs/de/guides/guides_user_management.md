---
category: "⚙️ Operations/Admin"
version: "v1.3.0"
status: "✅"
date: "22.12.2025"
---

# ⚙️ Benutzerverwaltung für Administratoren

Comprehensive user management guide using external authentication systems.

## 📋 Inhaltsverzeichnis

- [📋 Übersicht](#-übersicht)
- [✨ Features](#-features)
- [🚀 Quick Start](#-quick-start)
- [📖 Authentication & Integration](#-authentication--integration)
- [💡 Best Practices](#-best-practices)
- [🔧 Troubleshooting](#-troubleshooting)
- [📚 Siehe auch](#-siehe-auch)
- [📝 Changelog](#-changelog)

---

## 📋 Übersicht

**Wichtig:** ThemisDB besitzt **keine eigene Benutzerverwaltung**. Die Authentifizierung und Autorisierung erfolgt ausschließlich über **externe Systeme** wie:

- **JWT-Provider** (Keycloak, Auth0, Azure AD, AWS Cognito)
- **Apache Ranger** (Policy-Management)
- **Active Directory / LDAP** (über JWT-Integration)
- **OpenID Connect (OIDC)** Provider

**Stand:** 6. April 2026  
**Version:** 1.3.0  
**Kategorie:** ⚙️ Operations/Admin

---

## ✨ Features

- 🔐 **JWT-Based Auth** - Keycloak, Auth0, Azure AD, AWS Cognito
- 🏢 **Enterprise Integration** - Active Directory, LDAP via OIDC
- 📋 **Apache Ranger** - Policy-based access control
- 👥 **Multi-Provider** - Support for multiple authentication sources
- 🔄 **Token Management** - JWT validation, rotation, expiration
- 📊 **Audit Logging** - Track all authentication events

---

## 🚀 Quick Start

---

## Übersicht

1. [Architektur-Überblick](#architektur-überblick)
2. [JWT-basierte Authentifizierung](#jwt-basierte-authentifizierung)
3. [Active Directory / LDAP Integration](#active-directory--ldap-integration)
4. [Apache Ranger Integration](#apache-ranger-integration)
5. [RBAC und Scopes](#rbac-und-scopes)
6. [Policy Engine (ABAC)](#policy-engine-abac)
7. [Betriebliche Workflows](#betriebliche-workflows)
8. [Troubleshooting](#troubleshooting)
9. [Best Practices](#best-practices)

---

## Architektur-Überblick

ThemisDB implementiert ein **zweistufiges Sicherheitsmodell**:

```
┌─────────────────────────────────────────────────────────────┐
│                     Client Anfrage                           │
│         Authorization: Bearer <JWT oder API-Token>           │
└────────────────────┬────────────────────────────────────────┘
                     │
                     ▼
┌─────────────────────────────────────────────────────────────┐
│  Stufe 1: Token-Validierung (AuthMiddleware)                │
│  ├─ JWT-Validierung (JWKS, Signatur, Claims)                │
│  ├─ Statische API-Tokens (optional, für Services)           │
│  └─ Extraktion: user_id, groups, scopes                     │
└────────────────────┬────────────────────────────────────────┘
                     │ ✓ Token gültig
                     ▼
┌─────────────────────────────────────────────────────────────┐
│  Stufe 2: Policy Engine (ABAC - Optional)                   │
│  ├─ Ranger-kompatible Policies                              │
│  ├─ Subject/Action/Resource Matching                        │
│  ├─ IP-basierte Einschränkungen                             │
│  └─ Deny-Overrides Semantik                                 │
└────────────────────┬────────────────────────────────────────┘
                     │ ✓ Policy erlaubt
                     ▼
┌─────────────────────────────────────────────────────────────┐
│                  Zugriff gewährt                             │
└─────────────────────────────────────────────────────────────┘
```

### Externe Systeme

| System | Rolle | Integration |
|--------|-------|-------------|
| **Keycloak, Auth0, Azure AD** | Identity Provider (IdP) | JWT-Validierung via JWKS |
| **Active Directory** | Benutzerverwaltung | Via OIDC/SAML → JWT |
| **LDAP** | Gruppen-Verwaltung | Via IdP → JWT Groups Claim |
| **Apache Ranger** | Policy-Management | REST-API für Policy-Import |

---

## JWT-basierte Authentifizierung

### Grundkonzept

ThemisDB validiert **JSON Web Tokens (JWT)** mit RS256-Signatur gegen einen **JWKS-Endpunkt** (JSON Web Key Set).

**Workflow:**
1. Benutzer authentifiziert sich beim **Identity Provider** (z.B. Keycloak)
2. IdP stellt **JWT** mit Claims aus (user_id, roles, groups)
3. Client sendet JWT im `Authorization: Bearer <token>` Header
4. ThemisDB validiert JWT gegen JWKS und prüft Scopes

### Konfiguration

#### Environment-Variablen

```bash
# JWT Provider URL
# Hinweis: Keycloak 17+ nutzt /realms/ statt /auth/realms/
export THEMIS_JWT_JWKS_URL="https://keycloak.example.com/auth/realms/themis/protocol/openid-connect/certs"
# Für Keycloak 17+: https://keycloak.example.com/realms/themis/protocol/openid-connect/certs

# Erwartete Claims
export THEMIS_JWT_ISSUER="https://keycloak.example.com/auth/realms/themis"
# Für Keycloak 17+: https://keycloak.example.com/realms/themis
export THEMIS_JWT_AUDIENCE="themisdb-api"

# Cache & Toleranzen
export THEMIS_JWT_CACHE_TTL="3600"      # 1 Stunde
export THEMIS_JWT_CLOCK_SKEW="60"       # 60 Sekunden Toleranz

# Scope-Claim (welches JWT-Feld enthält die Rollen/Scopes?)
export THEMIS_JWT_SCOPE_CLAIM="roles"   # oder "groups" oder "scopes"
```

#### Konfigurationsdatei (config/auth.yaml)

```yaml
jwt:
  enabled: true
  # Keycloak 16 und früher: /auth/realms/
  # Keycloak 17+: /realms/ (ohne /auth)
  jwks_url: "https://keycloak.example.com/auth/realms/themis/protocol/openid-connect/certs"
  expected_issuer: "https://keycloak.example.com/auth/realms/themis"
  expected_audience: "themisdb-api"
  jwks_cache_ttl: 3600
  clock_skew: 60
  scope_claim: "roles"  # JWT-Feld für Scopes
```

### JWT-Format

Beispiel-JWT Payload:

```json
{
  "sub": "alice@example.com",
  "iss": "https://keycloak.example.com/auth/realms/themis",
  "aud": "themisdb-api",
  "exp": 1734567890,
  "nbf": 1734564290,
  "iat": 1734564290,
  "roles": ["admin", "data:read", "data:write"],
  "groups": ["engineering", "data-team"],
  "email": "alice@example.com"
}
```

**Wichtige Claims:**
- `sub` → Wird als `user_id` verwendet
- `roles`/`groups`/`scopes` → Werden als Scopes extrahiert (konfigurierbar via `scope_claim`)
- `exp`, `nbf`, `iat` → Zeitvalidierung (mit Clock Skew)
- `iss`, `aud` → Issuer/Audience Validierung

### API-Nutzung

```bash
# 1. Token vom IdP holen
# Keycloak 16 und früher:
TOKEN=$(curl -X POST https://keycloak.example.com/auth/realms/themis/protocol/openid-connect/token \
  -d "client_id=themisdb-client" \
  -d "client_secret=secret123" \
  -d "grant_type=client_credentials" | jq -r .access_token)

# Keycloak 17+:
# TOKEN=$(curl -X POST https://keycloak.example.com/realms/themis/protocol/openid-connect/token \
#   -d "client_id=themisdb-client" \
#   -d "client_secret=secret123" \
#   -d "grant_type=client_credentials" | jq -r .access_token)

# 2. ThemisDB API mit Token aufrufen
curl -H "Authorization: Bearer $TOKEN" \
     http://localhost:8765/entities/users:alice

# 3. Protected Endpoint (z.B. /config erfordert config:read Scope)
curl -H "Authorization: Bearer $TOKEN" \
     http://localhost:8765/config
```

---

## Active Directory / LDAP Integration

### Konzept

ThemisDB integriert sich **nicht direkt** mit Active Directory oder LDAP, sondern **indirekt über einen Identity Provider**, der AD/LDAP als Backend nutzt.

**Empfohlene Architektur:**

```
┌──────────────────┐
│ Active Directory │
│     / LDAP       │
└────────┬─────────┘
         │
         │ LDAP Bind / Group Sync
         ▼
┌──────────────────┐
│  Identity Provider│
│  (Keycloak, Auth0,│
│   Azure AD, etc.) │
└────────┬─────────┘
         │
         │ OIDC/SAML → JWT
         ▼
┌──────────────────┐
│    ThemisDB      │
│  JWT Validation  │
└──────────────────┘
```

### Beispiel: Keycloak mit Active Directory

#### 1. Keycloak User Federation (LDAP)

In Keycloak konfigurieren Sie eine **User Federation** für Active Directory:

**Realm Settings → User Federation → Add Provider → LDAP**

```yaml
Connection URL: ldap://ad.example.com:389
Bind DN: CN=svc_themis,OU=Service Accounts,DC=example,DC=com
Bind Credential: <service-account-password>

Users DN: OU=Users,DC=example,DC=com
Username LDAP attribute: sAMAccountName
UUID LDAP attribute: objectGUID

# Group Mapping
Edit Mode: WRITABLE
Sync Registrations: ON

# Group Mapper
Groups DN: OU=Groups,DC=example,DC=com
Group Name LDAP Attribute: cn
Member LDAP Attribute: member
```

#### 2. Keycloak Client für ThemisDB

**Clients → Create Client:**

```yaml
Client ID: themisdb-api
Client Protocol: openid-connect
Access Type: confidential

Valid Redirect URIs: https://themisdb.example.com/*

# Mappers: Rollen in JWT
Mappers → Create Protocol Mapper:
  Name: roles
  Mapper Type: User Realm Role
  Token Claim Name: roles
  Claim JSON Type: String
  Add to ID token: ON
  Add to access token: ON
```

#### 3. ThemisDB Konfiguration

```yaml
# config/auth.yaml
jwt:
  enabled: true
  jwks_url: "https://keycloak.example.com/auth/realms/production/protocol/openid-connect/certs"
  expected_issuer: "https://keycloak.example.com/auth/realms/production"
  expected_audience: "themisdb-api"
  scope_claim: "roles"  # Keycloak schreibt Rollen in "roles"
```

### Beispiel: Azure AD (Entra ID)

Azure AD kann **direkt als OIDC-Provider** genutzt werden:

#### 1. App Registration in Azure AD

1. **Azure Portal → Azure Active Directory → App registrations → New registration**
2. Name: `ThemisDB API`
3. Redirect URI: `https://themisdb.example.com/callback` (optional)
4. **Certificates & secrets → New client secret** (notieren!)

#### 2. API Permissions

- **Microsoft Graph → Application permissions:**
  - `User.Read.All`
  - `Group.Read.All`

#### 3. Token Configuration

- **Token configuration → Add groups claim:**
  - Group types: Security groups
  - Emit groups as role claims: ✓

#### 4. ThemisDB Konfiguration

```yaml
# config/auth.yaml
jwt:
  enabled: true
  jwks_url: "https://login.microsoftonline.com/{tenant-id}/discovery/v2.0/keys"
  expected_issuer: "https://login.microsoftonline.com/{tenant-id}/v2.0"
  expected_audience: "{client-id}"
  scope_claim: "roles"  # Azure AD: "roles" oder "groups"
```

#### 5. Token abrufen (Client Credentials Flow)

```bash
curl -X POST https://login.microsoftonline.com/{tenant-id}/oauth2/v2.0/token \
  -d "client_id={client-id}" \
  -d "client_secret={client-secret}" \
  -d "scope=api://{client-id}/.default" \
  -d "grant_type=client_credentials"
```

---

## Apache Ranger Integration

### Konzept

ThemisDB kann Policies von einem **Apache Ranger Server** importieren und in seiner **Policy Engine** evaluieren.

**Workflow:**
1. Administrator definiert Policies in Ranger UI
2. ThemisDB ruft Policies via Ranger REST-API ab
3. Policies werden in interne Policy-Engine geladen
4. Jede Anfrage wird gegen Policies geprüft

### Konfiguration

#### Environment-Variablen

```bash
export THEMIS_RANGER_BASE_URL="https://ranger.example.com"
export THEMIS_RANGER_SERVICE="themisdb-prod"
export THEMIS_RANGER_BEARER="ranger-admin-token-xyz"

# Optional: TLS
export THEMIS_RANGER_TLS_VERIFY="1"
export THEMIS_RANGER_CA_CERT="/etc/themis/certs/ranger_ca.pem"
```

#### Ranger-Client Konfiguration (config/ranger.yaml)

```yaml
ranger:
  enabled: true
  base_url: "https://ranger.example.com"
  policies_path: "/service/public/v2/api/policy"
  service_name: "themisdb-prod"
  bearer_token: "${THEMIS_RANGER_BEARER}"  # Referenz auf ENV
  
  tls_verify: true
  ca_cert_path: "/etc/themis/certs/ranger_ca.pem"
  
  # Timeouts
  connect_timeout_ms: 5000
  request_timeout_ms: 15000
  
  # Retry-Policy
  max_retries: 2
  retry_backoff_ms: 500
  
  # Auto-Sync (optional)
  auto_sync_enabled: true
  sync_interval_minutes: 15
```

### Policy-Import via API

```bash
# Policies aus Ranger importieren (manuell)
curl -X POST -H "Authorization: Bearer $THEMIS_ADMIN_TOKEN" \
     http://localhost:8765/policies/import/ranger

# Antwort:
# {
#   "status": "success",
#   "policies_imported": 12,
#   "timestamp": "2025-12-18T10:30:00Z"
# }

# Aktuelle Policies exportieren (Backup/Audit)
curl -H "Authorization: Bearer $THEMIS_ADMIN_TOKEN" \
     http://localhost:8765/policies/export/ranger > policies_backup.json
```

### Ranger Policy-Beispiel

In Ranger definierte Policy:

```json
{
  "id": 42,
  "name": "Allow read access to hr data for hr-team",
  "service": "themisdb-prod",
  "resources": {
    "entity": ["/entities/hr:*"]
  },
  "policyItems": [
    {
      "users": ["alice", "bob"],
      "groups": ["hr-team"],
      "accesses": [
        {"type": "read", "isAllowed": true}
      ]
    }
  ],
  "denyPolicyItems": []
}
```

**Konvertierung in ThemisDB Policy:**

```json
{
  "id": "ranger-42",
  "name": "Allow read access to hr data for hr-team",
  "subjects": ["alice", "bob", "group:hr-team"],
  "actions": ["read"],
  "resources": ["/entities/hr:*"],
  "effect": "allow"
}
```

### Ranger Service Definition

In Ranger muss ein **Service** für ThemisDB angelegt werden:

**Service Manager → Add Service:**

```yaml
Service Name: themisdb-prod
Service Type: Custom (oder neuer ThemisDB-Typ)

Configuration:
  themisdb.url: https://themisdb.example.com:8765
  policy.download.auth.users: themis-service-user
```

**Resource Definitions:**

```yaml
Resources:
  - Name: entity
    Label: Entity Path
    Description: Entity resource path (e.g., /entities/users:*)
    Recursive: true
    Matcher: regex

  - Name: action
    Label: Action
    Description: API action (read, write, delete, query)
    Matcher: string
```

---

## RBAC und Scopes

### Scope-basierte Autorisierung

ThemisDB nutzt **Scopes** (aus JWT-Claims oder statischen Tokens) für schnelle Autorisierung.

**Standard-Scopes:**

| Scope | Berechtigungen |
|-------|---------------|
| `admin` | Voller Zugriff auf alle Endpoints (Superuser) |
| `config:read` | Lesen der Konfiguration |
| `config:write` | Ändern der Konfiguration (Hot-Reload) |
| `cdc:read` | Lesen von Change Data Capture Streams |
| `cdc:admin` | Konfiguration von CDC (Retention, etc.) |
| `metrics:read` | Zugriff auf Prometheus-Metriken |
| `data:read` | Lesen von Entities, Queries, Graph, Vector |
| `data:write` | Schreiben von Entities (PUT, DELETE) |
| `pii:reveal` | Entschlüsselung pseudonymisierter PII-Daten |
| `pii:erase` | DSGVO Art. 17 - Recht auf Vergessenwerden |

### Scope-Mapping im Identity Provider

#### Keycloak: Realm Roles → JWT Roles

**Roles → Realm Roles:**

```yaml
admin
data:read
data:write
config:read
config:write
metrics:read
pii:reveal
```

**Users → Role Mappings:**

- User `alice`: `admin`, `data:read`, `data:write`
- User `bob`: `data:read`, `metrics:read`

**Client Mappers:**

```yaml
Name: roles-mapper
Mapper Type: User Realm Role
Token Claim Name: roles
Claim JSON Type: String
Multivalued: true
```

#### Azure AD: App Roles → JWT Roles

**App registrations → App roles:**

```json
{
  "allowedMemberTypes": ["User"],
  "description": "Administrator role",
  "displayName": "Admin",
  "value": "admin"
}
```

**Enterprise applications → Users and groups:**

- Assign users to app roles

### Statische API-Tokens (Optional)

Für **Service-Accounts** oder **Maschinen-zu-Maschine** Kommunikation können statische Tokens konfiguriert werden:

**config/auth_tokens.json:**

```json
{
  "tokens": [
    {
      "token": "themis-admin-abc123xyz",
      "user_id": "admin-service",
      "scopes": ["admin", "config:write", "data:write"]
    },
    {
      "token": "themis-readonly-def456",
      "user_id": "monitoring-service",
      "scopes": ["metrics:read", "cdc:read", "data:read"]
    }
  ]
}
```

**Nutzung:**

```bash
curl -H "Authorization: Bearer themis-admin-abc123xyz" \
     http://localhost:8765/config
```

**⚠️ Sicherheitshinweis:** Statische Tokens sind weniger sicher als JWT. Nutzen Sie sie nur für vertrauenswürdige Services.

---

## Policy Engine (ABAC)

### Attribute-Based Access Control

Zusätzlich zu Scopes können **Policies** definiert werden für feingranulare Kontrolle:

**Policy-Struktur:**

```yaml
- id: unique-policy-id
  name: Human-readable description
  subjects: ["user1", "admin", "group:engineering", "*"]
  actions: ["read", "write", "delete", "query"]
  resources: ["/entities/hr:*", "/metrics"]
  effect: allow  # oder deny
  allowed_ip_prefixes: ["10.0.", "192.168."]  # optional
```

### Policy-Beispiele

#### 1) HR-Daten nur aus internem Netzwerk

```yaml
- id: hr-internal-only
  name: HR-Daten nur aus internem Netzwerk
  subjects: ["*"]
  actions: ["read", "write", "delete"]
  resources: ["/entities/hr:*"]
  effect: allow
  allowed_ip_prefixes: ["10.0.", "192.168."]
```

#### 2) Metrics nur für Monitoring-Team

```yaml
- id: metrics-monitoring-team
  name: Metrics nur für Monitoring-Team
  subjects: ["group:monitoring", "admin"]
  actions: ["metrics.read"]
  resources: ["/metrics"]
  effect: allow
```

#### 3) Deny PII-Reveal für externe IPs

```yaml
- id: deny-pii-external
  name: PII-Reveal verweigern für externe IPs
  subjects: ["*"]
  actions: ["pii.reveal"]
  resources: ["/pii/reveal/*"]
  effect: deny
  # Nur wenn IP NICHT in allowed_ip_prefixes → deny
  allowed_ip_prefixes: ["10.0.", "192.168."]
```

### Policy-Evaluierung

**Evaluierungslogik:**

1. **Deny-Overrides:** Wenn eine Deny-Policy matched → Zugriff verweigert
2. **Allow-Policies:** Mindestens eine Allow-Policy muss matchen
3. **Default:** Wenn keine Policies konfiguriert → Allow (für Migration)
4. **IP-Check:** Wenn `allowed_ip_prefixes` gesetzt → Client-IP muss matchen

**Reihenfolge:**

```
1. Token-Scope-Check (schnell, in-memory)
   ↓ ✓ Scope vorhanden
2. Policy Engine Evaluation (wenn konfiguriert)
   ↓ ✓ Policy erlaubt
3. Zugriff gewährt
```

### Policy-Konfiguration

**config/policies.yaml:**

```yaml
- id: admin-allow-all
  name: Admin darf alles
  subjects: ["admin"]
  actions: ["*"]
  resources: ["/"]
  effect: allow

- id: readonly-safe-endpoints
  name: Readonly darf sichere Endpoints
  subjects: ["readonly", "group:monitoring"]
  actions: ["metrics.read", "config.read", "data.read"]
  resources: ["/metrics", "/config", "/entities/", "/query"]
  effect: allow

- id: deny-hr-external
  name: HR-Daten nur intern
  subjects: ["*"]
  actions: ["read", "write"]
  resources: ["/entities/hr:*"]
  effect: allow
  allowed_ip_prefixes: ["10.", "192.168.", "172.16."]
```

---

## Betriebliche Workflows

### Neuen Benutzer anlegen

**Schritt 1: Benutzer im Identity Provider anlegen**

**Keycloak:**
```
Users → Add user
Username: bob
Email: bob@example.com
Email Verified: ON

Role Mappings:
  - data:read
  - metrics:read
```

**Azure AD:**
```
Users → New user
User principal name: bob@example.com

App roles:
  - ThemisDB API: data:read, metrics:read
```

**Schritt 2: Benutzer bekommt Token**

```bash
# User Login (Authorization Code Flow)
# Browser-Flow: https://keycloak.example.com/auth/realms/themis/account
# Nach Login: JWT wird ausgestellt

# Oder: Client Credentials Flow (für Services)
TOKEN=$(curl -X POST https://keycloak.example.com/auth/realms/themis/protocol/openid-connect/token \
  -d "client_id=themisdb-client" \
  -d "client_secret=secret" \
  -d "grant_type=client_credentials" \
  -d "scope=data:read metrics:read" | jq -r .access_token)
```

**Schritt 3: Benutzer nutzt ThemisDB**

```bash
curl -H "Authorization: Bearer $TOKEN" \
     http://localhost:8765/entities/users:bob
```

### Benutzer-Rolle ändern

**Keycloak:**
```
Users → bob → Role Mappings
Remove: data:read
Add: data:write
```

**Nächster Token-Request enthält neue Rollen.**

### Benutzer deaktivieren

**Keycloak:**
```
Users → bob → Enabled: OFF
```

**Bestehende Tokens bleiben gültig bis Ablauf (`exp`)!**

**Sofortiger Entzug:**
- Token-Widerruf via IdP (wenn unterstützt)
- JWKS-Key-Rotation (alle Tokens ungültig)
- IP-Blocking (Firewall)

### Policy-Update

**Option 1: Lokale Policies (config/policies.yaml)**

```bash
# 1. Edit policies.yaml
vim /etc/themisdb/policies.yaml

# 2. Hot-Reload (erfordert config:write Scope)
curl -X POST -H "Authorization: Bearer $ADMIN_TOKEN" \
     -H "Content-Type: application/json" \
     -d '{"reload_policies": true}' \
     http://localhost:8765/config
```

**Option 2: Ranger Import (automatisch)**

```yaml
# config/ranger.yaml
auto_sync_enabled: true
sync_interval_minutes: 15
```

Policies werden alle 15 Minuten automatisch aus Ranger geladen.

**Option 3: Ranger Import (manuell)**

```bash
curl -X POST -H "Authorization: Bearer $ADMIN_TOKEN" \
     http://localhost:8765/policies/import/ranger
```

---

## Troubleshooting

### Problem: JWT-Validierung schlägt fehl

**Symptom:**
```
401 Unauthorized
{"error": "Invalid token: signature verification failed"}
```

**Diagnose:**

```bash
# 1. JWT dekodieren (ohne Validierung)
echo $TOKEN | cut -d. -f2 | base64 -d | jq .

# Prüfen:
# - "iss" stimmt mit THEMIS_JWT_ISSUER überein?
# - "aud" stimmt mit THEMIS_JWT_AUDIENCE überein?
# - "exp" ist noch nicht abgelaufen?
# - "kid" im Header existiert im JWKS?

# 2. JWKS abrufen
curl https://keycloak.example.com/auth/realms/themis/protocol/openid-connect/certs | jq .

# 3. ThemisDB Logs prüfen
docker logs themisdb | grep JWT
```

**Lösung:**
- Falsche `expected_issuer` oder `expected_audience` korrigieren
- JWKS-URL erreichbar? (Firewall, DNS)
- Clock Skew erhöhen (wenn Zeitdifferenz zwischen Systemen)

### Problem: Scope-Check schlägt fehl

**Symptom:**
```
403 Forbidden
{"error": "Missing required scope: config:write"}
```

**Diagnose:**

```bash
# 1. Token-Scopes prüfen
echo $TOKEN | cut -d. -f2 | base64 -d | jq .roles

# 2. Erwartete Scopes im Code prüfen
# Endpoint /config → Scope: config:write

# 3. IdP: Sind Rollen korrekt gemappt?
# Keycloak: Client → Mappers → roles
# Azure AD: App roles → Token configuration
```

**Lösung:**
- Benutzer-Rolle im IdP korrigieren
- Mapper im IdP prüfen (claim name: `roles` vs. `scopes`)
- `scope_claim` in config/auth.yaml korrigieren

### Problem: Policy Engine verweigert Zugriff

**Symptom:**
```
403 Forbidden
{"error": "Policy denied access"}
```

**Diagnose:**

```bash
# 1. Aktuelle Policies exportieren
curl -H "Authorization: Bearer $ADMIN_TOKEN" \
     http://localhost:8765/policies/export/ranger | jq .

# 2. Welche Policy matched?
# ThemisDB Logs (Debug-Level):
# DEBUG: Policy 'hr-internal-only' denied: IP 203.0.113.42 not in allowed prefixes

# 3. Audit-Logs prüfen
tail -f /var/log/themisdb/audit.log
```

**Lösung:**
- Deny-Policy entfernen oder Allow-Policy hinzufügen
- IP-Präfixe erweitern (wenn IP-basiert)
- `subjects` in Policy korrigieren

### Problem: Ranger-Import schlägt fehl

**Symptom:**
```
500 Internal Server Error
{"error": "Failed to fetch policies from Ranger"}
```

**Diagnose:**

```bash
# 1. Ranger-URL erreichbar?
curl -v https://ranger.example.com/service/public/v2/api/policy?serviceName=themisdb-prod

# 2. Bearer-Token gültig?
curl -H "Authorization: Bearer $THEMIS_RANGER_BEARER" \
     https://ranger.example.com/service/public/v2/api/policy?serviceName=themisdb-prod

# 3. TLS-Zertifikat vertrauenswürdig?
openssl s_client -connect ranger.example.com:443 -CAfile /etc/themis/certs/ranger_ca.pem
```

**Lösung:**
- Firewall-Regeln prüfen
- Bearer-Token erneuern
- CA-Zertifikat korrekt installieren
- `tls_verify: false` (nur für Entwicklung!)

---

## Best Practices

### 1. Nutzen Sie einen Identity Provider

**❌ Nicht empfohlen:** Statische Tokens in Konfigurationsdateien

**✅ Empfohlen:** JWT via Keycloak, Auth0, Azure AD

**Vorteile:**
- Zentrale Benutzerverwaltung
- Single Sign-On (SSO)
- Token-Rotation automatisch
- Audit-Logs im IdP
- Multi-Faktor-Authentifizierung (MFA)

### 2. Least Privilege Principle

Vergeben Sie **minimale Scopes**:

```yaml
# ❌ Schlecht
roles: ["admin"]

# ✅ Gut
roles: ["data:read", "metrics:read"]
```

### 3. Kurze Token-Laufzeiten

```yaml
# Keycloak: Realm Settings → Tokens
Access Token Lifespan: 5 minutes
Refresh Token Lifespan: 30 minutes
```

**Vorteile:**
- Gestohlene Tokens haben kurze Gültigkeit
- Forced Re-Authentication bei längeren Sessions

### 4. TLS immer aktivieren

```yaml
# config/themisdb.yaml
security:
  tls_enabled: true
  tls_cert_file: "/etc/themisdb/certs/server.crt"
  tls_key_file: "/etc/themisdb/certs/server.key"
  tls_ca_file: "/etc/themisdb/certs/ca.crt"
```

**Ohne TLS:** JWT werden im Klartext übertragen → MITM-Angriffe möglich

### 5. IP-basierte Einschränkungen

Für **hochsensible Endpunkte** (PII-Reveal, Admin-APIs):

```yaml
- id: pii-reveal-internal-only
  name: PII-Reveal nur aus internem Netzwerk
  subjects: ["*"]
  actions: ["pii.reveal"]
  resources: ["/pii/reveal/*"]
  effect: allow
  allowed_ip_prefixes: ["10.0.", "192.168."]
```

### 6. Audit-Logs aktivieren

```yaml
audit:
  enabled: true
  log_file: "/var/log/themisdb/audit.log"
  events:
    - authentication
    - authorization
    - data_modification
    - admin_operations
```

**Integration mit SIEM:** Splunk, Elastic, Azure Sentinel

### 7. Secrets Management

**❌ Nicht:** Secrets in Git committen

**✅ Empfohlen:** HashiCorp Vault, Kubernetes Secrets, Azure Key Vault

```bash
# Environment-Variablen aus Vault
export THEMIS_JWT_JWKS_URL=$(vault kv get -field=jwks_url secret/themisdb)
export THEMIS_RANGER_BEARER=$(vault kv get -field=bearer secret/ranger)
```

### 8. Monitoring & Alerting

**Prometheus-Metriken:**

```yaml
# Autorisierungsfehler überwachen
themis_authz_denied_total{reason="missing_scope"} > 10

# Ungültige Token
themis_authz_invalid_token_total > 5
```

**Alerting-Regel:**

```yaml
- alert: HighAuthFailureRate
  expr: rate(themis_authz_denied_total[5m]) > 0.1
  annotations:
    summary: "Hohe Autorisierungsfehlerrate"
```

### 9. Regelmäßige Policy-Reviews

**Quartalsweise:**
- Nicht genutzte Policies entfernen
- Overly permissive Policies verschärfen
- Audit-Logs auf Anomalien prüfen

### 10. Disaster Recovery

**Backup:**
```bash
# Policies exportieren
curl -H "Authorization: Bearer $ADMIN_TOKEN" \
     http://localhost:8765/policies/export/ranger > policies_backup_$(date +%Y%m%d).json

# Auth-Konfiguration sichern
cp /etc/themisdb/auth.yaml /backups/auth_backup_$(date +%Y%m%d).yaml
```

**Restore:**
```bash
# Policies importieren
curl -X POST -H "Authorization: Bearer $ADMIN_TOKEN" \
     -H "Content-Type: application/json" \
     -d @policies_backup_20251218.json \
     http://localhost:8765/policies/import
```

---

## Zusammenfassung

**ThemisDB hat keine eigene Benutzerverwaltung**, sondern integriert sich mit **externen Systemen**:

| System | Zweck | Integration |
|--------|-------|-------------|
| **Keycloak, Auth0, Azure AD** | Identity Provider | JWT + JWKS |
| **Active Directory** | User Directory | Via IdP (LDAP) |
| **LDAP** | Group Management | Via IdP |
| **Apache Ranger** | Policy Management | REST-API |

**Empfohlene Architektur:**

```
Active Directory / LDAP
         ↓
Identity Provider (Keycloak / Azure AD)
         ↓ JWT
      ThemisDB
   (JWT Validation + Policy Engine)
```

**Weitere Dokumentation:**

- [JWT Authentication](../auth/jwt.md) - Technische Details JWT-Validierung
- [RBAC & Authorization](guides_rbac_authorization.md) - Scopes und Policy Engine
- [Security Implementation](../security/security_implementation.md) - Sicherheitsarchitektur
- [Administrator Guide](ADMINISTRATOR_GUIDE.md) - Allgemeiner Admin-Guide

---

**Bei Fragen oder Problemen:** [GitHub Issues](https://github.com/makr-code/ThemisDB/issues)
