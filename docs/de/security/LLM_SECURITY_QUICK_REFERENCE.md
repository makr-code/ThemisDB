# LLM & LoRa Security - Quick Reference

**Version:** 1.0  
**Datum:** 7. Januar 2026  
**Zielgruppe:** Entwickler & DevOps

---

## 🚀 Quick Start

> **⚠️ WICHTIGER HINWEIS ZUM PRODUKTIONSEINSATZ**
> 
> Die LoRa-Signaturvalidierung ist derzeit als **Prototyp/Konzept** implementiert
> und verwendet Stub-Code. **NICHT für Produktionsumgebungen geeignet** ohne:
> 
> - Vollständige OpenSSL X.509 Signaturverifikation
> - LoRa-Dateiformat-Parsing
> - Security-Audit
> 
> Die Prompt Injection Detection und Embedding Anomaly Detection sind
> produktionsreif und können sofort eingesetzt werden.

### 1. LoRa-Adapter-Sicherheit aktivieren

```cpp
#include "llm/lora_security_validator.h"
#include "llm/multi_lora_manager.h"

using namespace themis::llm;

// Configure security
LoRASecurityConfig security_config;
security_config.require_signature = true;
security_config.verify_checksum = true;
security_config.detect_weight_anomalies = true;

// Add trusted signers
security_config.trusted_signers = {
    "a1b2c3d4...",  // SHA-256 fingerprint of trusted cert
    "e5f6g7h8..."
};

// Create validator
LoRASecurityValidator validator(security_config);

// Validate adapter before loading
auto sig_result = validator.verifyEmbeddedSignature("legal-qa.lora");
if (!sig_result.is_valid) {
    LOG_ERROR("LoRa signature invalid: {}", sig_result.error_message);
    return false;
}

auto integrity_result = validator.checkIntegrity("legal-qa.lora");
if (!integrity_result.is_intact) {
    LOG_ERROR("LoRa integrity check failed");
    return false;
}

// Safe to load
MultiLoRAManager lora_manager(config);
lora_manager.loadLoRA("legal-qa", "legal-qa.lora", "mistral-7b");
```

### 2. Prompt Injection Detection

```cpp
#include "llm/lora_security_validator.h"

// Configure detector
PromptInjectionDetector::Config detector_config;
detector_config.enabled = true;
detector_config.risk_threshold = 0.7f;
detector_config.block_high_risk = true;

PromptInjectionDetector detector(detector_config);

// Check user input
std::string user_prompt = get_user_input();

if (detector.isSuspicious(user_prompt)) {
    LOG_WARN("Suspicious prompt detected");
    
    // Option 1: Block
    return error_response("Prompt appears malicious");
    
    // Option 2: Sanitize
    user_prompt = detector.sanitizePrompt(user_prompt);
}

// Safe to process
process_llm_request(user_prompt);
```

### 3. Embedding Anomaly Detection

```cpp
#include "llm/lora_security_validator.h"

// Configure detector
EmbeddingAnomalyDetector::Config anomaly_config;
anomaly_config.enabled = true;
anomaly_config.outlier_threshold = 3.0f;
anomaly_config.min_samples = 100;

EmbeddingAnomalyDetector anomaly_detector(anomaly_config);

// Build baseline with normal embeddings
for (const auto& doc : corpus) {
    auto embedding = generate_embedding(doc);
    anomaly_detector.updateBaseline(embedding);
}

// Check new embedding
auto new_embedding = generate_embedding(new_document);
float anomaly_score = anomaly_detector.getAnomalyScore(new_embedding);

if (anomaly_score > 0.8f) {
    LOG_WARN("Anomalous embedding detected: score={}", anomaly_score);
    // Reject or quarantine
}
```

---

## 🛡️ Security Checklist

### LoRa-Adapter-Sicherheit

- [ ] **Signaturvalidierung aktiviert** (`require_signature = true`)
- [ ] **Trusted Signers konfiguriert** (mindestens 1 Zertifikat)
- [ ] **Checksummen-Verifikation** (`verify_checksum = true`)
- [ ] **Anomalie-Detection für Weights** (`detect_weight_anomalies = true`)
- [ ] **Metadata-Validierung** (base_model, rank, size)
- [ ] **Audit-Logging für Adapter-Operationen**

### Prompt-Sicherheit

- [ ] **Prompt Injection Detection aktiviert**
- [ ] **Risk-Threshold konfiguriert** (empfohlen: 0.7)
- [ ] **Sanitization bei verdächtigen Prompts**
- [ ] **Logging aller Detection-Events**
- [ ] **Rate-Limiting für verdächtige Benutzer**

### Embedding-Sicherheit

- [ ] **Vector Encryption aktiviert** (siehe [VECTOR_ENCRYPTION_CONFIGURATION.md](VECTOR_ENCRYPTION_CONFIGURATION.md))
- [ ] **HNSW Encryption aktiviert** (siehe [HNSW_ENCRYPTION_CONFIGURATION.md](HNSW_ENCRYPTION_CONFIGURATION.md))
- [ ] **Anomalie-Detection für Embeddings**
- [ ] **Baseline regelmäßig aktualisieren**
- [ ] **Threshold-basierte Alerts**

---

## 📊 Monitoring

### Grafana Metriken

