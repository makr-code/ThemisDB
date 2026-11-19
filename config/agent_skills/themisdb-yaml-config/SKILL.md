---
name: themisdb-yaml-config
description: Draft and review ThemisDB YAML configuration using verified repository schemas, examples, and validator limits.
metadata:
  AI_Used: true
  AI_Models: "unknown"
  AI_Contributor: "GitHub Copilot"
  Human_Review: false
  Human_Author: "unknown"
---

> AI provenance: Exact model backend not verifiable from final document text
> alone.

# Draft or review ThemisDB YAML configuration

Use this skill when creating, changing, or explaining a ThemisDB YAML
configuration. Produce a minimal candidate or review; do not imply that a
configuration was installed, applied, or hot-reloaded.

1. Establish the configuration purpose, target ThemisDB version and edition,
   deployment platform, and the exact file or subsystem. If these affect
   supported keys or defaults and are unknown, ask before assuming them.
2. Locate the authoritative schema, sample and module documentation for that
   specific configuration. The main schema is
   `config/schema/themisdb.config.schema.json`; it is not a universal schema for
   every YAML file in `config/`. Do not infer supported keys from unrelated
   examples or invent values, defaults, environment-variable expansion, or
   secret-reference syntax.
3. Make the smallest requested change. Preserve unrelated keys and comments.
   Keep credentials and private values out of examples and responses; use
   placeholders unless a documented secret-injection mechanism is verified.
   Do not weaken authentication, TLS, authorization, or network-binding
   protections to make a sample appear to work.
4. Distinguish YAML parsing from schema validation and runtime acceptance.
   `ConfigSchemaValidator` implements a documented subset of JSON Schema
   Draft 7, not full Draft 7 conformance. Report the exact schema and checks
   used; preserve validation warnings as well as errors. A successful parse or
   subset-schema pass does not establish that the server accepts or applies
   every setting.
5. If validation or runtime behavior cannot be checked with an available
   authorized validator or the exact version's source, state that limitation
   and provide the candidate without claiming it is valid or active. Report
   parser, schema, edition, or runtime errors explicitly.
6. Treat file watching as notification only: a changed YAML file is not
   necessarily parsed or applied automatically. Do not claim hot reload unless
   the target subsystem documents and verifies that behavior.

Never put secrets in generated YAML, execute administrative changes, or
present an unverified example as a production-safe configuration.
