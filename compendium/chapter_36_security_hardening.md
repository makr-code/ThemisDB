# Kapitel 36: Security Hardening Playbook

> *"Security ist kein Feature, es ist Architektur. Ein sicheres System ist gebaut von innen heraus, nicht bolzen-on later."*

---

## Überblick

Dieses Kapitel bietet Praktiker-Anleitungen für Production-Grade Security in ThemisDB. Von grundlegenden Authentifizierungspattern bis zu erweiterten Threat-Mitigation-Strategien.

**Was Sie in diesem Kapitel lernen:**
- Network Segmentation und Firewall Rules
- Authentication & Authorization Patterns
- End-to-End Encryption
- SQL Injection & Injection Attack Prevention
- Access Control Lists (ACL) & Role-Based Access Control (RBAC)
- Audit Logging & Intrusion Detection
- Secrets Management
- Compliance & Regulatory Frameworks
- Incident Response Playbooks

---

<figure>

```mermaid
graph TB
    Client[Client Application] --> TLS[TLS slash SSL Layer]
    TLS --> Auth[Authentication]
    
    Auth --> JWT{JWT Token<br/>Validation}
    JWT -->|Valid| RBAC[RBAC Check]
    JWT -->|Invalid| Reject1[Reject 401]
    
    RBAC --> Perm{Permission<br/>Check}
    Perm -->|Granted| EncData[Encrypted Data Access]
    Perm -->|Denied| Reject2[Reject 403]
    
    EncData --> Decrypt[Decrypt at Runtime]
    Decrypt --> Result[Return Data]
    
    Result --> Audit[Audit Log]
    
    style TLS fill:#4facfe
    style EncData fill:#43e97b
    style Audit fill:#f093fb
```

<figcaption><b>Abb. 36.0:</b> Security-Layers: Defense in Depth</figcaption>
</figure>

---

## 36.1 Network Security

### Firwall-Regeln

```yaml
# themis-firewall.yaml
---
rules:
  # Externe API -> ThemisDB
  - name: "Allow API Gateway"
    type: "inbound"
    protocol: "tcp"
    port: 8529
    source: "10.0.1.0/24"  # API Gateway subnet
    action: "allow"
  
  # Client App -> ThemisDB
  - name: "Allow App Servers"
    type: "inbound"
    protocol: "tcp"
    port: 8529
    source: "10.0.2.0/24"  # App subnet
    action: "allow"
  
  # Replication zwischen Nodes
  - name: "Allow Replication"
    type: "inbound"
    protocol: "tcp"
    port: 8628
    source: "10.0.3.0/24"  # Cluster subnet
    action: "allow"
  
  # Deny All (Default)
  - name: "Deny All"
    type: "inbound"
    protocol: "all"
    action: "deny"
```

### TLS/SSL Configuration

```yaml
# themis.conf - Security Section
security:
  tls:
    enabled: true
    version: "1.3"  # Minimum
    certificate_path: "/etc/themis/certs/server.crt"
    key_path: "/etc/themis/certs/server.key"
    cipher_suites:
      - "TLS_AES_256_GCM_SHA384"
      - "TLS_CHACHA20_POLY1305_SHA256"
      - "TLS_AES_128_GCM_SHA256"
    verify_client: true  # Mutual TLS
    client_ca_path: "/etc/themis/certs/client-ca.crt"
```

---

## 36.2 Authentication & Authorization

### RBAC (Role-Based Access Control)

```aql
-- Define roles
FUNCTION create_role(role_name, permissions) {
  RETURN INSERT {
    role: role_name,
    permissions: permissions,
    created_at: NOW()
  } INTO roles
}

-- Example: Reader role
LET reader_perms = {
  collections: {
    "*": ["read"]  -- All collections, read-only
  },
  functions: ["all"]  -- Can call all functions
}

-- Create reader role
create_role("reader", reader_perms)

-- Assign user to role
UPDATE {_id: "users/alice"} WITH {
  roles: ["reader"],
  role_updated_at: NOW()
} IN users
```

### JWT Token Validation

