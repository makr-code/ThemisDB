---
name: aql-mutation-draft
description: Draft cautious AQL mutation candidates from explicit intent and verified collection details; never execute them.
metadata:
  AI_Used: true
  AI_Models: "unknown"
  AI_Contributor: "GitHub Copilot"
  Human_Review: false
  Human_Author: "unknown"
---

> AI provenance: Exact model backend not verifiable from final document text
> alone.

# Draft a bounded AQL mutation

Use this skill only when the user explicitly requests a data change. Produce
an unexecuted candidate, not an authorization decision or a claim that data
was changed.

1. Establish the exact operation, target collection, intended records, fields
   and values, expected affected-record count, and any rollback requirement.
   Ask for missing details rather than guessing collections, keys, fields, or
   values.
2. Verify the syntax against the target version's parser and focused tests.
   The mutation parser tests cover `INSERT`, `UPDATE`, `REPLACE`, `REMOVE`, and
   `UPSERT`; examples in older roadmaps or frozen contracts may conflict with
   current source and must not be treated as proof of deployed behavior.
3. Preserve operation semantics: `UPDATE` changes selected fields, `REPLACE`
   substitutes the document, `REMOVE` deletes, and `UPSERT` inserts or updates
   according to its search expression. If the requested operation's exact
   semantics cannot be verified for the target version, stop and state what
   remains uncertain.
4. Require an explicit, narrow record-selection condition for every
   `UPDATE`, `REPLACE`, `REMOVE`, or `UPSERT`. Never draft an unbounded or
   whole-collection mutation. A `FILTER`, `LIMIT`, or safety-validator pass
   alone does not prove that the intended records alone will change.
5. Where possible, provide a separate read-only preview that returns the
   proposed target keys and relevant current values. Keep its predicate
   identical to the mutation's selection logic, and warn that data may change
   between preview and write; preview is not an atomic guarantee.
6. Clearly label the mutation as a candidate. Explain its target, selection
   logic, expected impact, assumptions, and any unverified syntax or behavior.
   Do not claim that parsing, validation, or preview guarantees runtime
   acceptance or successful application.
7. Do not execute the query. Execution requires an explicitly requested
   operation, a separately authorized executor, and host-enforced
   authentication, collection/tenant permissions, mutation policy, resource
   bounds, and any required user confirmation. If these controls are unknown,
   provide the draft only.

Never bypass read-only policies, infer permission from the prompt, or turn a
broad request such as "clean up old data" into a destructive mutation without
the user specifying the exact scope.
