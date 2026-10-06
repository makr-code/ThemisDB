# default_military_process.yaml — Detailerklärung

## Zweck
Prozessmodell für militärische Missionsplanung und kontrollierte Durchführung mit Review-Schleife.

## Prozessbild
```mermaid
flowchart LR
  A[Mission Analysis] --> B[COA]
  B --> C[Risk/Rules]
  C --> D[Execution]
  D --> E[AAR]
```
