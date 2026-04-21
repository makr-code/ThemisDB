/**
 * Delegation Router — classifies a user prompt and decides whether it
 * should be handled by local Ollama or routed to Copilot (cloud).
 *
 * Routing heuristic (keyword-based, fast, no extra LLM call required):
 *
 * → Ollama (local)   : boilerplate, test generation, refactoring, documentation
 * → Copilot (cloud)  : security review, architecture decisions, complex debugging
 *
 * ThemisDB-specific override (when ollamaBridge.themisDbRules = true):
 *   C++ code generation  → always Ollama
 *   Security / audit     → always Copilot
 */

export type Destination = "ollama" | "copilot";

export interface RoutingDecision {
  destination: Destination;
  reason: string;
  /** Suggested Ollama model for this task type (only when destination === "ollama"). */
  suggestedModel?: string;
}

// ---------------------------------------------------------------------------
// Keyword sets that drive classification
// ---------------------------------------------------------------------------

const OLLAMA_KEYWORDS: ReadonlyArray<RegExp> = [
  /\bboilerplate\b/i,
  /\bscaffold\b/i,
  /\bgenerate\s+(test|spec|unit\s+test|mock)/i,
  /\bwrite\s+(test|spec|mock)/i,
  /\brefactor\b/i,
  /\bextract\s+(method|function|class)\b/i,
  /\bdocument(ation)?\b/i,
  /\bjsdoc\b/i,
  /\bdoxygen\b/i,
  /\breadme\b/i,
  /\badd\s+(getter|setter|constructor|destructor)\b/i,
  /\bimplement\s+(interface|abstract|override)\b/i,
  /\bconvert\s+(to|from)\b/i,
  /\btranslate\s+(to|from)\b/i,
  /\bformat\s+code\b/i,
  /\bclean\s+up\b/i,
  /\bfill\s+in\b/i,
  /\bcomplete\s+the\b/i,
  /\bcreate\s+(a\s+)?(class|struct|enum|function)\b/i,
];

const COPILOT_KEYWORDS: ReadonlyArray<RegExp> = [
  /\bsecurity\b/i,
  /\bvulnerabilit(y|ies)\b/i,
  /\baudit\b/i,
  /\bcve\b/i,
  /\bexploit\b/i,
  /\binjection\b/i,
  /\bxss\b/i,
  /\bsql\s+injection\b/i,
  /\bthreat\s+model\b/i,
  /\barchitect(ure|ural)?\b/i,
  /\bdesign\s+(pattern|decision|tradeoff)\b/i,
  /\bscalab(ility|le)\b/i,
  /\bconsistency\s+(model|guarantee)\b/i,
  /\bdistributed\s+system\b/i,
  /\bcap\s+theorem\b/i,
  /\bconcurren(cy|t)\s+(bug|issue|race)\b/i,
  /\bdeadlock\b/i,
  /\brace\s+condition\b/i,
  /\bcomplex\s+(bug|issue|problem|crash)\b/i,
  /\broot\s+cause\b/i,
  /\bpostmortem\b/i,
];

// ThemisDB-specific: C++ file/code patterns → always Ollama
const CPP_LANGUAGE_PATTERN = /\b(c\+\+|cpp|\.cpp|\.hpp|\.h|\.cc|\.hxx)\b/i;

// ---------------------------------------------------------------------------
// Classifier
// ---------------------------------------------------------------------------

export class DelegationRouter {
  /**
   * Classifies `prompt` and returns a routing decision.
   *
   * @param prompt          Raw user prompt text.
   * @param activeLanguage  Language identifier of the active editor file (e.g. "cpp").
   * @param themisDbRules   Whether ThemisDB-specific routing overrides are active.
   * @param delegationMode  User-configured mode ("auto" | "always" | "never").
   * @param defaultModel    Configured default Ollama model.
   * @param reasoningModel  Configured Ollama reasoning model.
   */
  classify(
    prompt: string,
    activeLanguage: string,
    themisDbRules: boolean,
    delegationMode: "auto" | "always" | "never",
    defaultModel: string,
    reasoningModel: string
  ): RoutingDecision {
    if (delegationMode === "always") {
      return {
        destination: "ollama",
        reason: "delegationMode=always: forced to local Ollama.",
        suggestedModel: defaultModel,
      };
    }

    if (delegationMode === "never") {
      return {
        destination: "copilot",
        reason: "delegationMode=never: forced to Copilot cloud.",
      };
    }

    // ThemisDB override: security/audit always → cloud
    if (themisDbRules && this.matchesAny(prompt, COPILOT_KEYWORDS)) {
      return {
        destination: "copilot",
        reason: "ThemisDB rule: security/architecture → Copilot cloud.",
      };
    }

    // ThemisDB override: C++ in active file or prompt → local
    if (
      themisDbRules &&
      (activeLanguage === "cpp" ||
        activeLanguage === "c" ||
        CPP_LANGUAGE_PATTERN.test(prompt))
    ) {
      return {
        destination: "ollama",
        reason: "ThemisDB rule: C++ code-generation → local Ollama (codellama).",
        suggestedModel: defaultModel,
      };
    }

    // Generic copilot keywords
    if (this.matchesAny(prompt, COPILOT_KEYWORDS)) {
      return {
        destination: "copilot",
        reason: "Prompt classified as security/architecture/debugging → Copilot cloud.",
      };
    }

    // Generic ollama keywords
    if (this.matchesAny(prompt, OLLAMA_KEYWORDS)) {
      const isDocTask =
        /\bdocument(ation)?\b/i.test(prompt) ||
        /\breadme\b/i.test(prompt) ||
        /\bjsdoc\b/i.test(prompt) ||
        /\bdoxygen\b/i.test(prompt);

      return {
        destination: "ollama",
        reason: "Prompt classified as boilerplate/test/refactoring/docs → local Ollama.",
        suggestedModel: isDocTask ? reasoningModel : defaultModel,
      };
    }

    // Fallback: short prompts without clear signals → copilot for quality
    return {
      destination: "copilot",
      reason: "No clear classification signal — defaulting to Copilot cloud.",
    };
  }

  private matchesAny(text: string, patterns: ReadonlyArray<RegExp>): boolean {
    return patterns.some((p) => p.test(text));
  }
}
