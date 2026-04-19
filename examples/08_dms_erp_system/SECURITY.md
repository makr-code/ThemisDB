> **Sicherheitshinweis:** Security-Angaben gegen aktuelle Build-Flags, Codepfade und Tests validieren.

# DMS/ERP System - Sicherheitskonzepte

## Übersicht

Dieses Dokument beschreibt die Sicherheitsarchitektur und Best Practices für das DMS/ERP-System.

## Sicherheitsarchitektur

### Defense in Depth

```
┌─────────────────────────────────────────┐
│  Layer 1: Network Security (Firewall)  │
├─────────────────────────────────────────┤
│  Layer 2: SSL/TLS Encryption           │
├─────────────────────────────────────────┤
│  Layer 3: Authentication & Authorization│
├─────────────────────────────────────────┤
│  Layer 4: Application Security         │
├─────────────────────────────────────────┤
│  Layer 5: Data Encryption at Rest      │
├─────────────────────────────────────────┤
│  Layer 6: Audit Logging                │
└─────────────────────────────────────────┘
```

## Authentifizierung

### Multi-Factor Authentication (MFA)

**TOTP-basiert**:
```python
from pyotp import TOTP

class MFAService:
    def setup_mfa(self, user):
        """Richtet MFA für Benutzer ein"""
        secret = pyotp.random_base32()
        user.mfa_secret = self.encrypt(secret)
        
        # QR-Code für Authenticator-App
        totp = TOTP(secret)
        qr_uri = totp.provisioning_uri(
            user.email,
            issuer_name="DMS/ERP System"
        )
        
        return qr_uri
    
    def verify_mfa(self, user, code):
        """Verifiziert MFA-Code"""
        secret = self.decrypt(user.mfa_secret)
        totp = TOTP(secret)
        return totp.verify(code, valid_window=1)
```

### Session Management

**Sichere Sessions**:
```python
from flask import session
import secrets

class SessionManager:
    def create_session(self, user):
        """Erstellt sichere Session"""
        session['user_id'] = user.id
        session['csrf_token'] = secrets.token_hex(32)
        session['created_at'] = datetime.now().isoformat()
        session.permanent = True  # Mit Timeout
        
    def validate_session(self):
        """Validiert aktive Session"""
        if 'user_id' not in session:
            return False
        
        # Session-Timeout prüfen (1 Stunde)
        created = datetime.fromisoformat(session['created_at'])
        if (datetime.now() - created).seconds > 3600:
            self.destroy_session()
            return False
        
        return True
    
    def destroy_session(self):
        """Beendet Session"""
        session.clear()
```

### Passwort-Sicherheit

**Hashing mit Argon2**:
```python
from argon2 import PasswordHasher

ph = PasswordHasher(
    time_cost=3,      # Iterationen
    memory_cost=65536,  # 64 MB
    parallelism=4,
    hash_len=32,
    salt_len=16
)

def hash_password(password):
    """Hasht Passwort sicher"""
    return ph.hash(password)

def verify_password(hash, password):
    """Verifiziert Passwort"""
    try:
        ph.verify(hash, password)
        return True
    except:
        return False
```

**Passwort-Richtlinien**:
```python
def validate_password_strength(password):
    """Prüft Passwort-Stärke"""
    errors = []
    
    if len(password) < 12:
        errors.append("Mindestens 12 Zeichen")
    
    if not re.search(r'[A-Z]', password):
        errors.append("Mindestens ein Großbuchstabe")
    
    if not re.search(r'[a-z]', password):
        errors.append("Mindestens ein Kleinbuchstabe")
    
    if not re.search(r'[0-9]', password):
        errors.append("Mindestens eine Zahl")
    
    if not re.search(r'[!@#$%^&*(),.?":{}|<>]', password):
        errors.append("Mindestens ein Sonderzeichen")
    
    # Häufige Passwörter prüfen
    if password.lower() in common_passwords:
        errors.append("Passwort zu häufig verwendet")
    
    return errors
```

## Autorisierung

### Role-Based Access Control (RBAC)

