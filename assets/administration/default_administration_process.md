# default_administration_process.yaml — Detailerklärung

## Zweck
Default-BPMN-Modell für die Gruppe `administration` mit ThemisDB-Governance und Gefährdungsanalyse.

## Prozessbild
```mermaid
flowchart LR
  A[Intake] --> B[Assessment]
  B --> C[Validation]
  C --> D[Decision]
  D --> E[Monitoring]
```
