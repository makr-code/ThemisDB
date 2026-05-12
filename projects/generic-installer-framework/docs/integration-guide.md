# Integration Guide

## Add GIUF to a Product

1. Provide product configuration files outside installer binaries.
2. Publish signed release manifests for each release.
3. Distribute GIUF CLI or embed GIUF library.
4. Trigger install/update/uninstall workflows from product bootstrapper.

## Required Inputs

- product config
- trust config with public keys
- release manifest + signature
- artifact payloads

## Recommended Deployment

- Keep configuration in separate repository.
- Version configurations independently from binaries.
- Require signature verification in all environments.