**Rollen-Definition**:
```python
class Role(Enum):
    SUPER_ADMIN = "super_admin"
    ADMIN = "admin"
    MANAGER = "manager"
    USER = "user"
    VIEWER = "viewer"

# Permissions pro Rolle
ROLE_PERMISSIONS = {
    Role.SUPER_ADMIN: ["*"],  # Alle Rechte
    Role.ADMIN: [
        "documents.create", "documents.read", "documents.update", "documents.delete",
        "workflows.manage", "users.manage", "reports.view"
    ],
    Role.MANAGER: [
        "documents.create", "documents.read", "documents.update",
        "workflows.approve", "reports.view"
    ],
    Role.USER: [
        "documents.create", "documents.read", "documents.update_own",
        "workflows.submit"
    ],
    Role.VIEWER: [
        "documents.read", "reports.view"
    ]
}
```

**Permission Check**:
```python
def requires_permission(permission):
    """Decorator für Permission-Check"""
    def decorator(f):
        @wraps(f)
        def decorated_function(*args, **kwargs):
            user = get_current_user()
            
            if not has_permission(user, permission):
                abort(403, "Keine Berechtigung")
            
            return f(*args, **kwargs)
        return decorated_function
    return decorator

def has_permission(user, permission):
    """Prüft ob User Permission hat"""
    user_permissions = ROLE_PERMISSIONS.get(user.role, [])
    
    # Wildcard für SUPER_ADMIN
    if "*" in user_permissions:
        return True
    
    return permission in user_permissions

# Verwendung
@app.route('/api/documents', methods=['POST'])
@requires_permission('documents.create')
def create_document():
    # Implementation
    pass
```

### Attribute-Based Access Control (ABAC)

**Context-basierte Zugriffskontrolle**:
```python
def check_document_access(user, document, action):
    """Prüft Zugriff auf Dokument"""
    
    # Owner hat immer Zugriff
    if document.created_by == user.id:
        return True
    
    # Rolle-basiert
    if action == "read":
        if user.role in [Role.ADMIN, Role.SUPER_ADMIN]:
            return True
        
        # Abteilungs-basiert
        if document.department == user.department:
            return True
    
    elif action == "update":
        if user.role in [Role.ADMIN, Role.SUPER_ADMIN]:
            return True
        
        # Status-basiert
        if document.status == "draft" and document.created_by == user.id:
            return True
    
    elif action == "delete":
        if user.role == Role.SUPER_ADMIN:
            return True
    
    return False
```

## Datenverschlüsselung

### Encryption at Rest

**Feld-Level Encryption**:
```python
from cryptography.fernet import Fernet

class EncryptionService:
    def __init__(self, key):
        self.cipher = Fernet(key)
    
    def encrypt_field(self, value):
        """Verschlüsselt einzelnes Feld"""
        if value is None:
            return None
        
        encrypted = self.cipher.encrypt(value.encode())
        return encrypted.decode()
    
    def decrypt_field(self, encrypted_value):
        """Entschlüsselt Feld"""
        if encrypted_value is None:
            return None
        
        decrypted = self.cipher.decrypt(encrypted_value.encode())
        return decrypted.decode()

# Sensitive Felder verschlüsseln
class Document:
    def __init__(self):
        self.title = "Public Title"
        self._content_encrypted = None  # Verschlüsselt
    
    @property
    def content(self):
        return encryption_service.decrypt_field(self._content_encrypted)
    
    @content.setter
    def content(self, value):
        self._content_encrypted = encryption_service.encrypt_field(value)
```

### Encryption in Transit

**TLS/SSL Konfiguration**:
```nginx
# Nginx SSL Config
ssl_protocols TLSv1.2 TLSv1.3;
ssl_ciphers 'ECDHE-ECDSA-AES128-GCM-SHA256:ECDHE-RSA-AES128-GCM-SHA256';
ssl_prefer_server_ciphers off;

# HSTS
add_header Strict-Transport-Security "max-age=63072000" always;

# Certificate Pinning
add_header Public-Key-Pins 'pin-sha256="base64+primary=="; pin-sha256="base64+backup=="; max-age=5184000';
```

## Input Validation & Sanitization

### SQL Injection Prevention

