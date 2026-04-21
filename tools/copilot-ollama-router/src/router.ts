/**
 * Delegation Router — classifies a user prompt and decides whether it
 * should be handled by local Ollama or routed to Copilot (cloud).
 *
 * Routing heuristic (keyword-based, fast, no extra LLM call required):
 *
 * → Ollama (local)   : boilerplate, test generation, refactoring, documentation
 * → Copilot (cloud)  : security review, architecture decisions, complex debugging
 *
 * ThemisDB-specific override (when copilotOllamaRouter.themisDbRules = true):
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

export interface RoutingPolicyConfig {
  routeBoilerplateToLocal: boolean;
  routeTestsToLocal: boolean;
  routeRefactorsToLocal: boolean;
  routeDocsToLocal: boolean;
  routeCmakeToLocal: boolean;
}

// ---------------------------------------------------------------------------
// Keyword sets that drive classification
// ---------------------------------------------------------------------------

const BOILERPLATE_KEYWORDS: ReadonlyArray<RegExp> = [
  /\bboilerplate\b/i,
  /\bscaffold\b/i,
  /\bfill\s+in\b/i,
  /\bcomplete\s+the\b/i,
  /\bcreate\s+(a\s+)?(class|struct|enum|function)\b/i,
  /\bimplement\s+(interface|abstract|override)\b/i,
  /\badd\s+(getter|setter|constructor|destructor)\b/i,
];

const TEST_KEYWORDS: ReadonlyArray<RegExp> = [
  /\bgenerate\s+(test|spec|unit\s+test|mock)/i,
  /\bwrite\s+(test|spec|mock)/i,
  /\bgtest\b/i,
  /\bfixture\b/i,
];

const REFACTOR_KEYWORDS: ReadonlyArray<RegExp> = [
  /\brefactor\b/i,
  /\bextract\s+(method|function|class)\b/i,
  /\bconvert\s+(to|from)\b/i,
  /\btranslate\s+(to|from)\b/i,
  /\bformat\s+code\b/i,
  /\bclean\s+up\b/i,
];

const DOC_KEYWORDS: ReadonlyArray<RegExp> = [
  /\bdocument(ation)?\b/i,
  /\bjsdoc\b/i,
  /\bdoxygen\b/i,
  /\breadme\b/i,
];

const CMAKE_KEYWORDS: ReadonlyArray<RegExp> = [
  /\bcmakelists\.txt\b/i,
  /\bcmake\b/i,
  /\bbuild\s+system\b/i,
  /\btest\s+target\b/i,
  /\bpreset\b/i,
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
    reasoningModel: string,
    policies: RoutingPolicyConfig
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

    if (this.matchesAny(prompt, COPILOT_KEYWORDS)) {
      return {
        destination: "copilot",
        reason: "Prompt classified as security/architecture/debugging → Copilot cloud.",
      };
    }

    if (policies.routeCmakeToLocal && this.matchesAny(prompt, CMAKE_KEYWORDS)) {
      return {
        destination: "ollama",
        reason: "Workspace policy: CMake/build-system tasks → local Ollama.",
        suggestedModel: defaultModel,
      };
    }

    if (policies.routeTestsToLocal && this.matchesAny(prompt, TEST_KEYWORDS)) {
      return {
        destination: "ollama",
        reason: "Workspace policy: tests/specs/mocks → local Ollama.",
        suggestedModel: defaultModel,
      };
    }

    if (
      policies.routeBoilerplateToLocal &&
      this.matchesAny(prompt, BOILERPLATE_KEYWORDS)
    ) {
      return {
        destination: "ollama",
        reason: "Workspace policy: boilerplate/scaffolding → local Ollama.",
        suggestedModel: defaultModel,
      };
    }

    if (
      policies.routeRefactorsToLocal &&
      this.matchesAny(prompt, REFACTOR_KEYWORDS)
    ) {
      return {
        destination: "ollama",
        reason: "Workspace policy: refactoring/cleanup → local Ollama.",
        suggestedModel: defaultModel,
      };
    }

    if (policies.routeDocsToLocal && this.matchesAny(prompt, DOC_KEYWORDS)) {
      return {
        destination: "ollama",
        reason: "Workspace policy: docs/comments/README → local Ollama.",
        suggestedModel: reasoningModel,
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
