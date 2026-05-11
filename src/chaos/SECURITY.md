> **Sicherheitshinweis:** Security-Angaben gegen aktuelle Build-Flags, Codepfade und Tests validieren.

# chaos security

## Threat Model
- Misconfiguration may inject persistent faults unexpectedly.
- Invalid probabilities or empty node ids can degrade test reliability.

## Controls
- `injectFault` validates target id and probability range.
- Fault lifecycle is guarded by mutex-protected state.
- Scheduler lifecycle (`start`/`stop`) uses explicit running state and controlled thread join.

## Remaining Risks
- Callback handlers are external and can block if implemented poorly.
- Fault events are in-memory and process-local; no signed/audited distributed event channel exists yet.

## Operational Guidance
- Restrict use of chaos hooks to test/staging paths unless explicitly approved in production rollout plans.
- Keep callback code minimal and non-blocking to avoid lock contention and latency spikes.
- Review fault injection entry-points in service code for authorization and environment gating.
