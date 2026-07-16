## Scope
- Add a reproducible WinGet manifest generation path tied to release ZIP artifacts.
- Keep scope limited to packaging and release automation files.

## Affected Files
- scripts/release/publish-local-release.ps1
- scripts/release/new-winget-manifest.ps1
- packaging/winget/README.md

## Acceptance Criteria
- A maintainer can generate WinGet manifests from a concrete version, ZIP URL, and SHA256.
- The local release script can emit manifests after building the ZIP artifact.
- Documentation explains generation and submission to winget-pkgs.

## Verification
- Run the manifest generator against a synthetic release URL and an existing version string.
- Validate the release script syntax with PowerShell parsing.