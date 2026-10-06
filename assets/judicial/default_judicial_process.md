# default_judicial_process.yaml — Detailerklärung

## Zweck
Prozessbild für judikative Entscheidungsfindung mit Anhörung, Beweiswürdigung und begründetem Urteil.

## Prozessbild
```mermaid
flowchart LR
  A[Admissibility] --> B[Hearing]
  B --> C[Evidence Weighing]
  C --> D[Legal Assessment]
  D --> E[Judgment]
```
