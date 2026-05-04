# Copilot Instructions (Repository-wide)

## Documentation tasks (MUST)
If the task involves documentation (any change under `docs/**`):

1) Use the standard:
   - Spec: `docs/_standards/doc_header.schema.yml`
   - Template: `docs/_standards/DOC_TEMPLATE.md`

2) Every doc under `docs/**` MUST start with a clickable breadcrumb link chain:
   - `[docs](...) > [<lang>](...) > [<domain?>](...) > [<module>](...) > [<doc_kind>](./<doc_kind>.md)`

3) Every doc header MUST include:
   - **Datum** (YYYY-MM-DD)
   - **Status**
   - **Primary (Quelle der Wahrheit)** (links to `src/**`, `include/**`, `examples/**`)
   - **Bezug / Reference** (issue/PR/module context)

4) Prefer linking to Primary docs instead of duplicating canonical information.