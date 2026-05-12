# Sample Product Config for GIUF

This folder contains a product-agnostic sample configuration set for the generic installer framework.

## Files

- product.yaml: Product metadata and install/update defaults
- channels.yaml: Available release channels and rollout policies
- sources.yaml: Release source endpoints and retry settings
- trust.yaml: Public keys and verification policy
- install.recipe.yaml: Install actions and post-install tasks
- uninstall.recipe.yaml: Uninstall actions and cleanup rules

## Notes

- Keep these files outside installer binaries.
- Sign all runtime-relevant files (at least manifest and trust-related config).
- Version each config file in source control.