```python
# auth.py
import jwt
from datetime import datetime, timedelta
import hashlib

class JWTAuthenticator:
    def __init__(self, secret_key):
        self.secret_key = secret_key
        self.algorithm = "HS256"
    
    def create_token(self, user_id, roles, expires_in=3600):
        payload = {
            "user_id": user_id,
            "roles": roles,
            "iat": datetime.utcnow(),
            "exp": datetime.utcnow() + timedelta(seconds=expires_in)
        }
        token = jwt.encode(payload, self.secret_key, algorithm=self.algorithm)
        return token
    
    def verify_token(self, token):
        try:
            payload = jwt.decode(token, self.secret_key, algorithms=[self.algorithm])
            return payload
        except jwt.ExpiredSignatureError:
            raise Exception("Token has expired")
        except jwt.InvalidTokenError:
            raise Exception("Invalid token")
    
    def get_user_permissions(self, payload):
        user_id = payload["user_id"]
        roles = payload["roles"]
        # Lookup permissions from database
        return {"roles": roles, "timestamp": datetime.utcnow()}
```

---

## 36.3 Encryption Patterns

### Field-Level Encryption

```aql
-- Encrypt sensitive fields before storage
FUNCTION encrypt_field(value, key) {
  -- Use ThemisDB's built-in CRYPTO_ENCRYPT function
  RETURN CRYPTO_ENCRYPT(value, key, 'AES-256-GCM')
}

-- Store with encrypted field
INSERT {
  user_id: 'user_123',
  name: 'Alice',
  email: encrypt_field('alice@example.com', encryption_key),
  ssn: encrypt_field('123-45-6789', encryption_key),
  created_at: NOW()
} INTO users

-- Decrypt on read
FOR user IN users
  FILTER user.user_id == 'user_123'
  LET decrypted_email = CRYPTO_DECRYPT(user.email, encryption_key)
  RETURN {
    name: user.name,
    email: decrypted_email
  }
```

### End-to-End Encryption (E2EE)

```python
# e2e_crypto.py
from cryptography.hazmat.primitives import hashes
from cryptography.hazmat.primitives.asymmetric import rsa, padding
from cryptography.hazmat.backends import default_backend

class E2EEncryption:
    def __init__(self):
        self.backend = default_backend()
    
    def generate_keypair(self):
        """Generate RSA keypair for client"""
        private_key = rsa.generate_private_key(
            public_exponent=65537,
            key_size=4096,
            backend=self.backend
        )
        public_key = private_key.public_key()
        return private_key, public_key
    
    def encrypt_message(self, message, public_key):
        """Encrypt message with public key"""
        ciphertext = public_key.encrypt(
            message.encode(),
            padding.OAEP(
                mgf=padding.MGF1(algorithm=hashes.SHA256()),
                algorithm=hashes.SHA256(),
                label=None
            )
        )
        return ciphertext.hex()
    
    def decrypt_message(self, ciphertext_hex, private_key):
        """Decrypt with private key (only client)"""
        ciphertext = bytes.fromhex(ciphertext_hex)
        plaintext = private_key.decrypt(
            ciphertext,
            padding.OAEP(
                mgf=padding.MGF1(algorithm=hashes.SHA256()),
                algorithm=hashes.SHA256(),
                label=None
            )
        )
        return plaintext.decode()
```

---

## 36.4 Injection Prevention

### AQL Injection Protection

```aql
-- ❌ UNSAFE: User input directly in query
FUNCTION unsafe_search(search_term) {
  RETURN EXECUTE(
    CONCAT("FOR doc IN collection FILTER LIKE(doc.name, '%", search_term, "%') RETURN doc")
  )
  -- Attacker can inject: %') OR (1==1) //'
}

-- ✅ SAFE: Parameterized queries
FUNCTION safe_search(search_term) {
  RETURN (
    FOR doc IN collection
    FILTER LIKE(doc.name, @search_pattern)
    RETURN doc
  )
}

-- Call with bound parameters:
safe_search('@search_pattern', '%term%')
```

