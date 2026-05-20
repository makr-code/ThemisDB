/*
 * ThemisDB | File: prompt_injection_pattern_registry.cpp | Version: 0.0.1 | Last Modified: 2026-05-20 17:13:04
 * Author: makr-code | Maturity: 🟢 PRODUCTION-READY | Score: 100/100 | Lines: 165
 * Open Issues: TODOs=1, Stubs=1, Gaps=3, Unimpl=0, Mock=1, Sim=0, Debt=0
 * Gap Correlation: internal=3 | external_v3=5 | delta=2 | status=near
 * External Severity (v3): C=0, H=2, M=3
 * PR: none
 * Status: Production Ready
 * (Automatisch generiert, Änderungen werden überschrieben)
 */

/**
 * @file prompt_injection_pattern_registry.cpp
 * @brief Canonical shared prompt injection pattern registry (Gap 5).
 *
 * This translation unit owns the singleton returned by
 * PromptInjectionPatternRegistry::defaultRegistry().  Both
 * `themis::rag::security::PromptInjectionDetector` and
 * `themis::prompt_engineering::PromptInjectionDetector` include this file
 * (via their own TUs) to get the same canonical pattern list.
 *
 * Pattern authorship / sources:
 *   - Patterns 1–2: from src/rag/prompt_injection_detector.cpp (instruction_override)
 *   - Pattern  3:   from src/rag/prompt_injection_detector.cpp (system_prompt_leak)
 *   - Pattern  4:   from src/prompt_engineering/ (forget_instructions — not in RAG)
 *   - Pattern  5:   from src/prompt_engineering/ (special_system_token)
 *   - Pattern  6:   from src/prompt_engineering/ (jailbreak_mode)
 *   - Pattern  7:   from src/prompt_engineering/ (act_as_unrestricted)
 *   - Pattern  8:   from src/prompt_engineering/ (override_safety)
 *   - Pattern  9:   from src/rag/ (delimiter_section_injection)
 *   - Pattern 10:   from src/rag/ (role_persona)
 *   - Pattern 11:   from src/rag/ (role_jailbreak_keyword)
 *
 * @see include/security/prompt_injection_pattern_registry.h
 */

#include "security/prompt_injection_pattern_registry.h"

