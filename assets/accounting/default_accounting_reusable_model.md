# default_accounting_reusable_model.yaml — Detailerklärung

## Zweck
Reusable-Modell mit wiederverwendbaren BPMN-Bausteinen für `accounting`.

## Prozessbild
```mermaid
flowchart LR
  A[Evidence Bundle] --> B[Risk Gate]
  B --> C[Approval Gate]
  C --> D[Rollback Gate]
```
