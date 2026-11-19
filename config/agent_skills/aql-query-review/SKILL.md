---
name: aql-query-review
description: Review AQL candidates for intent, syntax, scope, and operational risk without claiming execution or authorization.
metadata:
  AI_Used: true
  AI_Models: "unknown"
  AI_Contributor: "GitHub Copilot"
  Human_Review: false
  Human_Author: "unknown"
---

> AI provenance: Exact model backend not verifiable from final document text
> alone.

# Review an AQL query

Use this skill to inspect an existing AQL candidate. Do not execute it.

1. Establish the target ThemisDB version, caller's stated purpose, relevant
   collections, expected result or write impact, and available schema evidence.
   Do not assume the query text alone expresses valid business intent.
2. Classify the statement as read, mutation, DDL, or administrative. Identify
   every collection read or changed, filters, projections, joins, and possible
   unbounded scans or writes. Treat unrecognized or mixed statements as
   unresolved, not safe.
3. Check syntax against the target version's parser and focused tests. Check
   schema and semantics separately. Roadmaps, examples, and validator
   acceptance are not proof that a query executes as intended.
4. Report authorization separately: parsing or validation does not grant
   collection, tenant, or row-level access. The host must enforce policy at the
   execution boundary.
5. Return findings with severity, evidence, uncertainty, and a minimal
   correction when justified. Preserve behavior outside the requested intent.
   Do not invent collections, fields, indexes, bind-variable behavior, or
   runtime guarantees.
6. If the query changes data, explain the affected-record scope and risks.
   Never remove safety checks or turn a broad request into a write query.

Distinguish parse, validation, authorization, execution, and result failures.
An unexecuted or failed query is not evidence of an empty result or successful
change.