**Parametrisierte Queries**:
```python
# ❌ Unsicher
query = f"SELECT * FROM documents WHERE id = '{doc_id}'"

# ✅ Sicher
query = "SELECT * FROM documents WHERE id = ?"
result = db.execute(query, (doc_id,))
```

### XSS Prevention

**Output Encoding**:
```python
from markupsafe import escape

def safe_render(user_input):
    """Escaped HTML für sicheres Rendering"""
    return escape(user_input)

# In Templates
{{ user_input | e }}  # Jinja2 Auto-Escape
```

### CSRF Protection

**Token-basiert**:
```python
from flask_wtf.csrf import CSRFProtect

csrf = CSRFProtect(app)

# In Forms
<form method="post">
    <input type="hidden" name="csrf_token" value="{{ csrf_token() }}"/>
    <!-- Form fields -->
</form>
```

### File Upload Security

**Sichere Datei-Uploads**:
```python
import magic

ALLOWED_EXTENSIONS = {'pdf', 'docx', 'xlsx', 'png', 'jpg'}
MAX_FILE_SIZE = 50 * 1024 * 1024  # 50 MB

def secure_file_upload(file):
    """Validiert und speichert Datei sicher"""
    
    # Extension Check
    if not allowed_file(file.filename):
        raise ValueError("Dateityp nicht erlaubt")
    
    # Size Check
    file.seek(0, 2)
    size = file.tell()
    file.seek(0)
    if size > MAX_FILE_SIZE:
        raise ValueError("Datei zu groß")
    
    # Magic Bytes Check (echten Dateityp prüfen)
    mime = magic.from_buffer(file.read(1024), mime=True)
    file.seek(0)
    if mime not in ALLOWED_MIME_TYPES:
        raise ValueError("Dateityp nicht erlaubt")
    
    # Virus Scan (ClamAV)
    scan_result = scan_file_for_viruses(file)
    if not scan_result['clean']:
        raise ValueError("Virus gefunden")
    
    # Sicherer Dateiname
    filename = secure_filename(file.filename)
    
    # Random Prefix
    filename = f"{secrets.token_hex(8)}_{filename}"
    
    # Außerhalb Web Root speichern
    filepath = os.path.join('/secure/uploads', filename)
    file.save(filepath)
    
    return filename
```

## Audit Logging

### Compliance-konformes Logging

**Audit Trail Implementation**:
```python
class AuditLog:
    def log_action(self, user, action, entity_type, entity_id, changes=None):
        """Loggt Benutzeraktion"""
        log_entry = {
            'timestamp': datetime.now().isoformat(),
            'user_id': user.id,
            'user_email': user.email,
            'user_ip': request.remote_addr,
            'action': action,  # CREATE, READ, UPDATE, DELETE
            'entity_type': entity_type,
            'entity_id': entity_id,
            'changes': changes,  # Alte vs neue Werte
            'session_id': session.get('id'),
            'user_agent': request.headers.get('User-Agent')
        }
        
        # In Datenbank speichern
        db.audit_logs.insert(log_entry)
        
        # Optional: Syslog
        syslog.info(f"AUDIT: {user.email} {action} {entity_type} {entity_id}")

# Verwendung
@app.route('/api/documents/<doc_id>', methods=['PUT'])
def update_document(doc_id):
    old_doc = get_document(doc_id)
    new_doc = update_document_data(doc_id, request.json)
    
    # Änderungen tracken
    changes = {
        'old': old_doc.to_dict(),
        'new': new_doc.to_dict()
    }
    
    audit.log_action(
        user=current_user,
        action='UPDATE',
        entity_type='document',
        entity_id=doc_id,
        changes=changes
    )
    
    return jsonify(new_doc.to_dict())
```

### GDPR-Compliance

**Daten-Anonymisierung**:
```python
def anonymize_user_data(user_id):
    """Anonymisiert Benutzerdaten (GDPR Art. 17)"""
    user = User.get(user_id)
    
    # Persönliche Daten entfernen
    user.email = f"deleted_{user_id}@anonymized.local"
    user.name = f"Deleted User {user_id}"
    user.phone = None
    user.address = None
    
    # Audit-Logs anonymisieren
    AuditLog.update_many(
        {'user_id': user_id},
        {'$set': {'user_email': user.email}}
    )
    
    user.deleted_at = datetime.now()
    user.save()
```

