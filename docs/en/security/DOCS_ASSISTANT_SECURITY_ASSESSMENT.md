# Documentation Assistant - Security Assessment (TODO)

**Status:** 🔴 **IN PLANUNG** - Muss vor Implementierung von zusätzlichen Datenbanken abgeschlossen werden

**Datum:** 2026-01-11  
**Verantwortlich:** Security Team + makr-code  
**Priorität:** HIGH

---

## Executive Summary

Die aktuelle Implementierung des Documentation Assistants verwendet eine einzelne, vom System bereitgestellte Dokumentationsdatenbank (`docs.db`). Für zukünftige Erweiterungen ist geplant, Administratoren das Einhängen zusätzlicher, nutzungsspezifischer Dokumentationsdatenbanken zu ermöglichen.

**Diese Feature-Erweiterung erfordert eine umfassende Sicherheitsbewertung vor der Implementierung.**

---

## 1. Bedrohungsmodell

### 1.1 Threat Actors

| Actor | Motivation | Capabilities |
|-------|-----------|--------------|
| **Malicious Admin** | Privilege Escalation, Data Exfiltration | High - Full system access |
| **Compromised Admin Account** | Lateral Movement | High - Stolen credentials |
| **Malicious Database Provider** | Code Injection, RCE | Medium - Can craft malicious DB files |
| **Insider Threat** | Data Theft | Medium - Limited system access |

### 1.2 Attack Vectors

#### 1.2.1 Malicious Database Content
**Threat:** Admin hängt Datenbank mit schädlichem Inhalt ein

**Attack Scenarios:**
```yaml
# Beispiel: Datenbank mit Injection-Payloads
additional_databases:
  - name: "malicious_db"
    path: "/tmp/evil.db"
    # Enthält: SQL Injection, XSS, Command Injection Payloads
```

**Potential Impact:**
- ✅ **MITIGATED**: RAG-System verwendet Dokumente nur als Kontext für LLM
- ⚠️ **RISK**: LLM könnte schädliche Anweisungen generieren basierend auf bösartigen Inhalten
- ⚠️ **RISK**: XSS in Web-UI wenn Dokumenteninhalte nicht sanitized werden

**Mitigations:**
1. Content-Validierung beim Laden
2. HTML/Script-Tag-Filtering
3. LLM Output Sanitization
4. CSP (Content Security Policy) in Web-UI

#### 1.2.2 Path Traversal
**Threat:** Admin versucht auf System-Dateien zuzugreifen

```yaml
additional_databases:
  - name: "evil"
    path: "../../../etc/passwd"  # Path Traversal
    path: "/proc/self/mem"        # Memory Access
    path: "file:///etc/shadow"    # URL-based access
```

**Mitigations:**
1. ✅ Pfad-Canonicalization und Whitelist-basierte Validierung
2. ✅ Chroot/Jail für Datenbankzugriff
3. ✅ Dateityp-Validierung (nur .db, .rocksdb erlaubt)
4. ✅ Symlink-Überprüfung

#### 1.2.3 Resource Exhaustion
**Threat:** Admin hängt extrem große Datenbank ein

```yaml
additional_databases:
  - name: "huge_db"
    path: "/tmp/100GB.db"  # DoS through memory exhaustion
```

**Mitigations:**
1. ✅ Maximale Datenbankgröße (config: `max_size_mb`)
2. ✅ Memory-Limits pro Datenbank
3. ✅ Query-Timeouts
4. ✅ Rate-Limiting pro Datenbank

#### 1.2.4 Code Injection via RocksDB
**Threat:** Speziell crafted RocksDB-Datei führt zu Code-Execution

```yaml
additional_databases:
  - name: "evil_rocksdb"
    path: "/tmp/crafted.db"
    # Enthält: Buffer Overflow, Format String, etc.
```

**Mitigations:**
1. ✅ RocksDB Version Pinning (keine alten Versionen mit bekannten Vulns)
2. ✅ Sandboxing (seccomp, AppArmor, SELinux)
3. ✅ Separate Process für jede Datenbank (Isolation)
4. ✅ Memory-safe RocksDB Wrapper

#### 1.2.5 Data Exfiltration
**Threat:** Admin nutzt Dokumentations-DB für Datenleck

```yaml
additional_databases:
  - name: "customer_data_leak"
    path: "/var/lib/themisdb/production_data.db"
    # Enthält: PII, Credentials, Business Secrets
```

