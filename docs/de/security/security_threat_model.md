# Threat Model (light)

**Stand:** 5. Dezember 2025  
**Version:** v1.3.0  
**Kategorie:** 🔒 Security

---

## 📑 Inhaltsverzeichnis

- [Assets](#assets)
- [Akteure](#akteure)
- [Vertrauensgrenzen](#vertrauensgrenzen)
- [Hauptrisiken (Auszug)](#hauptrisiken-auszug)
- [Gegenmaßnahmen](#gegenmaßnahmen)
- [Beobachtbarkeit](#beobachtbarkeit)
- [Weiterlesen](#weiterlesen)

---


Ziel: Risiken sichtbar machen und mit pragmatischen Kontrollen adressieren.

## Assets
- Datenbankinhalte (Dokumente, Inhalte, Vektoren, Zeitreihen)
- Schlüsselmaterial (LEK/KEK/DEK)
- Audit‑Trails (Changefeed)

## Akteure
- Admin/Operator (berechtigt)
- Anwendung/Service (technisch)
- Angreifer extern/intern (unberechtigt/teilberechtigt)

## Vertrauensgrenzen
- Client ↔ Reverse‑Proxy ↔ Themis‑Server ↔ Storage (RocksDB)
- Externe Schlüsselverwaltung (Vault o. ä.)

## Hauptrisiken (Auszug)
- Unautorisierte Schlüsselrotation/Schlüsselabgriff
- Datenexfiltration über Admin‑APIs
- PII‑Leakage in Logs/Exports
- Manipulation Audit‑Trail

### LLM & LoRa-spezifische Risiken (v1.4.0+)
- **Prompt Injection:** Manipulation von System-Prompts oder Benutzer-Eingaben
- **LoRa Model Poisoning:** Vergiftete Adapter mit Backdoors
- **Vector Embedding Manipulation:** Manipulation von HNSW-Indizes für RAG
- **Adapter Weight Extraction:** Seitenkanal-Angriffe auf LoRa-Weights
- **Multi-LoRa Adversarial Switching:** Ausnutzung von Adapter-Batching
- Siehe: [LLM_LORA_ATTACK_VECTORS.md](LLM_LORA_ATTACK_VECTORS.md)

## Gegenmaßnahmen
- RBAC/Netzwerk‑Kontrollen vor Admin‑APIs (/keys/rotate, /changefeed/retention)
- TLS‑Terminations‑Proxy, mTLS optional
- Secrets‑Management (kein Klartext im Repo/Config)
- Minimierte Logs; Pseudonymisierung sensibler Werte
- Regelmäßige Rotation, Least‑Privilege, Vier‑Augen‑Prinzip bei kritischen Aktionen
- Backup/Restore mit Integritätsprüfungen

### LLM & LoRa-Schutzmaßnahmen (v1.4.0+)
- **LoRa Signature Verification:** Signaturvalidierung für Adapter (`LoRASecurityValidator`)
- **Prompt Injection Detection:** Pattern-basierte Erkennung (`PromptInjectionDetector`)
- **Embedding Anomaly Detection:** Statistische Anomalieerkennung (`EmbeddingAnomalyDetector`)
- **Vector Encryption:** At-Rest-Verschlüsselung für HNSW (Phase 1+2 ✅)
- **Adapter Integrity Checks:** Checksum-Validierung und Weight-Anomalie-Detection
- Siehe: Implementierung in `include/llm/lora_security_validator.h`

## Beobachtbarkeit
- Health/Metrics Endpunkte überwachen (/health, /metrics)
- Alarme für Schlüsselablauf, Retention‑Fehler, Anomalien im Changefeed

Weiterlesen:
- security/key_management.md, security/audit_and_retention.md
- encryption_strategy.md, security_hardening_guide.md, security_audit_checklist.md