### Input Validation

```python
# validation.py
import re
from typing import Any, Dict

class InputValidator:
    @staticmethod
    def validate_email(email: str) -> bool:
        pattern = r'^[a-zA-Z0-9._%+-]+@[a-zA-Z0-9.-]+\.[a-zA-Z]{2,}$'
        return re.match(pattern, email) is not None
    
    @staticmethod
    def validate_aql_identifier(identifier: str) -> bool:
        # Only allow alphanumeric and underscore
        pattern = r'^[a-zA-Z_][a-zA-Z0-9_]*$'
        return re.match(pattern, identifier) is not None
    
    @staticmethod
    def sanitize_collection_name(name: str) -> str:
        # Whitelist: only alphanumeric, underscore, dash
        sanitized = re.sub(r'[^a-zA-Z0-9_\-]', '', name)
        return sanitized.lower()
    
    @staticmethod
    def validate_query_size(query: str, max_size: int = 10000) -> bool:
        return len(query) <= max_size
```

---

## 36.5 Audit Logging

### Comprehensive Audit Trail

```aql
-- Log all operations
FUNCTION audit_log(operation, user_id, resource, changes) {
  RETURN INSERT {
    timestamp: NOW(),
    operation: operation,  -- "INSERT", "UPDATE", "DELETE"
    user_id: user_id,
    resource: resource,    -- "users/123"
    changes: changes,      -- {field: old_value -> new_value}
    ip_address: @ip_address,
    user_agent: @user_agent
  } INTO audit_logs
}

-- Usage:
UPDATE {_id: 'users/123'} WITH {email: 'new@example.com'}
audit_log('UPDATE', 'admin_001', 'users/123', {
  email: 'old@example.com' -> 'new@example.com'
})
```

### Immutable Audit Log

```python
# audit_storage.py
class ImmutableAuditLog:
    def __init__(self, db):
        self.db = db
    
    def append_log(self, log_entry):
        """Append-only: cannot update/delete"""
        # Insert with timestamp
        log_entry['_timestamp'] = time.time()
        log_entry['_hash'] = self._compute_hash(log_entry)
        
        # Previous entry's hash
        last_entry = self.db.get_last_audit_log()
        if last_entry:
            log_entry['_previous_hash'] = last_entry['_hash']
        
        return self.db.insert_audit_log(log_entry)
    
    def _compute_hash(self, entry):
        """Cryptographic hash for integrity"""
        import hashlib
        content = json.dumps(entry, sort_keys=True)
        return hashlib.sha256(content.encode()).hexdigest()
    
    def verify_integrity(self):
        """Detect tampering by verifying hash chain"""
        logs = self.db.get_all_audit_logs()
        for i, log in enumerate(logs):
            if i > 0:
                prev_log = logs[i-1]
                if log['_previous_hash'] != prev_log['_hash']:
                    raise Exception(f"Integrity violation at log {i}")
```

---

## 36.6 Secrets Management

### Kubernetes Secrets Integration

```yaml
# kubernetes-secret.yaml
apiVersion: v1
kind: Secret
metadata:
  name: themis-secrets
  namespace: themis
type: Opaque
stringData:
  # Database encryption key
  database_key: "base64_encoded_key_here"
  
  # TLS certificates
  tls_cert: |
    -----BEGIN CERTIFICATE-----
    ...
    -----END CERTIFICATE-----
  
  tls_key: |
    -----BEGIN PRIVATE KEY-----
    ...
    -----END PRIVATE KEY-----
  
  # API keys for external services
  stripe_api_key: "sk_live_..."
  sendgrid_api_key: "SG...."
```

### Vault Integration

```python
# secrets_manager.py
import hvac
import os

class VaultSecretsManager:
    def __init__(self, vault_addr, token):
        self.client = hvac.Client(url=vault_addr, token=token)
    
    def get_secret(self, secret_path):
        response = self.client.secrets.kv.v2.read_secret_version(
            path=secret_path
        )
        return response['data']['data']
    
    def rotate_database_key(self):
        """Rotate encryption key - called before data re-encryption"""
        new_key = os.urandom(32)  # 256-bit key
        self.client.secrets.kv.v2.create_or_update_secret(
            path="themis/database_key",
            secret_dict={"key": new_key.hex()}
        )
        return new_key
```