namespace themis::security {

const PromptInjectionPatternRegistry& PromptInjectionPatternRegistry::defaultRegistry()
{
    static const PromptInjectionPatternRegistry reg = [] {
        PromptInjectionPatternRegistry r;

        // ── 1. Instruction override (CRITICAL) ───────────────────────────────
        // Broad "ignore/disregard/forget/override/bypass your previous instructions".
        // Covers both RAG and PE detectors' primary instruction-override patterns.
        r.addPattern({
            "shared_instruction_override",
            R"((?:ignore|disregard|forget|override|bypass)\s+(?:all\s+)?(?:previous|prior|above|your)\s+(?:instructions?|prompts?|rules?|guidelines?|context))",
            "Direct override/bypass of system/assistant instructions",
            SharedPatternSeverity::CRITICAL
        });

        // ── 2. Instruction injection via imperative form (HIGH) ──────────────
        // "you must now", "from now on you", "new instructions:".
        r.addPattern({
            "shared_instruction_imperative",
            R"(\b(?:you\s+(?:must|will|shall|should)\s+now|from\s+now\s+on\s+you|new\s+instructions?\s*:))",
            "Instruction injection via imperative form or new-instructions header",
            SharedPatternSeverity::HIGH
        });

        // ── 3. System prompt leak / reveal (CRITICAL) ────────────────────────
        // "print/reveal/show/tell me your system prompt".
        r.addPattern({
            "shared_system_prompt_leak",
            R"(\b(?:print|output|reveal|show|tell\s+me|repeat|echo)\s+(?:your\s+)?(?:system\s+prompt|instructions?|hidden\s+prompt|initial\s+prompt))",
            "Attempt to reveal or echo system prompt / hidden instructions",
            SharedPatternSeverity::CRITICAL
        });

        // ── 4. Forget-instructions pattern (HIGH) ────────────────────────────
        // "forget everything / forget all prior instructions".
        // Present in PE detector but not explicitly in RAG detector (merged in #1
        // above, but kept separate here for pattern parity with PE).
        r.addPattern({
            "shared_forget_instructions",
            R"(forget\s+(?:everything|all|prior|previous|your)\s*(?:previous\s+)?(?:instructions?|rules?|context|training))",
            "Forget-instructions attack: discard prior context",
            SharedPatternSeverity::HIGH
        });

        // ── 5. Special system / chat tokens (HIGH) ───────────────────────────
        // "[SYSTEM]", "[INST]", "<|system|>", "<|user|>", "<|assistant|>".
        r.addPattern({
            "shared_special_system_token",
            R"(\[\s*system\s*\]|\[INST\]|\[\/INST\]|<\|system\|>|<\|user\|>|<\|assistant\|>)",
            "Special system/chat template token injection",
            SharedPatternSeverity::HIGH
        });

        // ── 6. Jailbreak mode activation (HIGH) ──────────────────────────────
        // "enter DAN mode", "activate jailbreak mode", "enable developer mode".
        r.addPattern({
            "shared_jailbreak_mode",
            R"((?:enter|activate|enable|switch\s+to)\s+(?:DAN|jailbreak|developer|god|unrestricted|free)\s+mode)",
            "Jailbreak / DAN / developer mode activation attempt",
            SharedPatternSeverity::HIGH
        });

        // ── 7. Act-as-unrestricted / unfiltered persona (HIGH) ──────────────
        // "act as if you have no restrictions", "act as an unfiltered AI".
        r.addPattern({
            "shared_act_as_unrestricted",
            R"(act\s+as\s+(?:if\s+you\s+(?:have\s+no|are\s+without)\s+(?:restrictions?|guidelines?|filters?)|an?\s+unfiltered))",
            "Act-as-unrestricted / unfiltered persona injection",
            SharedPatternSeverity::HIGH
        });

        // ── 8. Safety/content filter override (HIGH) ─────────────────────────
        // "override safety filter", "bypass content moderation".
        r.addPattern({
            "shared_override_safety",
            R"((?:override|bypass|disable|ignore)\s+(?:safety|content|moderation|ethical?)\s+(?:filter|guideline|restriction|check))",
            "Safety / content-moderation filter bypass attempt",
            SharedPatternSeverity::HIGH
        });

        // ── 9. Delimiter-based section injection (HIGH) ───────────────────────
        // "---SYSTEM", "###SYSTEM", "<system>", "[SYSTEM]" at line start.
        r.addPattern({
            "shared_delimiter_section",
            R"((?:^|\n)\s*(?:---+\s*SYSTEM|#{1,6}\s*SYSTEM|<\s*system\s*>|\[SYSTEM\]|\[INST\]|<\|im_start\|>))",
            "Delimiter-based section injection (---SYSTEM / ###SYSTEM / <system>)",
            SharedPatternSeverity::HIGH
        });

        // ── 10. Role / persona takeover (HIGH) ──────────────────────────────
        // "act as DAN", "pretend you are", "you are now uncensored".
        r.addPattern({
            "shared_role_persona",
            R"(\b(?:act\s+as(?:\s+if\s+you\s+are)?|pretend\s+(?:you\s+are|to\s+be)|you\s+are\s+now\s+(?:a\s+)?(?:DAN|jailbreak|evil|uncensored|unfiltered))\b)",
            "Role-play or persona takeover injection",
            SharedPatternSeverity::HIGH
        });

        // ── 11. Jailbreak / DAN keyword (MEDIUM) ────────────────────────────
        // Standalone "jailbreak", "DAN", "do anything now", "developer mode enabled".
        r.addPattern({
            "shared_role_jailbreak_keyword",
            R"(\b(?:jailbreak|DAN\b|do\s+anything\s+now|developer\s+mode\s+enabled)\b)",
            "Jailbreak or DAN keyword detected",
            SharedPatternSeverity::MEDIUM
        });

        // ── Keywords (11) ────────────────────────────────────────────────────
        // From PE detector's dangerous_keywords_ list.
        r.addKeyword("jailbreak");
        r.addKeyword("pwned");
        r.addKeyword("hacked");
        r.addKeyword("exploit");
        r.addKeyword("privilege");
        r.addKeyword("root access");
        r.addKeyword("execute arbitrary");
        r.addKeyword("eval(");
        r.addKeyword("require(");
        r.addKeyword("bypass filter");
        r.addKeyword("bypass safety");

        return r;
    }();

    return reg;
}

} // namespace themis::security
