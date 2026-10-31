---
Author: "ThemisDB Contributors"
Created: "2026-09-20"
Last Updated: "2026-10-31"
Status: "review"
AI_Used: true
AI_Models: "unknown"
AI_Contributor: "GitHub Copilot"
Human_Review: false
Human_Author: "ThemisDB Contributors"
---

Exact model backend not verifiable from final document text alone.

# Config Doxygen Evidence Status

## Current generation evidence

Generated on 2026-10-31 with Doxygen 1.9.8 and `Doxyfile.audit` using
`scripts/generate_module_doxygen_xml.py --modules config`. The generator
completed with `PASS`: 30 C/C++ files, 104 compounds, 51 classes/structs, and
15 namespaces. The Doxygen process exited successfully; its warning log
contains 109 warnings, primarily duplicate `@param` documentation and
undocumented arguments in other config APIs. The encrypted-store header no
longer has warnings for its public declarations after correcting their
documentation.

The generated XML was reviewed for `ConfigEncryptedStore::get()`,
`tryGet()`, `rotateKey()`, and `serialize()`. The extracted brief descriptions
match the current API comments; `tryGet()` documents the absent-key result,
`rotateKey()` documents the exclusive-lock wait, and `serialize()` identifies
the embedded key material. This is a Doxygen generation and spot review, not a
complete public-symbol coverage audit.

The XML, manifest, summary, and warning log were generated under a temporary
directory and are not checked in. Reproduce from the repository root with:

```sh
python3 scripts/generate_module_doxygen_xml.py \
  --modules config \
  --output-root /tmp/themis-config-doxygen/artifacts \
  --manifest /tmp/themis-config-doxygen/manifest.json \
  --summary-md /tmp/themis-config-doxygen/summary.md \
  --doxyfile Doxyfile.audit \
  --doxygen-bin doxygen \
  --module-timeout-seconds 180
```

## Source of API documentation

Public declarations are in `include/config/*.h`; implementation is under
`src/config/*.cpp`. XML counts are inventory metadata only and do not establish
complete API documentation coverage. The remaining warnings should be triaged
before claiming a clean Doxygen run. See
[`../../include/config/README.md`](../../include/config/README.md) for the
header inventory and [`MODULE_GAPS.md`](MODULE_GAPS.md) for contract and
validation gaps.
