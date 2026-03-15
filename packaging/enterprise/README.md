# ThemisDB Enterprise Packaging

This directory contains **Enterprise Edition** packaging artefacts:

- `.deb` / `.rpm` packages for Linux enterprise distributions
- Windows MSI installers with enterprise features
- macOS PKG with enterprise licensing

## Status

Enterprise-specific packaging artefacts will be placed here when the
Enterprise Edition release pipeline is activated.

## Branch policy

Files in `packaging/enterprise/**` are **blocked from `main`** (Community
release lane) by the PR path gate. They belong exclusively to the `enterprise`
release lane.

See [RELEASE_STRATEGY.md](../../RELEASE_STRATEGY.md) for the full policy.