**Mitigations:**
1. ✅ Separate Namespace für Docs-DBs
2. ✅ Audit-Logging aller Datenbankzugriffe
3. ✅ RBAC-basierte Zugriffskontrolle
4. ✅ DLP (Data Loss Prevention) Scanning

---

## 2. Sicherheitsanforderungen

### 2.1 Authentifizierung und Autorisierung

**Requirement AUTH-1:** Nur Administratoren dürfen zusätzliche Datenbanken hinzufügen
```yaml
permissions:
  docs_assistant.database.add:
    required_role: administrator
    requires_2fa: true  # Two-Factor Authentication
```

**Requirement AUTH-2:** Digitale Signaturen für Datenbanken
```yaml
additional_databases:
  - name: "legal_docs"
    path: "/var/lib/themisdb/legal.db"
    signature: "RSA-SHA256:abc123..."
    signed_by: "security@example.com"
    signature_timestamp: "2026-01-11T10:00:00Z"
```

**Requirement AUTH-3:** Datenbank-Provenance-Tracking
```yaml
database_metadata:
  created_by: "admin@example.com"
  created_at: "2026-01-11T09:00:00Z"
  source: "https://docs.example.com/legal_v1.2.db"
  checksum_sha256: "abc123..."
  approved_by: "security@example.com"
  approval_date: "2026-01-11T10:00:00Z"
```

### 2.2 Datenbank-Validierung

**Requirement VAL-1:** Schema-Validierung
```cpp
bool validateDatabaseSchema(const std::string& db_path) {
    // Prüfe Column Families
    std::vector<std::string> expected_cfs = {
        "default", "relational", "graph_nodes", 
        "graph_edges", "vector", "metadata", "document"
    };
    
    // Prüfe Metadaten
    auto metadata = getMetadata(db_path);
    if (metadata["version"] != "1.0") return false;
    if (metadata["type"] != "documentation") return false;
    
    return true;
}
```

**Requirement VAL-2:** Content-Filtering
```cpp
bool validateDocumentContent(const json& doc) {
    // Keine Scripts
    if (containsScriptTags(doc["content"])) return false;
    
    // Keine Binary Data (außer Embeddings)
    if (containsBinaryData(doc["content"])) return false;
    
    // Keine URLs zu internen Systemen
    if (containsInternalURLs(doc["content"])) return false;
    
    // Maximale Dokumentgröße
    if (doc["content"].size() > MAX_DOC_SIZE) return false;
    
    return true;
}
```

**Requirement VAL-3:** Malware-Scanning
```bash
# Integration mit ClamAV oder VirusTotal
clamscan --recursive /var/lib/themisdb/custom_docs.db
```

### 2.3 Isolation und Sandboxing

**Requirement ISO-1:** Separate RocksDB-Instanzen
```cpp
// Jede Datenbank in separater RocksDB-Instanz
class DocsDatabase {
    std::unique_ptr<rocksdb::DB> db_;
    SecurityContext security_ctx_;
    ResourceLimits limits_;
};

std::map<std::string, std::unique_ptr<DocsDatabase>> databases_;
databases_["system"] = createSystemDatabase();
databases_["custom_legal"] = createCustomDatabase("legal.db");
```

**Requirement ISO-2:** Process-Isolation (optional)
```cpp
// Jede Datenbank in separatem Prozess via Fork
pid_t db_process = fork();
if (db_process == 0) {
    // Child process: Load and query database
    setupSeccompFilter();  // Limit syscalls
    dropPrivileges();       // Drop root
    loadDatabase("/var/lib/themisdb/custom.db");
    serveQueries();
    exit(0);
}
```

**Requirement ISO-3:** seccomp/AppArmor/SELinux
```bash
# AppArmor Profile für ThemisDB Docs Assistant
profile themisdb-docs {
  # Erlaubt: Lesen von Datenbanken
  /var/lib/themisdb/**.db r,
  
  # Verboten: Schreiben außerhalb data/
  deny /** w,
  
  # Verboten: Netzwerkzugriff
  deny network,
  
  # Erlaubt: Logging
  /var/log/themisdb/** w,
}
```

### 2.4 Audit und Monitoring

**Requirement AUD-1:** Vollständiger Audit-Trail
```json
{
  "event": "database_added",
  "timestamp": "2026-01-11T10:00:00Z",
  "user": "admin@example.com",
  "user_ip": "192.168.1.100",
  "database_name": "custom_legal",
  "database_path": "/var/lib/themisdb/legal.db",
  "database_checksum": "sha256:abc123...",
  "signature_valid": true,
  "approved_by": "security@example.com"
}
```

