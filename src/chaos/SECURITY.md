> **Sicherheitshinweis:** Security-Angaben gegen aktuelle Build-Flags, Codepfade und Tests validieren.

# chaos security

## Threat Model
- Misconfiguration may inject persistent faults unexpectedly.
- Invalid probabilities or empty node ids can degrade test reliability.

## Controls
- `injectFault` validates target id and probability range.
- Fault lifecycle is guarded by mutex-protected state.

## Remaining Risks
- Callback handlers are external and can block if implemented poorly.