```cpp
// In src/llm/grafana_metrics.cpp:

// LoRa Security Metrics
grafana_metrics::lora_signature_failures.inc();
grafana_metrics::lora_integrity_check_failures.inc();
grafana_metrics::lora_anomalies_detected.inc();

// Prompt Injection Metrics
grafana_metrics::prompt_injection_detections.inc();
grafana_metrics::prompt_injection_risk_score.set(risk_score);
grafana_metrics::prompt_injection_blocks.inc();

// Embedding Anomaly Metrics
grafana_metrics::embedding_anomaly_score.set(anomaly_score);
grafana_metrics::embedding_anomalies_detected.inc();
```

### Alert-Regeln

```yaml
# In grafana/alerts.yaml:

- alert: LoRASignatureFailure
  expr: lora_signature_failures > 0
  for: 1m
  severity: critical
  annotations:
    summary: "LoRa adapter signature validation failed"

- alert: HighPromptInjectionRate
  expr: rate(prompt_injection_detections[5m]) > 0.1
  for: 5m
  severity: warning
  annotations:
    summary: "High rate of prompt injection attempts"

- alert: EmbeddingAnomalyDetected
  expr: embedding_anomaly_score > 0.9
  for: 1m
  severity: warning
  annotations:
    summary: "Highly anomalous embedding detected"
```

---

## 🔧 Configuration

### config/llm_config.yaml

```yaml
llm:
  security:
    # LoRa Adapter Security
    lora_signature_required: true
    lora_trusted_signers:
      - "a1b2c3d4e5f6..."  # Cert fingerprints
      - "f6g7h8i9j0k1..."
    lora_checksum_verification: true
    lora_anomaly_detection: true
    lora_anomaly_threshold: 3.0
    
    # Prompt Injection Detection
    prompt_injection_enabled: true
    prompt_injection_threshold: 0.7
    prompt_injection_block_high_risk: false  # Warn only
    prompt_injection_log_detections: true
    
    # Embedding Anomaly Detection
    embedding_anomaly_enabled: true
    embedding_anomaly_threshold: 3.0
    embedding_min_baseline_samples: 100
```

### Environment Variables

```bash
# LoRa Security
export THEMIS_LORA_REQUIRE_SIGNATURE=true
export THEMIS_LORA_TRUSTED_SIGNERS="cert1.pem,cert2.pem"

# Prompt Security
export THEMIS_PROMPT_INJECTION_DETECTION=true
export THEMIS_PROMPT_INJECTION_THRESHOLD=0.7

# Embedding Security
export THEMIS_EMBEDDING_ANOMALY_DETECTION=true
```

---

## 🧪 Testing

### Unit Tests

```bash
# Run security tests
cd /home/runner/work/ThemisDB/ThemisDB
./build/tests/test_lora_security

# Expected output:
# [==========] Running 20 tests from 5 test suites.
# [----------] 6 tests from LoRASecurityTest
# [ RUN      ] LoRASecurityTest.CalculateChecksum
# [       OK ] LoRASecurityTest.CalculateChecksum (1 ms)
# ...
# [==========] 20 tests from 5 test suites ran. (234 ms total)
# [  PASSED  ] 20 tests.
```

### Integration Tests

```bash
# Test with real LoRa adapter
./scripts/test_lora_security.sh --adapter ./models/legal-qa.lora

# Test prompt injection detection
./scripts/test_prompt_injection.sh --prompts ./test_data/malicious_prompts.txt

# Test embedding anomalies
./scripts/test_embedding_anomaly.sh --corpus ./test_data/corpus.jsonl
```

---

## 🚨 Incident Response

### 1. LoRa Signature Validation Failed

**Symptom:** `lora_signature_failures` metric increased

**Schritte:**
1. Identifiziere betroffenen Adapter: Check Audit-Logs
2. Verifiziere Signatur manuell mit OpenSSL
3. Prüfe ob Adapter manipuliert wurde (Checksum)
4. Entferne verdächtigen Adapter aus Produktion
5. Informiere Security-Team

### 2. High Rate of Prompt Injection Attempts

**Symptom:** `prompt_injection_detections` steigt plötzlich

**Schritte:**
1. Identifiziere Benutzer/IP-Adresse
2. Aktiviere Rate-Limiting für Benutzer
3. Prüfe ob koordinierter Angriff
4. Ggf. temporär blockieren
5. Analyse der Prompts für neue Patterns

### 3. Embedding Anomaly Alert

**Symptom:** `embedding_anomaly_score > 0.9`

**Schritte:**
1. Identifiziere Dokument/Vektor-ID
2. Quarantäne des verdächtigen Embeddings
3. Prüfe auf Poisoning-Angriff
4. Update Baseline falls False Positive
5. Überprüfe Embedding-Modell

---

## 📚 Weitere Ressourcen

- **[LLM_LORA_ATTACK_VECTORS.md](LLM_LORA_ATTACK_VECTORS.md)** - Vollständige Bedrohungsanalyse
- **[security_threat_model.md](security_threat_model.md)** - Allgemeines Threat Model
- **[Multi-LoRa Manager](../../include/llm/multi_lora_manager.h)** - LoRa-Verwaltung
- **[Security Validator](../../include/llm/lora_security_validator.h)** - Sicherheitsvalidierung

---

## 🆘 Support

Bei Fragen oder Problemen:
- **Issues:** https://github.com/makr-code/ThemisDB/issues
- **Slack:** #security-channel
- **Email:** security@themisdb.com
