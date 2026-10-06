# default_legal_process.yaml — Detailerklärung

## Zweck
Standardisierter Rechtsprüfungsprozess mit sauberer Trennung zwischen Tatsachen und Normauslegung.

## Prozessbild
```mermaid
flowchart LR
  A[Norms] --> B[Facts]
  B --> C[Subsumption]
  C --> D[Conflict Check]
  D --> E[Legal Memo]
```
