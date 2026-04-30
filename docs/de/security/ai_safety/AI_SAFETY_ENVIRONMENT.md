# 🌍 Environment Isolation Guard — Schicht 6

> **Konfiguration:** `config/security.yaml` — `environment:`-Block
>
> Verhindert, dass KI-Agenten in Produktionsumgebungen dieselben Rechte haben wie
> in Entwicklungs- oder Stagingumgebungen.

---

## Übersicht

Eine der Hauptursachen des Cursor-Vorfalls war das Fehlen einer
**Umgebungsunterscheidung**: Der KI-Agent hatte in der Produktionsdatenbank
dieselben Rechte wie in der Entwicklungsumgebung.

Der **Environment Isolation Guard** implementiert umgebungsspezifische Restriktionen
für alle KI-initiierten Operationen.

---

## Umgebungsklassen

| Umgebung | `block_destructive` Default | Approval | CRITICAL Hard-Block |
|---|---|---|---|
| `production` | `true` | Immer erforderlich | Ja (außer Sonderrolle) |
| `staging` | `false` | Bei `CRITICAL` | Nein (mit Approval) |
| `development` | `false` | Nein | Nein |

---

## Konfigurationsschema

```yaml
# config/security.yaml

environment:
  # Umgebungsname: production | staging | development
  # Wirkt auf alle KI-Agent-Operationen via MCP und AI Orchestrator
  name: production

  ai_agent_restrictions:
    # Hard-Block für alle DESTRUCTIVE-Operationen (kein Approval möglich)
    block_destructive: true

    # Approval auch für WRITE_SAFE (unterhalb DESTRUCTIVE) erzwingen
    require_approval: true

    # Whitelist für erlaubte Collections (leer = alle erlaubt, wird durch denied eingeschränkt)
    allowed_collections: []

    # Blacklist: KI-Agent kann diese Collections NIEMALS verändern
    # Auch nicht mit Approval. Hard-Block.
    denied_collections:
      - users
      - audit_log
      - billing
      - _system
      - _graphs

    # RBAC-Rolle die CRITICAL-Operationen in Produktion erlaubt
    # Wenn nicht gesetzt: CRITICAL ist in production IMMER blockiert
    require_role_for_critical: AI_DESTRUCTIVE_PRODUCTION_OPS

    # Maximale Anzahl KI-Operationen pro Stunde (Rate-Limiting)
    max_operations_per_hour: 100

    # Maximale Anzahl gleichzeitig wartender Approvals
    max_pending_approvals: 10
```

---

## Entscheidungsmatrix

```
KI-Operation kommt an
│
├── Umgebung = production UND block_destructive = true?
│   └── Op-Klasse ≥ DESTRUCTIVE?
│       ├── Ja: target_collection in denied_collections?
│       │   └── Ja → HARD BLOCK (kein Approval möglich)
│       │   └── Nein: Op-Klasse = CRITICAL?
│       │       ├── Ja: Caller hat Rolle AI_DESTRUCTIVE_PRODUCTION_OPS?
│       │       │   ├── Ja → HILG (Approval mit CRITICAL-Level)
│       │       │   └── Nein → HARD BLOCK
│       │       └── Nein (DESTRUCTIVE) → HILG (Approval)
│       └── Nein (READ_ONLY, WRITE_SAFE) → Ausführen
│
├── Umgebung = staging?
│   └── Op-Klasse = CRITICAL → HILG (Approval)
│   └── Sonst → Ausführen (oder je nach Mode-Config)
│
└── Umgebung = development?
    └── Ausführen (kein automatisches Approval)
```

---

## Denied Collections — Verhalten

Operationen auf `denied_collections` werden **immer** hardblockt, auch:
- wenn der Operator eine Approval erteilt hat
- wenn der Caller die Rolle `AI_DESTRUCTIVE_PRODUCTION_OPS` hat
- wenn die Operation als `READ_ONLY` klassifiziert ist (Ausnahme: Lesezugriff ist erlaubt)

Schreib- und Löschoperationen auf denied Collections sind ausnahmslos blockiert.

---

## RBAC-Rolle: `AI_DESTRUCTIVE_PRODUCTION_OPS`

Diese Rolle erlaubt `CRITICAL`-Operationen in der Produktionsumgebung (nach Approval).
Sie sollte:

- nur einer minimalen Anzahl Service-Accounts zugewiesen werden (DBA-Team)
- in der bestehenden RBAC-Konfiguration (`config/rbac.yaml`) definiert werden
- über den `AuditLogger` bei jeder Zuweisung/Entziehung protokolliert werden
- **niemals** einem KI-Agenten direkt zugewiesen werden (nur menschlichen Approvers)

```yaml
# config/rbac.yaml (Beispiel)
roles:
  - name: AI_DESTRUCTIVE_PRODUCTION_OPS
    description: "Erlaubt CRITICAL KI-Operationen in Produktion nach Approval"
    permissions:
      - ai_critical_approve
      - ai_rollback
    max_sessions: 2
    require_mfa: true
    audit_all_actions: true
```

---

## Umgebungserkennung

Die Umgebung wird aus `config/security.yaml` geladen. Für Container-Deployments
kann sie über eine Umgebungsvariable überschrieben werden:

```bash
# Umgebungsvariable (höhere Priorität als YAML)
THEMIS_ENVIRONMENT=production

# Kubernetes ConfigMap
env:
  - name: THEMIS_ENVIRONMENT
    valueFrom:
      configMapKeyRef:
        name: themis-config
        key: environment
```

**Sicherheitshinweis:** Die Umgebungsvariable darf nicht durch KI-Agenten
gesetzt oder verändert werden können.

---

## Metriken

```
themis_ai_env_blocked_total{env="production", reason="denied_collection"}
themis_ai_env_blocked_total{env="production", reason="missing_role"}
themis_ai_env_blocked_total{env="production", reason="block_destructive"}
themis_ai_rate_limit_exceeded_total{env="production"}
```

---

## Testfälle (Geplant: `tests/security/ai_safety/test_ai_environment_guard.cpp`)

| Test-ID | Szenario | Erwartetes Ergebnis |
|---|---|---|
| ENV-01 | DELETE in production, block_destructive=true | HARD BLOCK |
| ENV-02 | DELETE in development, block_destructive=false | Approval (per Mode-Config) |
| ENV-03 | CRITICAL in production, ohne Rolle | HARD BLOCK |
| ENV-04 | CRITICAL in production, mit Rolle | HILG Approval-Flow |
| ENV-05 | Operation auf `denied_collection: users` | HARD BLOCK |
| ENV-06 | READ auf `denied_collection: users` | Erlaubt |
| ENV-07 | Rate-Limit überschritten | `429 Too Many Requests` |
| ENV-08 | staging: CRITICAL → Approval | HILG ohne Rollenanforderung |

---

## Roadmap-Verknüpfung

- **ASL-9:** Environment-Konfiguration laden und auswerten → Q3 2026 (Phase 3)
