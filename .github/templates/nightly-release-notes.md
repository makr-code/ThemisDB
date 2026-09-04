# ThemisDB Nightly Release — {{ build_date }}

**Build Number:** `{{ github_run_number }}`  
**Build ID:** `{{ github_run_id }}`  
**Commit:** [`{{ git_commit_short }}`]({{ commit_url }})  
**Branch:** `{{ git_branch }}`  
**Release Type:** `nightly`

## ⚠️ Pre-release Notice

This is a **nightly build** of ThemisDB's `develop` branch. It reflects the latest development work and may contain **unfinished features, breaking changes, and known issues**. This build is **not recommended for production use**.

### Use Cases

- **Development & Testing**: Test the latest features before stable releases
- **Early Feedback**: Contribute feedback on in-progress work
- **CI/CD Integration**: Automated testing and integration pipelines

### Stability Notice

- 🟡 **API Stability**: Subject to change without notice
- 🟡 **Feature Completeness**: May be partially implemented
- 🟡 **Performance**: Unoptimized; sanitizers may be active
- 🟡 **Documentation**: May lag behind implementation

---

## Changes Since Last Nightly

{{ changelog_entries }}

### Commit Summary

- **Total Commits:** `{{ commit_count }}`
- **Files Changed:** `{{ files_changed }}`
- **Insertions:** `+{{ insertions }}`
- **Deletions:** `-{{ deletions }}`

---

## Platform Support

| Platform | Binary | Docker |
|----------|--------|--------|
| **Linux x86_64** | ✅ themisdb-nightly.tar.gz | ✅ themisdb:nightly-{{ build_date }} |
| **Linux ARM64** | ✅ themisdb-arm64-nightly.tar.gz | ✅ themisdb:nightly-{{ build_date }}-arm64 |

---

## How to Use

### Docker

Pull the latest nightly build:

```bash
docker pull themisdb/themisdb:nightly
# or specific date:
docker pull themisdb/themisdb:nightly-{{ build_date }}
```

Run:

```bash
docker run -it themisdb/themisdb:nightly
```

### Direct Binary

Download from this release:

```bash
# Extract
tar xzf themisdb-nightly.tar.gz
./themisdb/bin/themis_server
```

---

## Known Issues & Limitations

{{ known_issues }}

---

## Build Metadata

```json
{{ build_metadata_json }}
```

---

## Support & Feedback

- **Issues**: [Report on GitHub](https://github.com/makr-code/ThemisDB/issues)
- **Discussions**: [GitHub Discussions](https://github.com/makr-code/ThemisDB/discussions)
- **Security**: [Security Advisories](https://github.com/makr-code/ThemisDB/security/advisories)

---

**Generated:** {{ build_timestamp }}  
**Workflow:** [{{ github_action_url }}]({{ github_action_url }})
