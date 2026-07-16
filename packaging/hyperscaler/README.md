# ThemisDB Hyperscaler Packaging

This directory contains **Hyperscaler Edition** packaging artefacts:

- OCI/container bundles for cloud marketplaces (AWS, Azure, GCP)
- Operator Lifecycle Manager (OLM) bundles for OperatorHub
- Cloud-specific deployment packages (e.g., Azure Managed Application,
  AWS CloudFormation stacks, GCP Deployment Manager templates)

## Status

Hyperscaler-specific packaging artefacts will be placed here when the
Hyperscaler Edition release pipeline is activated.

## Branch policy

Files in `packaging/hyperscaler/**` are **blocked from `main`** (Community
release lane) and from `enterprise` (Enterprise lane) by the respective PR
path gates. They belong exclusively to the `hyperscaler` release lane.

See [RELEASE_STRATEGY.md](../../RELEASE_STRATEGY.md) for the full policy.