**Requirement AUD-2:** Query-Auditing
```json
{
  "event": "docs_query",
  "timestamp": "2026-01-11T10:05:00Z",
  "user": "user@example.com",
  "database": "custom_legal",
  "query": "What are the GDPR requirements?",
  "documents_accessed": [
    "legal.db:doc:gdpr_article_5",
    "legal.db:doc:gdpr_article_6"
  ],
  "response_generated": true
}
```

**Requirement AUD-3:** Anomalie-Detektion
```python
# Beispiel: Anomalie-Detektion-Regeln
rules = [
    # Zu viele Queries in kurzer Zeit
    {"condition": "query_rate > 1000/min", "alert": "potential_data_exfiltration"},
    
    # Queries von ungewöhnlicher IP
    {"condition": "source_ip not in whitelist", "alert": "suspicious_access"},
    
    # Zugriff auf viele verschiedene Dokumente
    {"condition": "unique_docs_accessed > 500/hour", "alert": "potential_scraping"},
]
```

### 2.5 Rate-Limiting

**Requirement RATE-1:** Pro-Datenbank Limits
```yaml
rate_limiting:
  per_database:
    system_docs:
      requests_per_minute: 100
    custom_legal:
      requests_per_minute: 50  # Niedriger für custom DBs
    custom_internal:
      requests_per_minute: 20
      
  global:
    requests_per_minute: 150  # Über alle DBs
    max_concurrent_queries: 10
```

**Requirement RATE-2:** Adaptive Rate-Limiting
```cpp
// Reduziere Limits bei verdächtigem Verhalten
if (detectAnomalousPattern(user)) {
    user_limits[user].requests_per_minute /= 2;
    logSecurityEvent("rate_limit_reduced", user);
}
```

---

## 3. Implementierungs-Roadmap

### Phase 1: Grundlegende Sicherheit (2-3 Wochen)
- [ ] Pfad-Validierung und Canonicalization
- [ ] Dateityp-Prüfung (nur .db, .rocksdb)
- [ ] Größenlimits (max_size_mb)
- [ ] Read-Only-Modus erzwingen
- [ ] RBAC-Integration (nur Admin kann DBs hinzufügen)

### Phase 2: Validierung und Filtering (3-4 Wochen)
- [ ] Schema-Validierung
- [ ] Content-Filtering (XSS, Scripts)
- [ ] Checksum-Validierung
- [ ] Malware-Scanning-Integration

### Phase 3: Isolation und Sandboxing (4-5 Wochen)
- [ ] Separate RocksDB-Instanzen pro Datenbank
- [ ] seccomp-Filter
- [ ] AppArmor/SELinux Profile
- [ ] Optional: Process-Isolation

### Phase 4: Audit und Monitoring (2-3 Wochen)
- [ ] Audit-Logging für alle DB-Operationen
- [ ] Query-Auditing
- [ ] Anomalie-Detektion
- [ ] Prometheus-Metriken

### Phase 5: Erweiterte Features (3-4 Wochen)
- [ ] Digitale Signaturen
- [ ] Provenance-Tracking
- [ ] Datenbank-Approval-Workflow
- [ ] Automated Security Scanning

**Total Estimated Time: 14-19 Wochen**

---

## 4. Security Testing

### 4.1 Penetration Testing
```bash
# Test 1: Path Traversal
curl -X POST /api/v1/admin/docs/database/add \
  -d '{"name": "evil", "path": "../../../etc/passwd"}'

# Test 2: Large Database
curl -X POST /api/v1/admin/docs/database/add \
  -d '{"name": "huge", "path": "/tmp/10GB.db"}'

# Test 3: Malicious Content
curl -X POST /api/v1/admin/docs/database/add \
  -d '{"name": "xss", "path": "/tmp/xss_payload.db"}'

# Test 4: Resource Exhaustion
for i in {1..10000}; do
  curl -X POST /api/v1/llm/docs/query \
    -d '{"database": "custom", "query": "test"}'
done
```

### 4.2 Fuzzing
```bash
# Fuzzing der Datenbankdateien
AFL++ -i corpus/ -o findings/ ./themis_server --docs-db @@

# Fuzzing der API-Endpoints
ffuf -w payloads.txt -u http://localhost:8765/api/v1/llm/docs/FUZZ
```

