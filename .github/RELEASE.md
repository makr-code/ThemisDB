# Release Process

This document describes how releases are created and managed for ThemisDB.

## Branch Strategy

- **`main`** — Read-only release branch. Only receives hotfixes and tagged releases.
- **`develop`** — Active development branch. All features and fixes are merged here first.
- **`release/*`** — Release preparation branches, merged into `main` via PR.
- **`hotfix/*`** — Critical fixes, merged directly into `main` and back-ported to `develop`.

## Creating a Release

Releases are created manually via the **Create Release Archive** workflow:

1. Go to **Actions → Create Release Archive**
2. Click **Run workflow**
3. Enter the version number (e.g., `1.4.0`)
4. Optionally check **Mark as pre-release** for alpha/beta versions
5. Click **Run workflow**

The workflow will:
- Create a source archive (`ThemisDB-v<version>.zip`)
- Generate a SHA256 checksum file
- Tag the commit (`v<version>`)
- Publish a GitHub Release with the archive and checksum

## Version Format

Versions follow [Semantic Versioning](https://semver.org/): `MAJOR.MINOR.PATCH`

Pre-release versions use a hyphen suffix: `1.4.0-alpha`, `1.4.0-rc1`

## Hotfix Process

For critical production fixes:

1. Branch off `main`: `git checkout -b hotfix/issue-description main`
2. Apply the fix
3. Open a PR targeting `main`
4. After merge, back-port to `develop` via a separate PR
5. Create a new patch release using the workflow above
