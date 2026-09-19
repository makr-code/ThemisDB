# ThemisDB Deploy Artefacts

This directory contains deployment artefacts for ThemisDB. Content is
organised by deployment target:

| Subdirectory  | Edition(s)                   | Description                     |
|---------------|------------------------------|---------------------------------|
| `kubernetes/` | Enterprise + Hyperscaler     | Kubernetes Operator, CRDs, manifests, Helm-ready examples |
| `systemd/`    | All (primarily Community)    | systemd service unit files      |

## Edition Mapping

### `deploy/kubernetes/` — Enterprise and Hyperscaler

The Kubernetes Operator and associated manifests (`deploy/kubernetes/`) are
intended for **Enterprise** and **Hyperscaler** deployments:

- **Enterprise:** Single-cluster Kubernetes deployments with HA replication.
- **Hyperscaler:** Multi-cluster, geo-distributed, with sharding and the
  full Operator reconciliation loop.

> **Branch policy:** Changes to `deploy/**` are **blocked from the `main`
> branch** (Community release lane) by the PR path gate at
> `.github/workflows/pr-path-gate-main.yml`.
> Kubernetes artefacts belong in the `enterprise` or `hyperscaler` release
> lanes.

### `deploy/systemd/` — Community (and higher)

The systemd unit files are primarily intended for **Community Edition**
single-node deployments. They are also used by Enterprise single-node setups.

## Subdirectory structure (planned)

As the Enterprise and Hyperscaler release lanes mature, this directory may
be reorganised into edition-explicit subdirectories:

```
deploy/
  kubernetes/          ← current (Enterprise + Hyperscaler)
  systemd/             ← current (Community + Enterprise)
  enterprise/          ← planned (Enterprise-specific deployment configs)
  hyperscaler/         ← planned (Hyperscaler-specific deployment configs)
```

See [RELEASE_STRATEGY.md](../RELEASE_STRATEGY.md) for the full branch and
edition model.
