---
Author: "ThemisDB Team"
Created: "2026-10-30"
Last Updated: "2026-10-30"
Status: "active"
AI_Used: true
AI_Models: "unknown"
AI_Contributor: "GitHub Copilot"
Human_Review: false
Human_Author: "ThemisDB Team"
---

# AI Module Trust Model

AI provenance note: Exact model backend not verifiable from final document text alone.

## Assets and trust boundaries

The generator handles prompt descriptions, endpoint configuration, remote
responses, generated source/build files, callback results, and optional local
telemetry. Trust transitions are:

1. The caller supplies a request and endpoint configuration.
2. The generator validates selected request fields and sends JSON to the
   configured transport/provider.
3. Provider response bytes and every generated field remain untrusted, even
   after JSON and size validation.
4. Optional C1, sandbox-verification, and C2 callbacks provide additional
   policy or telemetry decisions.
5. Downstream build, review, loading, and deployment are separate trust
   decisions and are not performed by this module.

## Implemented controls and their limits

- Description length and capability/dependency count, token syntax, token
  lengths, and uniqueness are checked. ASCII control characters are stripped
  from the description before serialization (except tab, newline, and
  carriage return). These checks do not establish semantic safety or prevent
  prompt injection.
- A non-empty endpoint allow-list requires literal string equality. It is not
  hostname resolution, URL canonicalization, or a network egress firewall.
- Request/response byte limits and generated-field limits constrain accepted
  data. The response limit is checked after body collection, and the bounds do
  not validate code semantics.
- JSON parsing, required implementation presence, and optional callback gates
  reject invalid JSON syntax or returned gate errors. Typed extraction of
  response fields and most callbacks are not wrapped in exception handling;
  malformed field types or thrown callback exceptions can escape instead of
  becoming a `Result` rejection. The `passed_security_checks` field and
  `security_report` come from the provider unless amended by an enabled
  callback; neither is independently verified by the generator.
- Sandbox materialization writes and reads back generated files and copies them
  to the output directory. It does not execute generated code in isolation,
  compile it, or run static analysis.
- The C1 callback's score is a configured policy input. A finite score at or
  above the configured minimum passes that gate; it is not proof of factual
  correctness or absence of vulnerabilities.

## Credentials and sensitive data

The API has no dedicated authentication or secret-provider field. Manage
provider authentication in the configured transport or deployment environment;
do not place secrets in prompts, generated artifacts, or endpoint URL
user-info. The generator's debug log includes the endpoint string and a bounded
prefix of the description. Endpoint trace baggage strips URL path, query, and
userinfo, but that does not redact the separate debug log. Treat log access and
retention accordingly.

C2 telemetry contains generated field sizes, the provider-supplied
`passed_security_checks` value, and the C1 score when present; it does not
include generated source or the prompt in the constructed metrics object. The
callback implementation and its destination still require their own data-flow
review.

## Incident handling

For suspected credential exposure, unsafe generation, or callback bypass:

1. Stop downstream use of the affected generated artifacts or generation route.
2. Preserve sanitized logs, configuration revision, callback/gate state, and
   artifact identifiers under the approved incident-retention policy.
3. Rotate exposed credentials through the provider/deployment secret process;
   do not copy credentials into incident tickets or test fixtures.
4. Escalate suspected vulnerabilities or unexpected safety acceptance through
   the repository security reporting process.
5. Resume only after the boundary/configuration issue is corrected and the
   focused failure-path tests pass.

## References

- [Security controls](./SECURITY.md)
- [Operations](./OPERATIONS.md)
- [Runbook](./RUNBOOK.md)
- [Implementation and gate order](./IMPLEMENTATION.md)
- [Public API](./API.md)
- [`AIPluginGenerator` implementation](./ai_plugin_generator.cpp)
- [`AIPluginGenerator` configuration](../../include/ai/ai_plugin_generator.h)
