### Context

This issue implements the roadmap item 'TOTP/MFA: Configurable Window and Audit on Drift' for the auth domain. It is sourced from the consolidated roadmap under 🟡 Medium Priority — Near-term (v1.5.0 – v1.8.0) and targets milestone v1.2.0.

Primary detail section: 11. TOTP/MFA: Configurable Window and Audit on Drift

### Goal

Deliver the scoped changes for TOTP/MFA: Configurable Window and Audit on Drift in src/auth/ and complete the linked detail section in a release-ready state for v1.2.0.

### Detailed Scope

### 11. TOTP/MFA: Configurable Window and Audit on Drift

**Priority:** Medium  
**Target Version:** v1.2.0

`mfa_authenticator.cpp:82` enforces only code lengths of 6 or 8 digits. The TOTP time window (number of ± intervals accepted) is not explicitly validated against a minimum/maximum in the public configuration path. A misconfiguration accepting a very wide window (e.g., ±5 steps = ±150 seconds) substantially weakens TOTP replay resistance beyond the `totp_replay_cache.cpp` mitigations.

**Implementation Notes:**
- `[ ]` Add `uint8_t max_window_steps = 1` to `TOTPConfig` in `include/auth/mfa_authenticator.h`; reject configurations where `time_step_window > 2` with `std::invalid_argument` in the constructor (`mfa_authenticator.cpp:82`)
- `[ ]` When a TOTP code validates against a non-zero time step offset (i.e., `step != 0`), emit an audit log entry via `auth_audit_logger.cpp` recording the subject, offset, and timestamp — large sustained offsets indicate a misconfigured device clock
- `[ ]` Expose `totp_drift_histogram` counter in `auth_metrics.cpp` (label: `step_offset`) for operational visibility

---

### Acceptance Criteria

- [ ] Add `uint8_t max_window_steps = 1` to `TOTPConfig` in `include/auth/mfa_authenticator.h`; reject configurations where `time_step_window > 2` with `std::invalid_argument` in the constructor (`mfa_authenticator.cpp:82`)
- [ ] When a TOTP code validates against a non-zero time step offset (i.e., `step != 0`), emit an audit log entry via `auth_audit_logger.cpp` recording the subject, offset, and timestamp — large sustained offsets indicate a misconfigured device clock
- [ ] Expose `totp_drift_histogram` counter in `auth_metrics.cpp` (label: `step_offset`) for operational visibility

### Relationships

- Roadmap row: #150 (🟡 Medium Priority — Near-term (v1.5.0 – v1.8.0))
- Depends on: none identified during generation.
- Part of: consolidated roadmap delivery tracking.

### References

- src/ROADMAP.md
- src/auth/FUTURE_ENHANCEMENTS.md#11-totpmfa-configurable-window-and-audit-on-drift
- Source key: roadmap:150:auth:v1.2.0:11-totpmfa-configurable-window-and-audit-on-drift

Generated from the consolidated source roadmap. Keep the roadmap and issue in sync when scope changes.

<!-- roadmap-source-key: roadmap:150:auth:v1.2.0:11-totpmfa-configurable-window-and-audit-on-drift -->
<!-- roadmap-ref: row=150;module=auth;target=v1.2.0 -->
<!-- roadmap-detail: src/auth/FUTURE_ENHANCEMENTS.md#11-totpmfa-configurable-window-and-audit-on-drift -->
