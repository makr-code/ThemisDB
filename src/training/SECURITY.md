> **Sicherheitshinweis:** Security-Angaben gegen aktuelle Build-Flags, Codepfade und Tests validieren.

<!-- Status: current | validated: 2026-04-06 -->
# Security — Training Module
> Report vulnerabilities via [SECURITY.md](../../../SECURITY.md).

## Threat Model

| Threat | Mitigation |
|--------|-----------|
| Model poisoning via malicious training data | Input validation and sanitization on training corpus |
| Checkpoint tampering | Checkpoint files include SHA-256 integrity hash |
| Training data exfiltration | Training data access requires elevated privileges |
| Model extraction via adapter weights | Adapter weights protected by storage-level encryption |
| Resource exhaustion (GPU DoS) | Training jobs subject to resource quotas and GPU time limits |

## Security Controls
- Training job execution requires authentication and authorization
- Model checkpoints stored with encryption at rest
- Training data access logged via `AuditLogger`
- Gradient clipping prevents NaN/Inf propagation from adversarial inputs

## Known Limitations
- Differential privacy for training not yet implemented
