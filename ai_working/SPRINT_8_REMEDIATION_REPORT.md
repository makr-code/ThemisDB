# Sprint 8: Move Semantics Remediation Report

**Total Gaps:** 19

**Files Affected:** 17


## By Type

- clear_after_move: 11 gaps

- member_access_after_move: 8 gaps


## Top Files


### src/ingestion/ingestion_manager.cpp (2 gaps)

- Line 1978: member_access_after_move

  Var: options, Severity: MEDIUM

- Line 2060: member_access_after_move

  Var: options, Severity: MEDIUM


### src/transaction/transaction_manager.cpp (2 gaps)

- Line 1225: member_access_after_move

  Var: old_entity, Severity: MEDIUM

- Line 1279: member_access_after_move

  Var: old_entity, Severity: MEDIUM


### src/search/search_highlighter.cpp (1 gaps)

- Line 54: clear_after_move

  Var: current, Severity: LOW


### src/rag/document_summarizer.cpp (1 gaps)

- Line 39: clear_after_move

  Var: cur, Severity: LOW


### src/rag/delegate_evaluator.cpp (1 gaps)

- Line 100: clear_after_move

  Var: cur, Severity: LOW


### src/rag/multi_step_rag.cpp (1 gaps)

- Line 240: clear_after_move

  Var: current_batch, Severity: LOW


### src/utils/pii_detector.cpp (1 gaps)

- Line 101: clear_after_move

  Var: engines_, Severity: LOW


### src/index/inverted_index.cpp (1 gaps)

- Line 185: clear_after_move

  Var: cur, Severity: LOW


### src/index/secondary_index.cpp (1 gaps)

- Line 3011: clear_after_move

  Var: current, Severity: LOW


### src/server/changefeed_api_handler.cpp (1 gaps)

- Line 571: member_access_after_move

  Var: cid_str, Severity: MEDIUM