### 4.3 Static Analysis
```bash
# Cppcheck
cppcheck --enable=all --inconclusive src/llm/docs_assistant.cpp

# Clang Static Analyzer
scan-build cmake --build build

# SonarQube
sonar-scanner -Dsonar.projectKey=themisdb-docs-assistant
```

---

## 5. Compliance und Regulations

### 5.1 GDPR (EU)
- **Art. 25**: Privacy by Design
  - Minimierung der Daten in Dokumentationsdatenbanken
  - Keine PII in Audit-Logs
  
- **Art. 32**: Security of Processing
  - Verschlüsselung at-rest und in-transit
  - Zugriffskontrolle
  - Audit-Logging

### 5.2 SOC 2
- **CC6.1**: Logical and Physical Access Controls
  - RBAC für Datenbankzugriff
  - 2FA für Admin-Operationen
  
- **CC7.2**: System Monitoring
  - Anomalie-Detektion
  - Audit-Logging
  - Alert-System

### 5.3 ISO 27001
- **A.9**: Access Control
- **A.12**: Operations Security
- **A.14**: System Acquisition, Development and Maintenance

---

## 6. Offene Fragen

### 6.1 Architektur-Entscheidungen
- [ ] **Q1**: Separate Prozesse für jede Datenbank oder shared process?
  - **Pro Separate**: Bessere Isolation
  - **Con Separate**: Höherer Overhead
  
- [ ] **Q2**: Synchrones oder asynchrones Laden von Datenbanken?
  - **Pro Async**: Kein Blocking des Main-Threads
  - **Con Async**: Komplexere Fehlerbehandlung

- [ ] **Q3**: Hot-Reload von Datenbanken oder Server-Neustart erforderlich?
  - **Pro Hot-Reload**: Keine Downtime
  - **Con Hot-Reload**: Komplexere Implementierung

### 6.2 Policy-Fragen
- [ ] **Q4**: Wer darf Datenbanken approven?
  - Option A: Nur Security Team
  - Option B: Admin + Security Team
  
- [ ] **Q5**: Wie lange werden Audit-Logs aufbewahrt?
  - Empfehlung: 90 Tage (GDPR-konform)
  
- [ ] **Q6**: Automatisches Entfernen von verdächtigen Datenbanken?
  - Option A: Automatisch bei Anomalie-Detektion
  - Option B: Manuelle Intervention erforderlich

---

## 7. Ressourcen und Referenzen

### 7.1 Standards
- **OWASP Top 10** - https://owasp.org/www-project-top-ten/
- **CWE Top 25** - https://cwe.mitre.org/top25/
- **NIST Cybersecurity Framework** - https://www.nist.gov/cyberframework

### 7.2 RocksDB Security
- **RocksDB Security Model** - https://github.com/facebook/rocksdb/wiki/Security
- **RocksDB Known Issues** - https://github.com/facebook/rocksdb/security/advisories

### 7.3 LLM Security
- **OWASP LLM Top 10** - https://owasp.org/www-project-top-10-for-large-language-model-applications/
- **Prompt Injection Attacks** - https://simonwillison.net/2023/Apr/14/worst-that-can-happen/

---

## 8. Action Items

**Immediate (vor Implementierung):**
1. [ ] Security Review Meeting einberufen
2. [ ] Threat Modeling Workshop
3. [ ] Genehmigung von Management einholen
4. [ ] Budget für Security Testing zuweisen

**Short-term (0-3 Monate):**
1. [ ] Phase 1 Implementierung (Basis-Sicherheit)
2. [ ] Erste Penetration Tests
3. [ ] Dokumentation aktualisieren

**Mid-term (3-6 Monate):**
1. [ ] Phase 2-4 Implementierung
2. [ ] Compliance-Audit
3. [ ] Security Training für Admins

**Long-term (6-12 Monate):**
1. [ ] Phase 5 Implementierung
2. [ ] Jährliches Security Assessment
3. [ ] Bug Bounty Program

---

**Nächste Schritte:**
1. Review dieses Dokuments durch Security Team
2. Priorisierung der Anforderungen
3. Entscheidung: Go/No-Go für zusätzliche Datenbanken Feature
4. Falls Go: Implementierungs-Timeline finalisieren

**Ansprechpartner:**
- **Security Lead**: TBD
- **Development Lead**: makr-code
- **Product Owner**: TBD