---

## 36.7 Compliance & Regulatory

### GDPR Compliance

```aql
-- Right to be forgotten: Pseudonymization
FUNCTION gdpr_pseudonymize_user(user_id) {
  LET user = DOCUMENT('users/' + user_id)
  
  UPDATE {_id: 'users/' + user_id} WITH {
    email: HASH(user.email),
    phone: null,
    address: null,
    name: "Anonymized User",
    gdpr_deleted_at: NOW(),
    is_pseudonymized: true
  } IN users
  
  -- Also delete from audit logs
  REMOVE l IN audit_logs
  FILTER l.resource == CONCAT('users/', user_id)
}
```

### SOC 2 Compliance Checklist

```markdown
## SOC 2 Type II Compliance

- [ ] **Access Control**
  - [ ] Multi-factor authentication (MFA)
  - [ ] Role-based access control (RBAC)
  - [ ] Audit logging of all access

- [ ] **Data Security**
  - [ ] End-to-end encryption (AES-256)
  - [ ] Field-level encryption for PII
  - [ ] Secure key management (Vault)

- [ ] **Availability**
  - [ ] 99.99% uptime SLA
  - [ ] Automated failover
  - [ ] Regular disaster recovery tests

- [ ] **Monitoring & Logging**
  - [ ] 24/7 intrusion detection
  - [ ] Immutable audit logs
  - [ ] Real-time alerting

- [ ] **Incident Response**
  - [ ] Documented IR procedures
  - [ ] Regular tabletop exercises
  - [ ] <1 hour incident response time
```

---

## 36.8 Incident Response Playbook

### Detection

```aql
-- Detect unusual access patterns
FUNCTION detect_brute_force(user_id, threshold = 5) {
  LET failed_logins = LENGTH(
    FOR log IN audit_logs
    FILTER log.user_id == user_id
    FILTER log.operation == 'FAILED_LOGIN'
    FILTER log.timestamp > DATE_SUBTRACT(NOW(), 1, 'hour')
    RETURN log
  )
  
  IF failed_logins >= threshold THEN
    RETURN {
      alert: "BRUTE_FORCE_DETECTED",
      user_id: user_id,
      failed_count: failed_logins,
      action: "LOCK_ACCOUNT"
    }
  END
  
  RETURN {alert: "normal"}
}
```

### Response

```python
# incident_response.py
class IncidentResponse:
    def lock_account(self, user_id):
        """Lock account immediately"""
        db.update_user(user_id, {'locked': True, 'locked_at': now()})
    
    def notify_security_team(self, incident):
        """Alert security team"""
        slack.send_message(f"🚨 SECURITY ALERT: {incident['alert']}")
        email.send_alert(incident)
    
    def revoke_tokens(self, user_id):
        """Invalidate all active tokens"""
        db.delete_user_sessions(user_id)
    
    def isolate_database(self):
        """Disable external access"""
        firewall.block_external_connections()
    
    def preserve_evidence(self):
        """Archive logs for forensic analysis"""
        backup.create_forensic_snapshot()
```

---

## Zusammenfassung

Production-Sicherheit erfordert:
- ✅ **Network Segmentation** - Firewall rules, TLS 1.3
- ✅ **Authentication** - JWT, MFA, RBAC
- ✅ **Encryption** - AES-256 field-level, E2E
- ✅ **Input Validation** - Parameterized queries, sanitization
- ✅ **Audit Logging** - Immutable logs, integrity checks
- ✅ **Secrets Management** - Vault, rotation
- ✅ **Compliance** - GDPR, SOC 2, audit trails
- ✅ **Incident Response** - Playbooks, automation

Mit diesen Patterns bauen Sie ein **Defense-in-Depth** System, wo mehrere Sicherheitsebenen zusammenarbeiten.
