---
name: aql-read-query
description: Draft a bounded, read-only AQL query from a question and verified collection schema.
metadata:
  AI_Used: true
  AI_Models: "unknown"
  AI_Contributor: "GitHub Copilot"
  Human_Review: false
  Human_Author: "unknown"
---

> AI provenance: Exact model backend not verifiable from final document text
> alone.

# Draft a read-only AQL query

Use this skill when a user wants an AQL query to inspect data. Produce a
candidate, not an authorization decision or a claim that it was executed.

1. Establish the question, allowed collection, relevant fields and intended
   result size. Ask for missing schema details; do not invent collections,
   attributes or indexes.
2. Draft a read-only query with a bounded `LIMIT` and return only fields needed
   for the question. Use bind parameters for values where the intended
   execution interface supports them; never concatenate untrusted input.
3. Give the query, expected input parameters, assumptions and result shape.
   State when syntax or schema cannot be verified.
4. Execute only if the caller explicitly requests it and a separately
   authorized executor enforces read-only access, collection permissions and
   resource limits. A prompt instruction or a tool allowlist alone is not
   sufficient. If these conditions cannot be established, return the draft
   without executing it.
5. Report validation, execution, permission or timeout failures explicitly.
   Never interpret an unexecuted or failed query as an empty result.

Do not issue writes, DDL, administrative actions or instructions to bypass
access controls.
