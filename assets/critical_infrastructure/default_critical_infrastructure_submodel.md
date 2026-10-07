# default_critical_infrastructure_submodel.yaml — Detailerklärung

## Zweck
Submodel für Eskalations- und Ausnahmebehandlung in `critical_infrastructure`-Prozessen.

## Prozessbild
```mermaid
flowchart LR
  A[Escalation Intake] --> B[Deep Validation]
  B --> C[Override Decision]
  C --> D[Audit Record]
```