## Security Headers

**HTTP Security Headers**:
```python
@app.after_request
def set_security_headers(response):
    """Setzt Security Headers"""
    headers = {
        'X-Content-Type-Options': 'nosniff',
        'X-Frame-Options': 'DENY',
        'X-XSS-Protection': '1; mode=block',
        'Strict-Transport-Security': 'max-age=31536000; includeSubDomains',
        'Content-Security-Policy': "default-src 'self'; script-src 'self' 'unsafe-inline'; style-src 'self' 'unsafe-inline'",
        'Referrer-Policy': 'strict-origin-when-cross-origin',
        'Permissions-Policy': 'geolocation=(), microphone=(), camera=()'
    }
    
    for key, value in headers.items():
        response.headers[key] = value
    
    return response
```

## Rate Limiting

**API Rate Limiting**:
```python
from flask_limiter import Limiter

limiter = Limiter(
    app,
    key_func=lambda: request.remote_addr,
    default_limits=["200 per day", "50 per hour"]
)

# Per-Endpoint Limits
@app.route('/api/login', methods=['POST'])
@limiter.limit("5 per minute")
def login():
    # Login-Logic
    pass

@app.route('/api/documents', methods=['GET'])
@limiter.limit("100 per minute")
def list_documents():
    # Implementation
    pass
```

## Incident Response

### Security Incident Handling

**Incident Response Plan**:
```python
class SecurityIncident:
    SEVERITY_CRITICAL = 1
    SEVERITY_HIGH = 2
    SEVERITY_MEDIUM = 3
    SEVERITY_LOW = 4
    
    def report_incident(self, type, severity, details):
        """Meldet Security Incident"""
        incident = {
            'timestamp': datetime.now(),
            'type': type,  # 'unauthorized_access', 'data_breach', etc.
            'severity': severity,
            'details': details,
            'status': 'open'
        }
        
        # Sofortige Benachrichtigung bei CRITICAL
        if severity == self.SEVERITY_CRITICAL:
            self.notify_security_team(incident)
            self.enable_lockdown_mode()
        
        # Incident speichern
        db.security_incidents.insert(incident)
        
        return incident['id']
    
    def enable_lockdown_mode(self):
        """Aktiviert Security Lockdown"""
        # Alle Sessions invalidieren
        session_manager.invalidate_all_sessions()
        
        # Admin-only Modus
        app.config['LOCKDOWN_MODE'] = True
        
        # Benachrichtigungen
        notify_all_users("System im Security Lockdown")
```

## Penetration Testing

### Security Checklist

**Regelmäßige Tests**:
```bash
# OWASP ZAP Scan
zap-cli quick-scan http://localhost:5000

# SQLMap Test
sqlmap -u "http://localhost:5000/api/documents?id=1" --batch

# Nikto Web Scanner
nikto -h localhost:5000

# SSL Test
testssl.sh localhost:443
```

## Best Practices

### ✅ DO

1. **Principle of Least Privilege** - Minimale Berechtigungen
2. **Defense in Depth** - Mehrere Sicherheitsebenen
3. **Fail Secure** - Bei Fehler sicher verhalten
4. **Keep Software Updated** - Regelmäßige Updates
5. **Security by Design** - Von Anfang an sicher entwickeln
6. **Regular Audits** - Regelmäßige Security-Reviews
7. **Encrypt Sensitive Data** - At rest und in transit
8. **Monitor and Alert** - Anomalien erkennen

### ❌ DON'T

1. **Passwörter im Klartext** - Immer hashen
2. **Default Credentials** - Immer ändern
3. **Unnötige Services** - Angriffsfläche reduzieren
4. **Detaillierte Error Messages** - Keine Infos leaken
5. **Unvalidierte Inputs** - Immer validieren
6. **Hardcoded Secrets** - Environment Variables nutzen
7. **Disabled Security Features** - Für "Performance"
8. **Outdated Dependencies** - Security Patches installieren

---

**Compliance**: GDPR, DSGVO, ISO 27001
**Letzte Aktualisierung**: 2025-12-22
