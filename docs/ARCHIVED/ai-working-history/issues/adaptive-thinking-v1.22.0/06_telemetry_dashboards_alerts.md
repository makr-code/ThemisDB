## Summary

Add observability for adaptive decisions, escalation behavior, latency, and token usage.

## Deliverables

- Prometheus metrics for adaptive flow
- Trace attributes for policy/scorer decisions
- Dashboard panels and alert thresholds

## Tasks

- [ ] Add metrics for requests/escalations/latency/tokens
- [ ] Add structured log fields and trace attributes
- [ ] Add dashboard JSON/panels for adaptive KPIs
- [ ] Add baseline alert thresholds and runbook links

## Acceptance Criteria

- Metrics visible in existing scrape pipeline
- Decision context available for request diagnosis
- Dashboards show pre/post adaptive deltas

## Labels

- type:feature
- area:observability
- area:llm
- priority:P1
- effort:medium
