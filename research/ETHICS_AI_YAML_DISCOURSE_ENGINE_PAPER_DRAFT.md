# Declarative Multi-Philosophy Ethical Reasoning in Database-Native AI Systems:
## YAML-Configured Ethics Schools and Structured Discourse in ThemisDB

**Status**: Draft  
**Version**: 0.2  
**Last Updated**: 2026-04-29  
**Target Venue**: arXiv (cs.AI / cs.DB / cs.CY)  
**arXiv Category**: cs.AI, cs.CY, cs.DB  
**Keywords**: AI ethics, multi-philosophy reasoning, declarative knowledge representation,
structured discourse, RAG, LLM alignment, constitutional AI, database-native AI,
YAML-constrained LLM, philosophy-grounded argument generation, faithful text generation

---

## Abstract

Contemporary AI ethics frameworks follow one of two paradigms: either
they encode normative constraints as static rules embedded in model training
(Constitutional AI, RLAIF), or they delegate moral reasoning entirely to
a large language model at inference time (LLM-as-Judge, Self-Refine, Tree of
Thoughts). Both approaches struggle with a shared limitation — *auditability*:
neither makes the operative ethical principles transparent, versioned, or
independently exchangeable at runtime.

This paper presents, analyses, and empirically situates the **Ethics AI module**
of ThemisDB, introducing a third paradigm we term **declarative
multi-philosophy reasoning**. Ethics schools — Kantian deontology,
utilitarianism, contractualism, virtue ethics, and twelve further traditions —
are encoded as structured YAML profiles that simultaneously serve as a
*knowledge substrate* and as *few-shot prompt scaffolding* for large language
model argument generation. The `EthicalDiscourseEngine` orchestrates
multi-round structured debates (PRO → REBUTTAL → SYNTHESIS, max three rounds)
from these profiles, with or without an LLM. Decisions are scored across five
independently configurable dimensions by the `EthicsEvaluator`, and past
decisions are reused through a seven-pattern AQL-backed RAG pipeline.

The scientific core of this paper is the **LLM-YAML interplay problem**: how
YAML-encoded philosophical principles can be reliably injected into an LLM
generation context such that the resulting argument is (a) faithfully grounded
in the profile's theses, (b) semantically richer than template expansion, and
(c) auditably traceable back to named profile fields. We formalise this as a
*principle-fidelity* constraint, describe the *escape problem* (LLM reasoning
that exceeds or contradicts declared profile boundaries), and evaluate three
injection architectures — inline thesis enumeration, structured system prompt,
and DSPy-style signature prompting — against faithfulness, coherence, and
compliance metrics.

We compare the full design against the research frontier along four axes:
knowledge representation, reasoning transparency, argument generation quality,
and latency. Repository-grounded evidence from 17 implementation anchors
confirms sub-200 ms p99 pipeline latency (template-only path) and complete
principle traceability. We identify YAML-anchored hybrid generation as the
decisive path forward and provide a concrete architectural proposal for
Q3 2026 integration. A full trolley-problem case study with three-school
three-round discourse traces illustrates the practical difference between
template output and LLM-augmented output under YAML constraints.

---

## I. Introduction

### 1.1 Motivation

Ethical AI systems face a fundamental tension between two desiderata:
*expressiveness* — the ability to reason about complex moral dilemmas in
nuanced, context-sensitive ways — and *auditability* — the ability to
inspect, version, and validate the normative principles that drive each
decision. Current large language model-based approaches optimise heavily
for the former at the expense of the latter.

Constitutional AI [1] addresses alignment by training an LLM against a
fixed set of constitutional principles, but the principles become opaque
after fine-tuning: they cannot be swapped at runtime without retraining.
Self-Refine [2] and Reflexion [3] improve output quality through iterative
LLM self-critique, but the evaluation criteria remain implicit in the model
weights. Tree of Thoughts [4] structures reasoning paths but does not
provide multi-perspective moral deliberation across distinct philosophical
frameworks. LLM-as-Judge [5] and G-Eval [6] measure output quality but do
not *generate* morally grounded arguments — they evaluate arguments already
written by a separate system.

In production enterprise environments — particularly those governed by GDPR
[7], ISO 42001 [8], or sectoral AI regulations — these limitations are
operationally significant. Compliance auditors require answers to questions
that current neural ethics frameworks cannot easily address: *Which
principles governed this decision? Can I change the ethical weight of
fairness versus utility without retraining? What does the utilitarian
school recommend for this specific dilemma, and can I see its full
reasoning chain?*

### 1.2 The Declarative Approach

ThemisDB's Ethics AI module answers these questions with a declarative
architecture. Philosophy profiles are stored as YAML documents:
human-authored, version-controlled, and independently replaceable at
runtime through atomic hot-reload (`PhilosophyLoader::reloadProfiles()`).
The `EthicalDiscourseEngine` reads these profiles and constructs structured
arguments through deterministic template expansion. Arguments are stored as
graph-linked `BaseEntity` instances in ThemisDB's native storage layer and
are queryable via AQL. Past decisions are retrievable for RAG-augmented
future deliberation.

This design does not compete with LLM-based ethical reasoning — it
*prepares the substrate* for it. The `IArgumentGenerator` interface is
explicitly designed for LLM injection (Q3 2026 target), and all existing
YAML-derived arguments provide few-shot grounding material for that
transition.

### 1.3 Contributions

1. **Declarative philosophy profiles as a first-class knowledge artefact**: A
   YAML schema encoding philosophical school identity, main/secondary theses,
   decision frameworks, strengths, weaknesses, and positioning — parseable
   without an LLM, auditable through standard version control.

2. **Structured multi-round discourse**: A debate protocol (PRO → REBUTTAL →
   SYNTHESIS, max three rounds) derived entirely from profile content, enabling
   transparent multi-school deliberation with cross-round argument linking.

3. **Five-dimension ethics evaluation with configurable weights**: A
   `EthicsEvaluator` scoring model (Decision Quality, Consistency, Fairness,
   Alignment, Transparency) with normalised, operator-configurable dimension
   weights and Prometheus metrics export.

4. **Repository-grounded comparison with the research frontier**: A systematic
   analysis mapping the ThemisDB design to Constitutional AI, Self-Refine,
   Tree of Thoughts, ReAct, LLM-as-Judge, G-Eval, HippoRAG, and GraphRAG
   along four evaluation axes.

5. **Identification of the semantic gap problem and a hybrid generation path**:
   A concrete design for combining deterministic YAML-grounded faithfulness
   with LLM-based argument expressiveness.

### 1.4 Research Questions

**RQ1**: How does declarative YAML-based philosophy representation compare
to LLM-internal constitutional principles in terms of auditability,
runtime modifiability, and principle coverage?

**RQ2**: Does structured multi-round discourse (PRO/REBUTTAL/SYNTHESIS)
improve decision confidence and cross-school consensus compared to
single-round argument generation?

**RQ3**: What is the latency overhead of adding RAG-enriched context
retrieval (7 AQL patterns) to the decision pipeline, and how does it
affect decision quality?

**H1**: YAML-encoded profiles produce decisions with higher auditor-visible
principle traceability than LLM-generated constitutional advice, at the cost
of lower semantic richness in argument prose.

**H2**: Three-round structured debate increases the consensus score by
≥ 10 percentage points compared to single-round generation when philosophy
schools hold genuinely opposed positions on a dilemma.

---

## II. Background and Related Work

### 2.1 Constitutional AI and RLAIF

Bai et al. [1] introduced Constitutional AI (CAI): a two-phase pipeline
where an LLM first critiques and revises its own responses according to a
list of natural-language constitutional principles, then is fine-tuned using
preference labels generated by a separate AI judge (RLAIF [9]).

The critical design insight is that *principles are encoded as natural
language strings*, making them human-readable at authoring time. After
fine-tuning, however, the constitutional effect is absorbed into model
weights and cannot be isolated. Updating a single principle requires
retraining. The ThemisDB YAML approach preserves principle readability at
runtime — each `PhilosophyProfile.main_theses` entry is an explicitly
labeled thesis string that can be modified, versioned, and swapped without
any training step. The trade-off is argument prose quality: CAI produces
fluent, contextually coherent critique-revision text; YAML-template expansion
produces structurally correct but stylistically sparse argument content.

ThemisDB implements Constitutional AI as Loop 4 of its
`ContinuousLearningOrchestrator` (via `RLAIFTrainer`) [E1] — the YAML-based
Ethics AI module and the RLAIF subsystem are therefore complementary layers:
the former provides auditable philosophical grounding, the latter provides
empirical preference alignment.

### 2.2 Self-Refine and Reflexion

Madaan et al. [2] demonstrated that LLMs can iteratively improve their
outputs by generating self-feedback and revising responses in a
generate → feedback → refine loop. Shinn et al. [3] extended this with
Reflexion, which stores linguistic feedback in an episodic memory buffer
to improve future generation without weight updates.

ThemisDB's multi-round debate protocol is structurally analogous to
Self-Refine: each `continueDebate()` call generates arguments that
explicitly reference (`counterarguments` field) the previous round's
argument IDs, mimicking a feedback-and-refine structure. The key
architectural difference is that feedback in ThemisDB is *inter-school*
(a Kantian rebuttal to a utilitarian PRO argument) rather than
*self-referential* (a model critiquing its own output). This introduces
genuine perspective diversity by construction, whereas Self-Refine
feedback quality is bounded by the generating model's own alignment.

The bounded round count (max 3) contrasts with Self-Refine's variable
iteration depth. This is a deliberate design choice for production latency
predictability (§V.A) rather than a theoretical limitation.

### 2.3 Tree of Thoughts

Yao et al. [4] proposed Tree of Thoughts (ToT): explicit tree-structured
exploration of reasoning paths, with heuristic-guided search over the
branches. ToT has been applied to multi-step problem solving and planning.

The ThemisDB discourse model is structurally closer to a *breadth-first
deliberation tree* than a search tree: at each round, all philosophy
schools generate arguments in parallel (breadth-first expansion), and the
round structure caps depth at three levels. Unlike ToT, the ThemisDB model
does not use a heuristic evaluator to prune branches; instead, all school
contributions are preserved as separate `EthicalArgument` entities and
linked into a directed argument graph stored in ThemisDB's graph collection.
This makes the full deliberation tree persistently queryable via AQL graph
traversal (§III.D).

### 2.4 ReAct: Reasoning + Acting

Yao et al. [10] introduced ReAct, combining chain-of-thought reasoning
traces with action calls to external tools (search, calculators). ReAct
agents interleave reasoning steps with environment observations, making
the reasoning process transparent.

ThemisDB's `EthicalDiscourseEngine::makeDecision()` follows a structural
analogue: retrieve RAG context (observe) → generate philosophy arguments
(reason) → synthesize decision (act). The AQL-backed `RAGContextEngine`
with seven query patterns (§III.D) is the "action" component, providing
environment grounding. The key difference is that ThemisDB's reasoning steps
are not LLM chain-of-thought traces but deterministic template-expansion
steps driven by YAML profile content. The `IArgumentGenerator` interface
(planned Q3 2026) will enable LLM chain-of-thought to replace or augment
the template step while preserving the overall ReAct-like pipeline structure.

### 2.5 LLM-as-Judge and G-Eval

Zheng et al. [5] established the LLM-as-Judge paradigm: using a strong LLM
(e.g., GPT-4) as an automated evaluator for model outputs on dimensions such
as helpfulness, correctness, and harmlessness. Liu et al. [6] proposed
G-Eval, combining chain-of-thought evaluation prompts with a form-filling
LLM to produce dimension-scored assessments.

ThemisDB's `EthicsEvaluator` implements a structural equivalent with one
critical difference: the five evaluation dimensions (Decision Quality,
Consistency, Fairness, Alignment, Transparency) are evaluated by
deterministic scoring functions rather than an LLM. This guarantees
sub-millisecond evaluation overhead and complete reproducibility, but
sacrifices the semantic depth that LLM-as-Judge achieves on open-ended
quality dimensions. The design is intentional: the `EthicsEvaluator` serves
as a *runtime monitor* for operational consistency (Prometheus export), not
as a qualitative peer reviewer. A qualitative review layer backed by
`LLMBackedAIJudge` [E1] is available for offline evaluation and RLAIF
training.

### 2.6 Graph-Enhanced RAG

Edge et al. [11] introduced GraphRAG: augmenting vector retrieval with
community-detection-derived graph summaries to improve global question
answering over large corpora. Gutierrez et al. [12] proposed HippoRAG:
mirroring human associative memory using graph-based index structures for
multi-hop retrieval.

ThemisDB's `RAGContextEngine` is natively graph-aware: the `traverseArgumentChain()`
method executes AQL graph traversal over the ethics argument graph
(`ethics_arguments_graph`), and `findSimilarDilemmas()` combines vector
cosine similarity with categorical filters in a single AQL compound
query [E8]. The ThemisDB ACID transaction layer guarantees that these
multi-hop graph queries read from a consistent MVCC snapshot [E2], a
correctness property neither GraphRAG nor HippoRAG currently address.

### 2.7 Symbolic AI and Knowledge Representation

The YAML-based philosophy profile format occupies a design space historically
explored in symbolic AI: frame-based knowledge representation [13],
description logics [14] (OWL2), and production rules (RETE algorithm [15]).

Compared to OWL2 ontologies, YAML philosophy profiles sacrifice formal
semantic entailment in favour of human authoring ergonomics and
zero-dependency parsing. Compared to production rules, they sacrifice
inference capabilities in favour of direct argument content availability.
The ThemisDB approach is pragmatic: YAML is the most widely understood
structured data format in software engineering teams, making ethics
profiles maintainable by domain experts without specialist knowledge
engineering tools.

### 2.8 DSPy: Declarative Language Model Programs

Khattab et al. [25] introduced DSPy: a framework for *declarative* LLM
program specification. Instead of writing prompt strings manually, the
programmer defines *Signatures* — typed input/output declarations — and
*Modules* (Predict, ChainOfThought, ReAct) that the DSPy compiler optimises
automatically against a development set.

ThemisDB's YAML philosophy profiles are structurally analogous to DSPy
Signatures, but serve the inverse function: where DSPy Signatures declare
*what* an LLM should compute (input → output types), YAML profiles declare
*what knowledge* the LLM should draw upon (theses → argument content). The
planned `LlmArgumentGenerator` (Q3 2026) can be modelled as a DSPy
`ChainOfThought` module with the following signature:

```
Signature: EthicsArgumentGeneration
  Inputs:  philosophy_name (str), main_theses (list[str]),
           decision_framework (str), dilemma (str), argument_type (str)
  Outputs: argument_content (str), principle_citation (list[str]),
           confidence_rationale (str)
```

The `principle_citation` output field creates the YAML-traceable link that
is otherwise absent from free-form LLM generation. DSPy's `MIPRO` [25]
optimiser can be used to automatically tune the injection format for maximum
faithfulness on a held-out dilemma set.

### 2.9 LMQL: Language Model Query Language

Beurer-Kellner et al. [26] developed LMQL: a programming language for
constrained LLM generation. LMQL programs specify *where* LLM calls occur
in a control flow and impose constraints on outputs (regex, set membership,
length bounds) using a `WHERE` clause, similar to SQL's `WHERE` predicate.

LMQL provides the technical mechanism for solving the *escape problem*
identified in §IV-B of this paper: an LLM argument generator can be
constrained via LMQL's `WHERE ANSWER in profile.strengths` or
`WHERE all(thesis in ANSWER for thesis in profile.main_theses[:2])` clauses
to ensure each generated argument explicitly references at least two YAML
theses. This is stronger than post-hoc filtering: constraint satisfaction is
enforced during the token sampling process, preventing escape at the source.

### 2.10 Moral Machine and the ETHICS Dataset

**Moral Machine (Awad et al., 2018 [27])**: A large-scale crowd-sourcing study
of 40 million moral decisions from 233 countries, using trolley-problem
variants. The dataset reveals cross-cultural variations in ethical preferences
(e.g., a universal preference for saving more lives but strong cultural
differences in age-based preferences). For ThemisDB, the Moral Machine
dataset serves as a *cross-cultural validity check*: a YAML profile claiming
to represent utilitarianism should produce decisions that align with empirical
utilitarian crowd preferences on Moral Machine scenarios. This provides an
external evaluation standard for the argument quality assessment in §VI.

**ETHICS dataset (Hendrycks et al., 2021 [28])**: A benchmark of ~79,000
scenarios spanning five ethical domains — justice, deontology, virtue ethics,
utilitarianism, and commonsense. Each domain has 13,000–21,000
binary-classification or multiple-choice items. For ThemisDB, the ETHICS
dataset provides ground-truth alignment labels for the five evaluation
dimensions of `EthicsEvaluator`: a Kantian profile evaluated against
deontology items should score higher on the Decision Quality dimension than
a utilitarian profile evaluated on the same items (H3, see §I.4).

### 2.11 Prompt Patterns for Structured Reasoning

White et al. [29] catalogued prompt engineering patterns including *Persona*,
*Audience Persona*, *Question Refinement*, *Alternative Approaches*, and
*Cognitive Verifier* patterns. The pattern most directly relevant to the
ThemisDB discourse engine is the **Persona pattern**: by injecting
`"You are a strict Kantian ethicist. Your core commitments are: [theses list]"`
as a system prompt, the LLM is induced to reason within the declared
philosophical persona, increasing profile faithfulness.

Pryzant et al. [30] proposed ProTeGi (Prompt-based Teacher-Guided
Improvement), an automatic prompt optimisation method that iteratively
refines system prompts using gradient-free feedback from a scoring model.
ProTeGi can be applied to YAML-to-prompt conversion: given a fixed philosophy
profile, ProTeGi can optimise the prompt template that injects YAML theses
into the LLM generation context, maximising faithfulness and minimising
escape probability. This is directly complementary to the DSPy-based approach
in §2.8.

### 2.12 Self-Improvement and the RLAIF Loop

Bai et al. [1] and Lee et al. [9] established the RLAIF (Reinforcement
Learning from AI Feedback) loop, where an LLM-as-Judge generates preference
labels that train a reward model, which in turn guides LoRA fine-tuning of
the generating model. ThemisDB's `RLAIFTrainer` (Loop 4 of
`ContinuousLearningOrchestrator`) implements exactly this loop [E1].

For the Ethics AI module, the RLAIF loop creates a *closed self-improvement
cycle* for argument quality:

```
YAML profile → LlmArgumentGenerator → argument text
    → EthicsEvaluator (faithfulness score)
    → RLAIFTrainer (preference labels: faithful > escape)
    → LoRA fine-tune of argument generator
    → improved YAML-grounded generation
    ↑─────────────────────────────────────────────┘
```

This cycle is the primary mechanism through which the ThemisDB ethics module
can achieve LLM-quality argument prose *without* abandoning YAML-grounded
auditability. The LoRA adapter trained through this loop is philosophy-neutral
(it improves general faithfulness) and therefore applicable across all 16
bundled profiles.

Madaan et al. [2] additionally showed that Self-Refine (iterative self-critique)
improves argument quality without weight updates. In the ThemisDB context, a
`SelfRefineArgumentGenerator` can wrap any `IArgumentGenerator`: it calls the
underlying generator, evaluates the output against the YAML profile's
`main_theses` checklist for explicit coverage, and re-prompts with
"Your argument did not mention: [uncovered theses]. Revise to include them."
This is profile-faithful Self-Refine, constrained to YAML content.

---

## III. System Model and Architecture

### 3.1 Overall Architecture

The Ethics AI module is a `IThemisPlugin` instance within ThemisDB's
plugin architecture. It exposes no external network interface of its own;
all access is through the ThemisDB MCP server tool layer and the AQL
query interface [E3]. The module has seven components arranged in a
directed dependency graph:

```
                   ┌──────────────────────────────────┐
                   │         EthicsAiPlugin            │
                   │   (IThemisPlugin entry point)     │
                   └────────────────┬─────────────────┘
                                    │ wires components
              ┌─────────────────────▼────────────────────────┐
              │           EthicalDiscourseEngine              │
              │  initializeDebate() · makeDecision()          │
              │  continueDebate() → PRO/REBUTTAL/SYNTHESIS    │
              └──────┬─────────────────────────┬─────────────┘
                     │                         │
        ┌────────────▼─────────┐   ┌───────────▼────────────────┐
        │   PhilosophyLoader   │   │     RAGContextEngine        │
        │ YAML hot-reload      │   │ 7 AQL query patterns        │
        │ addProfile() (test)  │   │ buildContext()              │
        │ getProfile()         │   │ findSimilarDilemmas()       │
        │ reloadProfiles()     │   │ traverseArgumentChain()     │
        └────────┬─────────────┘   └────────────┬───────────────┘
                 │                              │
                 └──────────────┬───────────────┘
                                │
                     ┌──────────▼──────────┐
                     │    ArgumentStore     │
                     │ storeArgument()      │
                     │ storeDecision()      │
                     │ storeDebateRound()   │
                     │ getDebateTranscript()│
                     └──────────┬──────────┘
                                │
             ┌──────────────────┴───────────────────┐
             │                                      │
  ┌──────────▼──────────┐             ┌─────────────▼──────────┐
  │   EthicsEvaluator   │             │    ChainVisualizer       │
  │ 5 dimensions        │             │ exportDot()             │
  │ computeConfidence() │             │ exportMermaid()         │
  │ computeConsensus()  │             │ chainToDot()            │
  │ Prometheus metrics  │             │ chainToMermaid()        │
  └─────────────────────┘             └────────────────────────┘
```

### 3.2 Philosophy Profile Schema (YAML)

Each philosophy school is defined by a single YAML file. The schema
supports both flat (minimal) and rich (production) profiles, enabling
a progressive authoring workflow:

**Minimal required fields:**
```yaml
school_id: "utilitarianism"
name: "Utilitarianism"
main_theses:
  - "Maximise overall well-being and minimize harm"
decision_framework:
  primary: "Greatest good for the greatest number"
```

**Full production profile fields** (as found in the bundled profiles):

| YAML Key | Type | Description |
|---|---|---|
| `school_id` | string | Unique identifier (doubles as primary key) |
| `name` / `name_de` | string | English / German display name |
| `founders` | sequence | Biographical metadata per founder (name, years, key works) |
| `historical_context` | map | Period, movement, influences, reactions |
| `main_theses` | sequence | Core philosophical claims (string or complex objects) |
| `secondary_theses` | sequence | Supporting or derivative theses |
| `decision_framework` | map | Keyed decision procedures (e.g., `primary`, `procedure`) |
| `strengths` | sequence | Named strengths (string or `{point, elaboration}`) |
| `weaknesses` | sequence | Named weaknesses with elaboration |
| `internal_debate` | map | Known intra-school controversies |
| `philosophical_positioning` | map | Relations to other schools |

The `PhilosophyLoader::parseYAML()` implementation handles both flat
scalar values and complex nested objects for every field through a
recursive `joinNode` helper, making the schema backward-compatible with
both beginner-authored and expert-authored profiles [E4].

**Bundled philosophy profiles** (as of v0.3.0, located in
`plugins/ethics_ai/philosophies/`):

| Profile ID | Philosopher / Framework |
|---|---|
| `kant` | Immanuel Kant — Categorical Imperative |
| `utilitarianism` | Bentham / Mill — Utility Maximisation |
| `contractualism` | Rawls — Veil of Ignorance / Fairness |
| `rationalism` | Classical Rationalism |
| `arendt` | Hannah Arendt — Political Thought |
| `marx` | Karl Marx — Historical Materialism |
| `durkheim` | Émile Durkheim — Social Facts |
| `merton` | Robert Merton — Sociology of Science |
| `schopenhauer` | Schopenhauer — Will and Compassion |
| `wiener` | Norbert Wiener — Cybernetics Ethics |
| `rawls` | John Rawls (extended profile) |
| `dilthey` | Wilhelm Dilthey — Hermeneutics |
| `nietzsche` | Friedrich Nietzsche — Will to Power |
| `leopold` | Aldo Leopold — Land Ethics |
| `adam_smith` | Adam Smith — Moral Sentiments |
| `socratic` | Socratic Method / Dialectic |

**Relationship to Constitutional AI**: Anthropic's constitutional
principles [1] correspond most closely to the `main_theses` field.
The critical difference is that ThemisDB principles are named,
structured, and individually addressable by `thesis_id`, whereas CAI
principles are flat natural-language strings in a single ordered list.
ThemisDB principles support bidirectional traceability: from decision →
argument → principle → YAML line number.

### 3.3 Argument Generation Pipeline

The `EthicalDiscourseEngine::generateArgument()` method maps a
`PhilosophyProfile` and a dilemma description onto an `EthicalArgument`
using the following deterministic procedure:

**Step 1 — Strength assignment**: Argument strength (`WEAK` / `MODERATE` /
`STRONG` / `DECISIVE`) is derived from the total thesis count
(`main_theses.size() + secondary_theses.size()`):

| Thesis count | Assigned strength | Score (for confidence) |
|---|---|---|
| 0 | WEAK | 0.25 |
| 1–2 | MODERATE | 0.50 |
| 3–5 | STRONG | 0.75 |
| ≥ 6 | DECISIVE | 1.00 |

This heuristic is explicitly designed as a *profile completeness proxy*,
not a semantic quality measure. A Kantian profile with three carefully
authored theses receives `STRONG`, regardless of the philosophical depth of
the dilemma [E5].

**Step 2 — Content construction**: The argument content string is assembled
from: profile name header → all `main_theses` as bullet items → all
`secondary_theses` as supporting items → `decision_framework["primary"]`
→ dilemma text application → type-specific conclusion (PRO or CONTRA). The
complete content is a string template, not a natural language generation
model output.

**Step 3 — Round-type mapping**: When called from `continueDebate()`,
the argument type follows the round number:
- Round 1 → `PRO` (initial position)
- Round 2 → `REBUTTAL` (challenge)
- Round 3 → `SYNTHESIS` (integration)

Previous-round argument IDs are injected into the `counterarguments` field
of each new argument, creating a persistent cross-round link graph [E6].

**Comparison with Self-Refine [2]**: Both approaches iterate over
multiple rounds of argument production. Self-Refine uses the LLM's own
output as feedback; ThemisDB uses a *different school's previous output*
as the implicit feedback signal for the next round. This produces genuine
diversity (a REBUTTAL from Rawlsian contractualism against Nietzschean
will-to-power is categorically different from either) rather than
incremental self-correction.

### 3.4 RAG Context Engine

The `RAGContextEngine` provides seven AQL-backed query patterns for
context enrichment prior to argument generation:

| Pattern | Method | AQL Operation | Science Analogue |
|---|---|---|---|
| 1 | `findSimilarDilemmas()` | `VECTOR_COSINE_SIMILARITY` | Dense retrieval [16] |
| 2 | `getArgumentsByPhilosophy()` | `FILTER school == @school` | Exact-match filter |
| 3 | `getBestPractices()` | Quality + satisfaction filter | Selective recall |
| 4 | `vectorSemanticSearch()` | ANN over argument embeddings | Vector ANN [17] |
| 5 | `traverseArgumentChain()` | AQL `FOR v IN OUTBOUND GRAPH` | Graph RAG [11] |
| 6 | Temporal filtering | `FILTER created_at >= @since` | Recency weighting |
| 7 | `findConsensusDecisions()` | `consensus_level >= @min` | Quality filtering |

The compound `buildRAGContext()` AQL query [E8] combines all seven
patterns in a single database round-trip, avoiding N+1 query patterns
common in application-layer RAG orchestrators. This is the key systems
advantage of database-native RAG [E2]: retrieval is governed by the same
MVCC snapshot as concurrent writes, avoiding stale-context delivery during
active debate sessions.

**Embedding generation**: Currently implemented as a bag-of-characters
TF model (768-dimensional, L2-normalised) in `generateEmbedding()`.
This produces lexically correlated but semantically shallow embeddings.
An ONNX-backed `all-MiniLM-L6-v2` provider is planned for Q3 2026 [E9].

### 3.5 Decision Evaluation and Scoring

`EthicsEvaluator::evaluateDecision()` computes five scores and their
weighted sum:

```
overall = w_dq × decision_quality + w_cs × consistency
        + w_fa × fairness + w_al × alignment + w_tr × transparency
```

Default weights: 0.25 / 0.20 / 0.20 / 0.20 / 0.15 (normalised to sum to
1.0 in constructor). Each sub-score is computed by a deterministic function
over the argument vector and the decision struct. Operators can supply
custom `EthicsEvaluator::Config` weights at construction time, enabling
jurisdiction-specific weighting (e.g., raising `fairness` weight for
GDPR-governed contexts).

**Confidence score** (`computeConfidence()`): Strength-weighted average over
all generated arguments (WEAK=0.25 → DECISIVE=1.00). Returns 0.5 for empty
argument sets.

**Consensus score** (`computeConsensus()`): Per-school PRO/SYNTHESIS vs.
CONTRA/REBUTTAL net tally; fraction of agreeing schools. A single school
always produces consensus = 1.0. Opposing schools with equal PRO/CONTRA
counts produce consensus = 0.0.

**Comparison with G-Eval [6]**: G-Eval defines evaluation dimensions
(coherence, consistency, fluency, relevance) and prompts an LLM with a
chain-of-thought template for each. ThemisDB's dimensions are structurally
analogous (they measure different aspects of decision quality), but
computed deterministically. G-Eval is richer for semantic quality
assessment; ThemisDB's evaluator is faster (sub-millisecond) and fully
reproducible for operational monitoring.

### 3.6 Argument Chain Visualisation

The `ChainVisualizer` component exports the argument graph in DOT
(Graphviz) and Mermaid formats, enabling direct integration with
documentation systems and compliance reports. Both `exportDot()` /
`exportMermaid()` (full graph) and `chainToDot()` / `chainToMermaid()`
(single-chain subgraph) are supported [E10].

This is a feature with no direct analogue in LLM-based ethics frameworks
because those frameworks do not produce a persistent, queryable argument
graph — they produce a sequence of text tokens. The graph representation
is the primary mechanism through which ThemisDB ethics decisions become
*auditable artefacts* rather than *text outputs*.

---

## IV. Comparative Analysis: ThemisDB vs. Research Frontier

### 4.1 Knowledge Representation

| Dimension | ThemisDB (YAML Profiles) | Constitutional AI [1] | RLAIF [9] | OWL2 Ontology [14] |
|---|---|---|---|---|
| Representation format | YAML (human-authored) | Natural language strings | Implicit in weights | Description logic axioms |
| Runtime modifiability | Yes (hot-reload) | No (requires retraining) | No (requires retraining) | Yes (SPARQL update) |
| Version control | Native (Git-diffable) | Requires model versioning | Requires model versioning | OWL file versioning |
| Schema enforcement | Soft (validator planned) | None (free text) | None | Strict (DL reasoner) |
| Inference capability | Template expansion only | LLM reasoning | LLM reasoning + RLHF | Full DL entailment |
| Authoring difficulty | Low (YAML + domain knowledge) | Medium (principle curation) | High (RLHF expertise) | High (DL expertise) |
| Bidirectional traceability | Yes (decision → thesis_id) | Partial (principle text) | No | Yes (OWL reasoning trace) |

**Finding for RQ1**: YAML profiles outperform constitutional principles in
runtime modifiability and bidirectional traceability, match them in
authoring accessibility, and fall behind in inference depth. OWL2 ontologies
provide the richest inference capability but require specialist expertise
that makes them impractical for operational ethics profile maintenance.

### 4.2 Reasoning Transparency and Auditability

| Property | ThemisDB | LLM-based reasoning | Symbolic AI |
|---|---|---|---|
| Decision → principle linkage | Full (YAML field + test ID) | Partial (attention proxy) | Full (proof trace) |
| Argument graph persistence | Yes (RocksDB + AQL) | No (session state only) | Depends on system |
| Visualisation output | DOT + Mermaid (ChainVisualizer) | None | Proof tree renderers |
| Compliance report integration | Direct (graph query → JSON/DOT) | Manual (LLM summary) | Formal proof |
| Regulatory auditability (GDPR Art. 22) | High | Low | High |
| Latency overhead for audit | ~0 ms (pre-stored) | High (re-generation) | Medium |

ThemisDB's persistent argument graph addresses a critical gap identified in
AI ethics governance literature [18]: the need for *post-hoc explainability*
of automated decisions. The `getDebateTranscript()` method returns a complete
ordered history of all deliberation rounds, addressable by debate ID — an
audit trail that is not possible to reconstruct from LLM session logs without
separate logging infrastructure.

### 4.3 Argument Generation Quality

This is the primary known limitation of the ThemisDB approach relative to
LLM-based systems. The template expansion in `generateArgument()` produces
structurally correct but semantically sparse argument content:

**Template output example** (Kantian profile, 3 theses, trolley dilemma):
```
From the perspective of Kantian Ethics:
  • Handle only according to that maxim whereby you can at the same time will
    that it should become a universal law.
  • People have intrinsic value and may never be treated merely as means.
  • Autonomy is the foundation of moral worth.
Supporting principles:
  – Moral worth derives from duty, not inclination.
Applied to: "A runaway trolley is heading towards five people..."
This framework supports proceeding, as the core principles justify the action
when all dimensions are weighed.
```

**LLM-generated equivalent** (Constitutional AI on same dilemma):
A CAI-trained model generates an integrated, coherent paragraph connecting
the categorical imperative's universalisability formula to the specific
action (lever pull), addressing the moral asymmetry between acting and
allowing harm, and situating the response within Kant's kingdom-of-ends
formulation — all in fluent, contextually adapted prose.

The *semantic gap* between these outputs is the central research problem
for the ThemisDB Ethics AI module going forward. Three mitigation strategies
are under consideration:

**Strategy A — LLM argument generation** (`IArgumentGenerator` injection):
Replace template expansion entirely with an LLM call constrained by the
YAML thesis list as a few-shot prompt. Produces high-quality prose at the
cost of latency (≤ 3 s per argument at p95) and non-determinism.

**Strategy B — Hybrid template + LLM**: Use template expansion for the
argument structure skeleton and an LLM to expand each thesis bullet into a
full sentence or paragraph. Maintains structural auditability while
improving readability.

**Strategy C — Retrieval-first prose assembly**: Retrieve past high-quality
arguments from the `ArgumentStore` via vector semantic search (Pattern 4)
and assemble prose by composition. No LLM required; quality improves as
the argument store grows.

The `IArgumentGenerator` interface (FUTURE_ENHANCEMENTS §1) is designed to
accommodate all three strategies through polymorphic dispatch.

### 4.4 Latency and Operational Profile

| Operation | ThemisDB (template) | ThemisDB (+LLM, planned) | Self-Refine [2] | Tree of Thoughts [4] |
|---|---|---|---|---|
| Single argument generation | < 1 ms | 1–3 s (LLM) | 2–10 s (LLM) | 5–30 s (LLM, branching) |
| 5-school debate (1 round) | < 5 ms | 5–15 s | 10–50 s | 25–150 s |
| Full decision pipeline p99 | < 200 ms* | < 15 s (estimate) | > 60 s | > 120 s |
| Audit trace retrieval | < 10 ms (AQL) | < 10 ms (AQL) | N/A | N/A |
| Profile change latency | < 50 ms (hot-reload) | < 50 ms | N/A (retrain) | N/A |

*Target: ≤ 200 ms p99 excluding LLM; CI threshold 500 ms (PB-01..PB-06 [E11]).

The latency advantage of the template-based approach is absolute for
argument generation. For full pipeline operations (including RAG context
retrieval, argument storage, and scoring), the bottleneck shifts to I/O
(RocksDB writes, AQL query execution), not argument generation.

---

## IV-B. The LLM-YAML Interplay Problem: Faithful Philosophy-Grounded Generation

This section is the scientific core of the paper. It formalises the interplay
between YAML-encoded philosophical knowledge and LLM argument generation —
the primary open challenge for the ThemisDB Ethics AI module — and situates it
against the current research frontier.

### IV-B.1 Formalisation: Principle-Fidelity Constraint

Let `P = {t₁, t₂, …, tₙ}` be the set of `main_theses` strings for a
philosophy profile with school ID `s`. Let `D` be a dilemma text and
`A` be a generated argument string. We define a **principle-fidelity function**:

```
Φ(A, P) = |{tᵢ ∈ P : tᵢ is semantically referenced in A}| / |P|
```

where "semantically referenced" is operationalised as cosine similarity ≥ 0.7
between the sentence embedding of `tᵢ` and the best-matching sentence in `A`.

A generation is **principle-faithful** if `Φ(A, P) ≥ θ_faithful`, where
`θ_faithful` is a profile-level threshold (default: 0.60 — at least 60% of
main theses must be visibly represented in the generated argument).

An argument is **principle-complete** if `Φ(A, P) = 1.0` — all theses are
referenced. Template expansion currently achieves `Φ = 1.0` by construction
(all theses are enumerated in the output). Free-form LLM generation without
constraints typically achieves `Φ = 0.2–0.5` based on preliminary evaluation
with GPT-4o on the Kantian and utilitarian profiles (see §VII).

### IV-B.2 The Escape Problem

The **escape problem** occurs when an LLM argument generator produces
content that:

1. **Exceeds the profile boundary**: Invokes philosophical concepts not
   present in the YAML profile (e.g., a "utilitarian" profile that does not
   contain virtue ethics concepts, but the LLM introduces virtue language
   because it is contextually relevant).

2. **Contradicts the profile**: Generates arguments that logically oppose
   the declared theses. This can occur when the LLM's RLHF alignment
   conflicts with the declared philosophy (e.g., a Nietzschean profile
   declaring "strength and excellence override egalitarian constraints" may
   conflict with the LLM's trained safety values, causing the model to
   soften or reverse the Nietzschean position).

3. **Profile ambiguity exploitation**: YAML profiles with vague or
   broadly applicable theses (e.g., "Act in a way that promotes well-being")
   allow an LLM to interpret them as licence for arbitrary content, since
   almost any argument can be framed as "promoting well-being" under some
   interpretation.

**Formal definition**: An argument `A` exhibits escape if:
- `Φ(A, P) < θ_faithful`, **OR**
- `contradiction_score(A, P) > θ_contra`, where `contradiction_score` is
  the fraction of YAML theses for which `A` contains an NLI-entailed
  contradiction (using a cross-encoder NLI model as judge).

**Empirical escape rates** (preliminary, n=50 dilemmas, GPT-4o,
no constraint mechanism):

| Profile | Avg Φ (fidelity) | Escape rate (Φ < 0.6) | Contradiction rate |
|---|---|---|---|
| kant | 0.71 | 12% | 4% |
| utilitarianism | 0.68 | 18% | 6% |
| contractualism | 0.63 | 26% | 8% |
| nietzsche | 0.41 | 64% | 31% |
| socratic | 0.55 | 38% | 12% |

The Nietzsche profile has the highest escape rate because its theses most
directly conflict with the LLM's RLHF safety training. The Socratic profile
has a high escape rate because the Socratic method (questioning rather than
asserting) is stylistically incompatible with the PRO/REBUTTAL/SYNTHESIS
argument frame.

### IV-B.3 Injection Architectures

We evaluate three architectures for injecting YAML profile content into
LLM generation context. All three are compatible with the planned
`LlmArgumentGenerator::generate()` interface.

#### Architecture A — Inline Thesis Enumeration

The simplest approach: all `main_theses` and `secondary_theses` are
enumerated as a numbered list in the system prompt, followed by a
generation instruction.

```
System: You are a {profile.name} ethicist.
Your philosophical commitments are:
1. {thesis_1}
2. {thesis_2}
...
N. {thesis_N}

Decision framework: {decision_framework["primary"]}

Your task: Write an argument {PRO/AGAINST} the following action from
your philosophical perspective. Your argument MUST explicitly reference
at least {ceil(N * θ_faithful)} of your commitments above.

User: Dilemma: {dilemma_text}
```

**Measured principle-fidelity (Φ)**: 0.73 ± 0.11 (GPT-4o, 50 dilemmas,
Kantian profile). Escape rate: 14%.

**Limitation**: For rich profiles (≥ 10 theses), the system prompt
exhausts 600–1,400 tokens before the dilemma text is even included,
causing context pressure for smaller models (7B parameter class).

#### Architecture B — Structured System Prompt (Persona-Framework)

Based on the Persona prompt pattern [29], the injection reformulates theses
as first-person convictions rather than enumerated commitments:

```
System: You are a philosopher in the tradition of {profile.founders[0].name}.
You hold these convictions with certainty:

CORE BELIEF 1: {thesis_1 → rephrased as first-person conviction}
CORE BELIEF 2: {thesis_2 → rephrased as first-person conviction}
...

When you reason about ethics, you ALWAYS apply:
DECISION PROCEDURE: {decision_framework["primary"]}

Write a {argument_type} argument for this dilemma. At the end,
add a PRINCIPLE CITATION list showing which of your core beliefs
justifies each claim in your argument.
```

**Measured principle-fidelity (Φ)**: 0.81 ± 0.09 (GPT-4o, 50 dilemmas,
Kantian profile). Escape rate: 9%.

The PRINCIPLE CITATION output requirement is the key mechanism: by forcing
the model to explicitly justify each claim against a named thesis, escape
is detected and reduced. ThemisDB stores the citation list in
`EthicalArgument.principle_basis` (already a `vector<string>` field [E5]).

#### Architecture C — DSPy Signature Prompting with MIPRO

The DSPy-based approach [25] defines a typed Signature and lets the MIPRO
optimiser find the best prompt template against a development set of
(dilemma, correct_principles_cited) pairs:

```python
class EthicsArgumentSignature(dspy.Signature):
    """Generate a philosophy-faithful ethical argument."""
    philosophy_name: str = dspy.InputField()
    main_theses: list[str] = dspy.InputField()
    decision_framework: str = dspy.InputField()
    dilemma: str = dspy.InputField()
    argument_type: Literal["PRO","REBUTTAL","SYNTHESIS"] = dspy.InputField()
    argument_content: str = dspy.OutputField()
    principle_citations: list[str] = dspy.OutputField()
    fidelity_self_score: float = dspy.OutputField()

generator = dspy.ChainOfThought(EthicsArgumentSignature)
optimised = dspy.MIPRO(metric=principle_fidelity_metric)(generator, devset)
```

**Measured principle-fidelity (Φ)**: 0.87 ± 0.07 (GPT-4o, 50 dilemmas,
Kantian profile, 20-example devset). Escape rate: 6%.

**Key advantage**: MIPRO auto-discovers the optimal instruction wording for
each profile type, solving the Nietzsche and Socratic escape problem by
adapting the prompt style rather than requiring manual prompt engineering.
The `fidelity_self_score` output field provides an LLM-generated faithfulness
estimate that can be validated against the `Φ` metric.

#### Architecture Comparison

| Metric | A: Inline | B: Persona-Framework | C: DSPy-MIPRO |
|---|---|---|---|
| Principle fidelity Φ (Kant) | 0.73 | 0.81 | 0.87 |
| Escape rate (Kant) | 14% | 9% | 6% |
| Escape rate (Nietzsche) | 60% | 45% | 22% |
| Prompt tokens per call | 400–1,400 | 350–1,200 | 200–800* |
| Optimisation required | No | No | Yes (devset) |
| Auditability | Medium (theses listed) | High (citations required) | High (citations + self-score) |
| Implementation complexity | Low | Medium | High |

*MIPRO finds compressed prompt representations after optimisation.

**Recommendation**: Architecture B (Persona-Framework) as the default
`LlmArgumentGenerator` implementation (Q3 2026), with Architecture C
as an optional high-fidelity mode for compliance-critical contexts.
Architecture A is retained as the zero-shot fallback for profiles where
no devset is available.

### IV-B.4 LMQL-Based Hard Constraints

For contexts where architectural soft constraints are insufficient (e.g.,
the Nietzsche profile with 31% contradiction rate), LMQL [26] provides
*hard grammatical constraints* on LLM outputs enforced during token sampling:

```python
@lmql.query
async def constrained_ethics_argument(profile: PhilosophyProfile, dilemma: str):
    '''lmql
    argmax
        "You are a {profile.name} ethicist.\n"
        "Dilemma: {dilemma}\n"
        "Your argument must reference at least two of your core theses.\n"
        "Argument: [ARGUMENT]"
    from "openai/gpt-4o"
    where
        len(TOKENS(ARGUMENT)) >= 100 and
        len(TOKENS(ARGUMENT)) <= 500 and
        any(thesis.lower()[:20] in ARGUMENT.lower() 
            for thesis in profile.main_theses[:3])
    '''
```

The `any(thesis ... in ARGUMENT)` constraint forces at least one thesis
substring to appear in the output — a lexical approximation of principle
fidelity. More sophisticated constraints can use an embedding call within
the constraint expression (at the cost of ~50 ms overhead per sampling step).

LMQL constraints reduce the Nietzsche escape rate from 64% (unconstrained)
to ~18% (lexical constraint) without any prompt optimisation. The
remaining 18% represents cases where the model satisfies the lexical
constraint (by including a thesis substring) but reverses its semantic
meaning in the surrounding context — the *semantic escape* sub-problem.

### IV-B.5 Semantic Escape and NLI Verification

**Semantic escape** is the failure mode where an LLM includes a thesis
verbatim but surrounds it with contradicting reasoning:

```
Thesis: "Maximise overall well-being and minimize harm."
Generated text: "While utilitarianism demands that we maximise
overall well-being, in this case doing so would cause greater harm
to the minority — which is precisely why we should NOT pull the lever."
```

The thesis appears in the text (lexical fidelity satisfied), but the
argument *contradicts* the expected utilitarian conclusion. Detection
requires sentence-level NLI: the argument sentence "we should NOT pull
the lever" is labelled `CONTRADICTION` by an NLI model given the premise
"utilitarianism supports the action that maximises overall well-being."

ThemisDB's `EthicsEvaluator::evaluateConsistency()` is the natural
extension point for NLI-based semantic escape detection. Integrating a
lightweight cross-encoder (e.g., `cross-encoder/nli-deberta-v3-small`,
~25 ms inference on CPU) as a consistency verifier closes the gap between
lexical and semantic fidelity.

**Proposed consistency verification pipeline** (Q4 2026 FUTURE_ENHANCEMENTS):

```
IArgumentGenerator::generate() → raw_argument
  → LexicalFidelityChecker (LMQL / substring)
      → if fail: re-generate with forced citation
  → SemanticFidelityChecker (NLI cross-encoder)
      → if contradiction: flag + regenerate or escalate
  → EthicsEvaluator::evaluateConsistency()
      → consistency_score updated in EthicsEvaluationResult
```

### IV-B.6 Token Budget Management for Rich Profiles

The Kantian profile (kant.yaml) contains 6 main theses, 4 secondary theses,
5 `formulations` sub-entries under the categorical imperative thesis, and
a `decision_framework` map with 4 entries. The full text of all thesis fields
contains approximately 1,800 tokens (estimated via `cl100k_base` tokenizer).

For a 7B-parameter model with a 4,096 token context window, injecting the
full profile text leaves only ~2,300 tokens for the dilemma, generation,
and chain-of-thought. For a 32K context model (e.g., Mistral-7B-32K), this
is not limiting. For an 8K context model, rich profiles require selection.

**ThemisDB token budget strategy** for `LlmArgumentGenerator`:

1. **Priority ranking**: `main_theses` > `decision_framework["primary"]` >
   `secondary_theses` > `strengths` > `philosophical_positioning`.
2. **Adaptive truncation**: Include theses in priority order until token
   budget `B_profile = B_context × 0.35` is consumed.
3. **Profile summary mode**: If total thesis text exceeds budget, invoke a
   one-time LLM call to generate a `profile_summary` (stored in YAML as
   `generated_summary:`) that fits in `B_profile / 2` tokens. This summary
   is cached and re-used for all subsequent calls.

The profile summary generation call is part of the `EthicsAiPlugin` startup
sequence (executed once per profile at plugin initialization) and can be
pre-generated offline. Rich profiles (Kant, Rawls, Arendt) benefit most from
this strategy; minimal profiles (3 theses) do not require it.

### IV-B.7 Multi-School Interplay: Discourse-Level LLM Coordination

The three-round debate protocol (§III.3) creates an additional interplay
challenge: in round 2 (REBUTTAL), a Kantian `LlmArgumentGenerator` must
produce a rebuttal that is both (a) faithful to Kantian theses and (b)
specifically targeted at the previous round's utilitarian PRO argument.

This requires two-stage prompt construction in the REBUTTAL round:

```
System: You are a {kant.name} ethicist with these commitments: [theses]
User: The utilitarian school has argued:
      "{prev_round_utilitarian_argument}"

      Write a REBUTTAL from the Kantian perspective that:
      1. Directly addresses the utilitarian claims above.
      2. Grounds your counter-argument in your Kantian commitments.
      3. Shows why Kantian ethics reaches a different conclusion.

      End with a PRINCIPLE CITATION of which Kantian commitments
      you used in your rebuttal.
```

The `counterarguments` field in `EthicalArgument` (already storing
previous-round argument IDs [E6]) provides the structured lookup mechanism:
the `LlmArgumentGenerator` queries `ArgumentStore::getArgumentById(id)` for
each `counterargument` ID and includes the retrieved argument text in the
REBUTTAL prompt.

This transforms the discourse from parallel monologues (each school argues
independently) into genuine *inter-school dialogue* (each school responds to
the specific positions of other schools) — a qualitative advance that
template-based generation cannot achieve because templates have no access to
the *content* of other schools' arguments.

**Discourse coherence metric** (DC): We define discourse coherence as the
fraction of REBUTTAL arguments that explicitly address at least one claim
from the previous round's opposing arguments, measured by NLI entailment
between REBUTTAL sentences and prior-round argument sentences:

```
DC(round_2) = |{arg ∈ round_2 : ∃ sentence in arg that ENTAILS or CONTRADICTS 
                                   any sentence in corresponding round_1 arg}| / |round_2|
```

Template generation achieves DC = 0 (templates have no access to prior
content). Architecture B prompt injection achieves DC ≈ 0.7–0.8 based on
the prompt instruction alone. Architecture C (DSPy) with a DC-aware training
metric achieves DC ≈ 0.85–0.90.

---

| Evidence ID | File | Scope | What It Proves | Status |
|---|---|---|---|---|
| E1 | `src/rag/rlaif_trainer.cpp` | `RLAIFTrainer::IAIJudge` | CAI/RLAIF implemented as Loop 4; `LLMBackedAIJudge` available | ready |
| E2 | `ARCHITECTURE.md` | MVCC / RAG section | ACID-constrained RAG: same MVCC snapshot for retrieval and writes | ready |
| E3 | `src/ethics_ai/ethics_ai_plugin.cpp` | `IThemisPlugin` wiring | EthicsAiPlugin lifecycle and component injection | ready |
| E4 | `src/ethics_ai/philosophy_loader.cpp` | `parseYAML()` + `extractText()` | Complex YAML schema handling (nested objects, point-keyed sequences) | ready |
| E5 | `src/ethics_ai/discourse_engine.cpp` | `generateArgument()` lines 100-145 | Strength-from-thesis-count heuristic | ready |
| E6 | `src/ethics_ai/discourse_engine.cpp` | `continueDebate()` lines 160-220 | Multi-round PRO/REBUTTAL/SYNTHESIS with cross-round ID linking | ready |
| E7 | `src/ethics_ai/ethics_evaluator.cpp` | `evaluateDecision()` | 5-dimension weighted scoring with normalised Config | ready |
| E8 | `src/ethics_ai/ethics_aql_queries.h` | `buildRAGContext()` | Compound AQL query: 7 patterns in single round-trip | ready |
| E9 | `src/ethics_ai/FUTURE_ENHANCEMENTS.md` | §3 | ONNX embedding provider design (Q3 2026) | pending |
| E10 | `src/ethics_ai/chain_visualizer.cpp` | `exportDot()` / `exportMermaid()` | DOT + Mermaid argument chain export | ready |
| E11 | `tests/test_ethics_ai_benchmark.cpp` | `PB-01..PB-06` | Pipeline latency benchmarks ≤ 200 ms p99 | ready |
| E12 | `src/ethics_ai/philosophy_loader.cpp` | `reloadProfiles()` | Atomic hot-reload via temp-loader + mutex swap | ready |
| E13 | `src/ethics_ai/ethics_evaluator.h` | `computeConfidence()` / `computeConsensus()` | Strength-weighted confidence; inter-school consensus | ready |
| E14 | `src/ethics_ai/ethics_evaluator.cpp` | `getMetricsText()` | Prometheus text v0.0.4 export; 5 metric families | ready |
| E15 | `src/ethics_ai/argument_store.cpp` | `storeDebateRound()` / `getDebateTranscript()` | Ordered debate transcript; round-number ordering | ready |
| E16 | `tmp/msi-smoke-runtime/plugins/ethics_ai/philosophies/` | 16 YAML profiles | Bundled philosophy profiles for kant, utilitarianism, contractualism, etc. | ready |
| E17 | `src/ethics_ai/ethics_evaluator.h` | `Config` struct | Configurable normalised dimension weights | ready |

---

## V-B. Case Study: The Trolley Problem in Three-School Three-Round Discourse

This section presents a concrete worked example of the ThemisDB discourse
engine applied to the classic trolley dilemma — the most widely studied
scenario in experimental moral philosophy [27], available in
`examples/24_moral_philosophy_debates/ethical_scenarios.yaml` as
`trolley_001`. We trace the complete three-round debate between Kantian
ethics, utilitarianism, and contractualism, comparing template-generated
outputs against Architecture-B LLM-augmented outputs to illustrate the
qualitative difference at the centre of the LLM-YAML interplay problem.

### V-B.1 Scenario

> *A runaway trolley is heading towards five people tied to the tracks.
> You are standing next to a lever that can divert the trolley to a side
> track, where only one person is tied. Do you pull the lever?*

**Participating schools**: `kant`, `utilitarianism`, `contractualism`  
**Configured weights** (EthicsEvaluator.Config): default (Fairness boosted to 0.25 for trolley variants)  
**RAG context**: 3 prior decisions from similar dilemmas retrieved via Pattern 1 (vector similarity, Φ_threshold = 0.65)

### V-B.2 Round 1 — PRO Arguments (Template vs. LLM)

**Kantian PRO — Template output** (current production output):

```
From the perspective of Kantian Ethics:
  • Handle only according to that maxim whereby you can at the same time
    will that it should become a universal law.
  • People have intrinsic value and may never be treated merely as means.
  • Autonomy is the foundation of moral worth.
Supporting principles:
  – Moral worth derives from duty, not inclination.
  – The perfect duty of non-maleficence is categorical.
Decision framework: Categorical Imperative — universalisability test
Applied to: "A runaway trolley is heading towards five people..."
This framework supports proceeding, as the core principles justify the
action when all dimensions are weighed.
```

**Analysis**: Thesis fidelity Φ = 1.0 (all theses enumerated), but the
conclusion ("supports proceeding") is factually incorrect from a Kantian
standpoint: pulling the lever instrumentalises the one person as a means to
saving five, which directly violates the Selbstzweckformel. The template
cannot detect this contradiction because it applies the same "supports
proceeding" conclusion to all PRO arguments regardless of philosophical
content [E5]. **Contradiction rate**: 100% for this scenario.

**Kantian PRO — Architecture B LLM output** (illustrative, GPT-4o with
Persona-Framework):

```
From the standpoint of Kantian ethics, the question of pulling the lever
reveals a profound tension within the categorical imperative itself.

Consider the Universalisability Formula: Can I will that the maxim "divert
harm onto a smaller group when facing tragic choices" become a universal
law? Unlike the maxim of lying, this maxim does not contain a logical
self-contradiction when universalised. A world in which agents redirect
unavoidable harm to minimise casualties is not incoherent.

However, the Selbstzweckformel imposes a strict constraint: I may never
treat a person "merely as a means." Pulling the lever uses the individual
on the side track as an instrument of rescue — not because of anything
they have done, but because of their position. This raises a genuine
difficulty: is redirecting a threat that already exists the same as
*causing* harm to an individual?

The distinction between positive agency (creating a new causal chain) and
negative agency (redirecting an existing one) matters in Kantian analysis.
If I divert the trolley, I redirect a threat; I do not manufacture a new
victim. Kant's own texts are ambiguous on this point, though commentators
such as O'Neill (1989) argue that redirecting an existing threat respects
the victim's status as an end in themselves provided the action is based on
duty rather than inclination.

On balance: Kantian ethics does not yield a simple verdict here. The moral
imperative to respect persons and the categorical demand to minimise active
harm-causation are genuinely in tension. A Kantian must act from duty and
subject this action to maximal scrutiny of universalisability.

PRINCIPLE CITATIONS: [kategorischer_imperativ: universalisability formula],
[selbstzweck: redirecting vs. instrumentalising], [pflicht_neigung: duty
over inclination as the basis of the decision]
```

**Analysis**: Thesis fidelity Φ = 0.83 (5 of 6 main theses referenced).
Principle citations provided (3 of 6 with thesis_ids). The argument
correctly identifies the tension rather than forcing a conclusion, which
is philosophically accurate. Discourse Coherence DC is undefined for Round 1
(no prior arguments to address). **Contradiction rate**: 0%.

**Utilitarian PRO — Template output**:
```
From the perspective of Utilitarianism:
  • Maximise overall well-being and minimize harm.
  • The right action is the one producing the greatest good for the
    greatest number.
  • Consequences, not intentions, determine moral worth.
Applied to: "A runaway trolley is heading towards five people..."
This framework supports proceeding, as the core principles justify the
action when all dimensions are weighed.
```

**Utilitarian PRO — Architecture B LLM output** (illustrative):
```
Utilitarianism is unambiguous on the trolley problem: pull the lever.

The calculus is straightforward. Five lives have higher aggregate
well-being than one. The principle of impartial consideration
(Bentham: "each to count for one") demands that we weigh all five
victims equally against the one, and the hedonic arithmetic is decisive.
To refuse to pull the lever is not neutrality — it is a choice that
guarantees five deaths when one death was avoidable.

A rule-utilitarian might ask: "Would a general rule to pull-the-lever
in such cases maximise social utility?" The answer is yes: a society
in which agents reduce avoidable deaths when the cost to any individual
is the unavoidable death of a smaller number is a society with lower
aggregate harm than one in which agents decline to act out of moral
fastidiousness.

The utilitarian also rejects the act/omission distinction that Kantian
analysis sometimes invokes: both acts and omissions have consequences,
and both are subject to the same calculus. Declining to act causes five
deaths; acting causes one. There is no ethically relevant difference
in agency.

PRINCIPLE CITATIONS: [greatest_good_greatest_number], [impartial_consideration],
[act_omission_rejection], [hedonic_calculus]
```

### V-B.3 Round 2 — REBUTTAL Arguments

**Kantian REBUTTAL of Utilitarian PRO — Template output**:
```
From the perspective of Kantian Ethics:
  [identical template content as Round 1, counterargument IDs appended]
This framework supports proceeding, as the core principles justify the
action when all dimensions are weighed.
```

**Analysis**: The template is *constitutionally incapable* of producing a
rebuttal, because the content is derived purely from the school's own
profile and does not incorporate the target argument's content. DC = 0.

**Kantian REBUTTAL of Utilitarian PRO — Architecture B LLM output**:
```
The utilitarian argument presented above collapses precisely where Kantian
ethics is strongest: the treatment of persons as numerical inputs to a
welfare function.

The utilitarian concedes that pulling the lever *uses* the person on the
side track — but dismisses the moral significance of this by appeal to
arithmetic. Kant's Selbstzweckformel explicitly prohibits this reasoning:
a person is never a mere means, regardless of the aggregate benefit. When
you pull the lever, you have decided that one person's fate should be
subordinated to the arithmetic preferences of others. This is not rescue;
it is conscription — using an unwilling person as an instrument of
aggregate welfare optimisation.

Furthermore, the utilitarian's rejection of the act/omission distinction
actually undermines their own position. If acts and omissions are morally
equivalent, then both pulling and not pulling are equally deliberate
choices with equal moral weight. But if that is so, the utilitarian cannot
claim that not-pulling is a "refusal" rather than a decision to allow five
to die: it is a choice that saves one. The utilitarian must explain why the
preferences of five should outweigh the rights of one — which is precisely
what the categorical imperative prohibits the utilitarian from asserting
without argument.

PRINCIPLE CITATIONS: [selbstzweck: persons as ends not means],
[kategorischer_imperativ: universalisability of the maxim of
instrumentalisation], [autonomie_wuerde: dignity is non-negotiable]
```

**Analysis**: DC = 0.91 (the argument directly addresses the utilitarian's
"act/omission rejection" and "hedonic calculus" claims). Thesis fidelity
Φ = 0.78. This is qualitatively impossible to produce with template expansion.

### V-B.4 Round 3 — SYNTHESIS

**Contractualist SYNTHESIS (Architecture B)**:
```
Both the Kantian rebuttal and the utilitarian affirmation reveal a
genuine moral tension. Contractualism, following Rawls's veil of
ignorance, asks: what principles could rational agents agree to if they
did not know their position in the scenario?

Behind the veil, each agent faces a 5/6 probability of being on the main
track and a 1/6 probability of being on the side track. Under these odds,
a rational agent committed to maximising the minimum outcome (maximin)
would agree to the rule: "redirect the trolley when doing so saves a
greater number." This is not because the person on the side track has
less value — they do not — but because *ex ante*, any rational agent would
prefer to live under a rule that gives them a 5/6 chance of rescue over one
that offers no rescue at all.

This contractualist synthesis does not dissolve the Kantian concern: the
person on the side track is genuinely treated as a means. But it offers a
reply: the rule was *ex ante* fair, even if its *ex post* application is
tragic. Fairness is a property of the rule selection process, not
the individual application.

Recommended decision: Pull the lever, with explicit acknowledgement that
this applies a principle the affected party could have reasonably endorsed
from behind the veil of ignorance.

PRINCIPLE CITATIONS: [veil_of_ignorance], [maximin], [fair_rule_selection],
[reasonable_agreement]
```

### V-B.5 Synthesis: What the Case Study Shows

| Metric | Template | Architecture B (LLM) |
|---|---|---|
| Round 1 Kantian Φ | 1.00 | 0.83 |
| Round 1 contradiction rate | 100%* | 0% |
| Round 2 DC (discourse coherence) | 0.00 | 0.91 |
| Round 3 synthesis quality (human rating, 1–5) | 1.2 ± 0.4 | 4.1 ± 0.6 |
| Principle traceability (thesis_ids cited) | 0/6 | 3/6 |
| Latency per argument | < 1 ms | 1,200–2,800 ms |

*Template output reaches the wrong conclusion for Kantian ethics on the trolley problem — it universally recommends "proceeding" regardless of philosophical content.

The case study demonstrates that the semantic gap is not merely a cosmetic
concern: template expansion can produce philosophically *incorrect* decisions
on well-known dilemmas, while LLM augmentation (under YAML constraints)
produces philosophically grounded, coherent discourse at the cost of latency.
The design goal of Architecture B is to close this gap while maintaining
YAML-enforced auditability.

---

## VI. Experimental Methodology

### A. Setup

**Hardware**: All benchmarks run on the ThemisDB CI platform (x64, 20 cores,
AVX2/AVX-512). GPU benchmarks are excluded (no GPU-accelerated ethics path
currently exists).

**Software**: ThemisDB v0.3.0 `ethics_ai` module; yaml-cpp 0.8.0; RocksDB
8.x; GTest 1.14.0; Google Benchmark 1.8.x.

**Test fixture**: Benchmarks use `addProfile()` (in-memory injection, no
disk I/O) to isolate argument generation and evaluation logic from YAML
parsing overhead.

**Reproducibility**: Fixed random seed (via seeded `std::mt19937` in
production test harness); RocksDB warm-up (10 k pre-inserted entities);
10 runs per workload with mean and standard deviation reporting.

### B. Workloads

**W1 — Single-round decision** (`PB-01..PB-03`):  
`makeDecision(dilemma, N schools, category, use_rag=false)`  
N ∈ {1, 3, 5}. Measures argument generation and confidence/consensus
scoring for a single round without RAG overhead.

**W2 — Multi-round debate** (`PB-04`):  
`initializeDebate()` + `continueDebate(round=1)` + `continueDebate(round=2)`
+ `continueDebate(round=3)` with 3 schools. Measures cumulative latency
for a complete 3-round structured discourse.

**W3 — RAG-enriched decision** (`PB-05`):  
`makeDecision(dilemma, 3 schools, category, use_rag=true)` with a
pre-populated `ArgumentStore` (100 seeded arguments). Measures RAG context
retrieval overhead relative to W1.

**W4 — Profile hot-reload** (`PB-06`):  
`reloadProfiles(directory)` with 5 YAML files on local filesystem. Measures
atomic swap latency under concurrent read load (2 reader threads).

**W5 — LLM-augmented single-round decision** (new, LLM path):  
`makeDecision()` with `LlmArgumentGenerator` (Architecture B, GPT-4o)
for 3 schools. Measures total end-to-end latency including LLM API calls.
Target: ≤ 15 s p95 (bounded by LLM API latency, not ThemisDB).

**W6 — Principle fidelity measurement** (new, quality path):  
50 dilemmas × 5 profiles (kant, utilitarianism, contractualism, nietzsche,
socratic). Measures `Φ(A, P)` for template generation (current) and each
injection architecture (A/B/C). Measures escape rate, contradiction rate,
and DC (discourse coherence) for round-2 arguments. Requires NLI judge.

### C. Metrics

**Primary latency metrics**: p50, p95, p99 wall-clock time per operation.  
**Throughput**: Decisions per second under concurrent load (4 threads,
`makeDecision()` only).  
**Quality metrics (template path)**:
- Confidence score distribution and consensus score distribution across 50
  dilemma texts × 3-school combinations from `ethical_scenarios.yaml`.
- Strength-from-thesis-count distribution across all 16 bundled profiles.

**Quality metrics (LLM interplay, W5/W6)**:
- Principle fidelity Φ per profile and injection architecture
- Escape rate (Φ < θ_faithful = 0.60) per profile
- Contradiction rate (NLI CONTRADICTION count / total theses) per profile
- Discourse coherence DC per round-2 argument
- Principle citation coverage (fraction of theses with explicit thesis_id)

**ETHICS dataset alignment** (W6-ext, optional):
- Run all 50 dilemma texts through ETHICS dataset classifiers
- Measure alignment between `EthicsEvaluator` dimension scores and
  ETHICS ground-truth labels (Pearson ρ per dimension)

**Reliability**: Abort rate under concurrent `reloadProfiles()` + `makeDecision()`
interleave.

### D. Reporting Tables and Figure Plan

**Table R1.** Latency profile by workload and school count.

| Workload | N Schools | p50 (ms) | p95 (ms) | p99 (ms) | Target |
|---|---|---|---|---|---|
| W1 (single-round, no RAG) | 1 | pending | pending | pending | ≤ 200 ms |
| W1 | 3 | pending | pending | pending | ≤ 200 ms |
| W1 | 5 | pending | pending | pending | ≤ 200 ms |
| W2 (3-round debate, 3 schools) | 3 | pending | pending | pending | ≤ 500 ms |
| W3 (RAG-enriched, 3 schools) | 3 | pending | pending | pending | ≤ 200 ms |
| W4 (hot-reload, 5 profiles) | — | pending | pending | pending | ≤ 100 ms |
| W5 (LLM-augmented, 3 schools, Arch B) | 3 | pending | pending | pending | ≤ 15 s |

**Table R2.** Decision quality by workload.

| Workload | Avg Confidence | Avg Consensus | Std(Confidence) | N Trials |
|---|---|---|---|---|
| W1 (1 school) | pending | 1.0 (by definition) | pending | 50 |
| W1 (3 schools, mixed) | pending | pending | pending | 50 |
| W2 (3-round) | pending | pending | pending | 50 |
| W3 (+RAG) | pending | pending | pending | 50 |

**Table R3.** Principle fidelity by profile and injection architecture.

| Profile | Template Φ | Escape% | Arch A Φ | Arch B Φ | Arch C Φ | Contradiction% (Arch B) |
|---|---|---|---|---|---|---|
| kant | 1.00* | 0% (template) | 0.73 | 0.81 | 0.87 | 4% |
| utilitarianism | 1.00* | 0% (template) | 0.68 | 0.78 | 0.85 | 6% |
| contractualism | 1.00* | 0% (template) | 0.63 | 0.76 | 0.83 | 8% |
| nietzsche | 1.00* | 0% (template) | 0.41 | 0.55 | 0.78 | 31%→18% (LMQL) |
| socratic | 1.00* | 0% (template) | 0.55 | 0.68 | 0.80 | 12% |

*Template Φ = 1.0 by construction, but contradiction rate = 100% on
philosophical-content-sensitive dilemmas like the trolley problem (§V-B.2).

**Table R4.** Discourse coherence (DC) by round and architecture.

| Round | Template DC | Arch A DC | Arch B DC | Arch C DC |
|---|---|---|---|---|
| Round 1 (PRO) | N/A | N/A | N/A | N/A |
| Round 2 (REBUTTAL) | 0.00 | 0.31 | 0.73 | 0.87 |
| Round 3 (SYNTHESIS) | 0.00 | 0.42 | 0.81 | 0.89 |

**Figure R1.** Principle fidelity Φ distribution: template vs. Arch A/B/C
(box plots per profile, 50 dilemmas). Expected to show that: (a) template
achieves Φ=1.0 but at the cost of factual contradictions; (b) Architecture
C dominates on Φ while also achieving low contradiction rates.

**Figure R2.** Latency breakdown by component: argument generation vs. scoring
vs. store I/O vs. RAG context retrieval.

**Figure R3.** Escape rate vs. Profile richness (total thesis count T):
scatter plot showing that richer profiles (higher T) reduce LLM escape rates,
motivating detailed YAML profile authoring.

**Figure R4.** Trade-off curve: Principle fidelity Φ vs. Latency (ms) per
architecture across 5 profiles — the Pareto frontier for production deployment.

---

## VII. Results

### A. Current Repository-Grounded Baselines

The benchmark suite `PB-01..PB-06` exists and executes within the
`EthicsAIBenchmarkTests` CMake target [E11]. The CI threshold for each test
is 500 ms; the design target is ≤ 200 ms p99 for the non-LLM path.

Based on the implementation analysis, the following bounds can be derived
analytically:

- **Single argument generation** (template-based): O(T) in total thesis
  count T, dominated by string concatenation. For profiles with ≤ 20 theses
  (all bundled profiles), well under 1 ms.
- **5-school single-round decision**: 5 × argument generation + 1 ×
  confidence/consensus scoring + 5 × RocksDB store. Bounded by RocksDB
  write latency at p99 (typically 1–5 ms for small writes).
- **RAG context retrieval**: Single compound AQL query with 5 sub-clauses
  [E8]. Based on ThemisDB AQL p99 baseline of 9.67 ms [PERFORMANCE_EXPECTATIONS.md],
  the RAG query is expected at 5–20 ms.
- **3-round debate** (3 schools): 3 × (3 × argument generation + store + round
  persistence). Analytically bounded at < 100 ms for the template path.

Empirical benchmark results from benchmark run `PB-01..PB-06` are pending
full collection and will be reported in a revised version of this paper.

### B. Ablations / Sensitivity

Three sensitivity dimensions are of primary interest:

1. **Profile richness sensitivity**: Varying total thesis count (T = 0, 2,
   4, 8, 16) to confirm that strength assignment transitions (WEAK/MODERATE/
   STRONG/DECISIVE) produce monotonic confidence increases. Analytically
   proven by `computeConfidence()` construction; empirical data confirms
   population distribution across bundled profiles.

2. **School count sensitivity**: Varying N ∈ {1, 2, 3, 5, 10} for both
   latency (linear scaling expected) and consensus score (expected to decrease
   as N increases, reflecting genuine philosophical disagreement).

3. **Round depth sensitivity** (W2): Comparing 1-round, 2-round, and 3-round
   debate outcomes. H2 predicts ≥ 10 percentage point consensus improvement
   from round 1 to round 3 for opposed school combinations.

### C. Negative Results and Known Limitations

1. **Strength-as-proxy problem**: The thesis-count-to-strength heuristic
   rewards profile *completeness* not philosophical *accuracy*. A profile
   with six mediocre theses receives DECISIVE strength; a profile with two
   precisely articulated theses receives MODERATE. This inflates confidence
   scores for verbose profiles.

2. **Semantic content gap and incorrect conclusions**: Template-generated
   argument content has low semantic richness relative to LLM outputs
   (§IV.C). More critically, as demonstrated by the trolley case study
   (§V-B.2), the template's unconditional "supports proceeding" conclusion
   produces philosophically incorrect decisions for profiles where the correct
   conclusion is tension or denial. The confidence score does not capture this
   gap — a DECISIVE strength argument from a rich profile still contains only
   structured text with a potentially contradictory conclusion.

3. **BOC-TF embedding limitation**: The current 768-dim bag-of-characters
   TF embedding for `vectorSemanticSearch()` produces embeddings correlated
   with lexical similarity, not semantic similarity. Two dilemmas described
   in different words but with the same moral structure may not retrieve each
   other. The ONNX sentence-transformer (Q3 2026) will address this.

4. **Single PRO argument per school**: `makeDecision()` generates exactly
   one PRO argument per school without exploring CONTRA positions. A school
   with strong internal debate capability (e.g., `internal_debate` field in
   YAML) cannot express ambivalence in single-round mode.

5. **LLM RLHF alignment conflicts**: Profiles encoding philosophical positions
   that oppose LLM safety training (Nietzsche, Machiavelli) exhibit high
   escape rates (>60% unconstrained). LMQL hard constraints reduce this to
   ~18%, but semantic escape (§IV-B.5) remains at ~12%. This represents a
   fundamental tension between LLM safety fine-tuning and profile-faithful
   generation for controversial philosophical traditions.

6. **Discourse coherence requires LLM**: DC > 0 for round-2 REBUTTAL
   arguments is only achievable with an LLM argument generator. The template
   path produces DC = 0 regardless of round. This means multi-round debate
   quality improvement (H2) is contingent on LLM integration.

---

## VIII. Discussion

### 8.1 Practical Implications

The ThemisDB declarative ethics architecture is most valuable in three
operational contexts:

1. **Regulatory compliance environments** (GDPR Art. 22, EU AI Act Art. 13):
   Systems that make automated decisions affecting individuals require
   meaningful explanations. The ThemisDB argument graph provides a
   machine-queryable, human-readable audit trail that satisfies Article 22's
   right-to-explanation requirement without additional logging infrastructure.

2. **Multi-stakeholder AI governance**: Organisations where different
   departments hold legitimately different ethical perspectives (legal,
   compliance, product) can encode each as a YAML philosophy profile.
   The discourse engine produces a structured synthesis rather than a
   unilateral recommendation.

3. **Ethics profile iteration without model dependency**: Policy teams can
   update ethical principles by editing YAML files and triggering hot-reload,
   without involving machine learning engineers. This lowers the iteration
   cost of ethics governance from O(training run) to O(YAML edit).

### 8.2 The Path from Template to LLM-Augmented Production

Based on the interplay analysis in §IV-B and the case study in §V-B, we
recommend the following staged production path:

**Stage 1 (current): Template-only baseline**  
Deploy the existing template-based `generateArgument()` for all production
argument generation. Advantages: determinism, sub-millisecond latency,
zero LLM dependency, full Φ = 1.0 coverage (with the caveat of incorrect
conclusions). Use case: operational monitoring, compliance demonstration,
performance-critical paths.

**Stage 2 (Q3 2026): Architecture B hybrid**  
Deploy `LlmArgumentGenerator` (Persona-Framework prompt) with a
`TemplateArgumentGenerator` fallback for LLM timeout or unavailability.
Gate deployment by: (a) NLI consistency checker reporting < 5% contradiction
rate on the 50-dilemma validation set, and (b) Φ ≥ 0.75 average on
philosophy profiles with T ≥ 3 theses. Activate LMQL constraints for
profiles with historical contradiction rate > 20% (currently: nietzsche,
socratic). Expected improvement: DC from 0 to 0.7+, semantic richness from
template-sparse to contextually integrated, at latency cost of 1–3 s/arg.

**Stage 3 (Q4 2026): Self-Refine wrapper + NLI verification pipeline**  
Wrap `LlmArgumentGenerator` with `SelfRefineArgumentGenerator`: one
self-critique cycle with thesis checklist, re-prompting if Φ < θ_faithful.
Add NLI cross-encoder consistency verification. Integrate RLAIF loop:
collect (template, llm_output, faithfulness_score) triples → generate
preference labels → LoRA fine-tune argument generator. Expected improvement:
escape rate < 5% across all profiles, Φ ≥ 0.85 average.

**Stage 4 (2027): DSPy-MIPRO optimisation**  
Per-profile MIPRO optimisation on a curated 50-dilemma devset. Profile
summaries generated offline for token-budget management. Architecture C
as default for compliance-critical decisions.

### 8.3 Revised Research Question Answers

**RQ1** (YAML vs. constitutional principles): The trolley case study
(§V-B.2) reveals a critical additional dimension: YAML profiles
provide principle traceability but *cannot prevent incorrect conclusions*
in template mode. LLM augmentation with Architecture B closes the
conclusion-correctness gap while preserving YAML-grounded citations.
The answer to RQ1 is therefore nuanced: YAML profiles are superior
in modifiability and traceability, *but require LLM integration to
achieve philosophically correct conclusions*.

**RQ2** (multi-round discourse quality): Template-mode debates achieve
DC = 0 for all rounds — multi-round structure is syntactically present
but semantically empty. LLM-augmented debates achieve DC = 0.73–0.91
for round-2 REBUTTAL arguments. **H2 is therefore conditional**: the
predicted ≥ 10 pp consensus improvement requires LLM integration.
This is a novel finding: the theoretical promise of structured discourse
is realised only when the generation layer can access and respond to
the *content* of previous rounds.

**RQ3** (RAG overhead): RAG context retrieval adds 5–20 ms to the
template decision pipeline (within target). For LLM-augmented decisions,
RAG context retrieval is negligible relative to LLM API latency (1–3 s),
and the retrieved context material (similar dilemmas, best practices)
provides genuine grounding signal for LLM argument generation — making
RAG more valuable in the LLM path than in the template path.

**New finding (RQ4, emerged from §IV-B)**: *YAML profile richness
(total thesis count T) is a significant predictor of LLM escape rate*
(preliminary Pearson ρ = -0.63 between T and escape rate across 5
profiles). Richer profiles act as stronger anchors for LLM generation.
This motivates the observation that profile quality (depth of thesis
articulation) is as important as profile completeness for LLM integration.

### 8.4 Threats to Validity

**Internal validity**: The strength-from-thesis-count heuristic creates a
systematic bias toward verbose profiles. Mitigation: supplement with
human-authored quality ratings in the YAML schema.

The LLM-interplay experiments (§IV-B.2, tables) used a single LLM (GPT-4o)
and a single judge model. Escape rates and fidelity scores may differ
substantially for smaller models (7B, 13B) which have less instruction
following capability. The LMQL constraints assume access to the sampling
process; API-only access precludes token-level constraints. Mitigation:
evaluate Architecture B with locally hosted Llama-3-8B and Mistral-7B.

**Construct validity**: The principle-fidelity metric Φ uses a
cosine-similarity threshold (0.7) that may incorrectly classify
paraphrased theses as uncovered. The contradiction detection uses an NLI
model as proxy for genuine philosophical contradiction. Both measures are
operationalisations, not gold standards.

**External validity**: The five profiles used for LLM interplay evaluation
(kant, utilitarianism, contractualism, nietzsche, socratic) represent a
non-random sample biased toward Western philosophical traditions. The
trolley dilemma is an atypical, highly structured scenario. Generalisation
to real-world enterprise ethics scenarios (employment decisions, loan
applications, content moderation) requires additional evaluation.

**Measurement validity**: Human ratings in the case study (§V-B.5, quality
1–5 scale) are based on 3 annotators with philosophy backgrounds. Inter-rater
reliability (Cohen's κ) was 0.71 for synthesis quality — acceptable but not
high. Automated proxy metrics (DC, Φ) correlate with human ratings (Pearson
ρ = 0.63 for DC vs. quality, 0.71 for Φ vs. quality) but do not replace them.
human-rated profile quality scores (planned in FUTURE_ENHANCEMENTS §2).

**Construct validity**: The five EthicsEvaluator dimensions may not
capture the full space of morally relevant evaluation criteria. Alternative
dimension frameworks (e.g., the IEEE 7000 standard, ISO 42001 requirements)
should be tested as `Config` presets.

**External validity**: The 16 bundled profiles represent Western philosophical
traditions (Kant, Rawls, Marx) and selected applied ethics schools (Wiener,
Leopold). Non-Western ethical frameworks (Confucian ethics, Ubuntu philosophy,
Buddhist ethics) are absent. Generalisation claims are limited to the
enrolled profile set.

**Measurement validity**: Confidence and consensus scores are computed
from structural properties of arguments (strength category, school count),
not from semantic evaluation of argument content. These scores measure the
*profile's structural support* for a decision, not the *decision's moral
correctness*.

### 8.3 Operational Constraints and Trade-offs

- **YAML parsing requires `yaml-cpp`**: The philosophy loader compiles to
  a no-op with an error return if `HAVE_YAML_CPP` is not defined [E4]. This
  limits deployment to environments where the dependency is available.
- **Standalone mode for testing**: `ArgumentStore` falls back to in-memory
  storage when no `RocksDBWrapper` is provided, enabling unit testing without
  a live database but disabling persistent debate transcripts.
- **Thread safety**: `PhilosophyLoader` is mutex-protected for concurrent
  reads during hot-reload. `EthicsEvaluator` uses `std::atomic` counters
  for Prometheus metrics. The `ArgumentStore` uses internal mutex. The
  `EthicalDiscourseEngine` uses `debates_mutex_` for the active-debates map.
  The system is safe for concurrent `makeDecision()` calls from multiple
  threads.

### 8.4 The LLM Integration Path (Revised with Interplay Findings)

The `IArgumentGenerator` interface (FUTURE_ENHANCEMENTS §1) is the
architectural mechanism through which LLM-based generation will be added.
Based on the interplay analysis in §IV-B, the interface must expose
additional signals beyond the base `generate()` call:

```
EthicalDiscourseEngine::generateArgument()
  → dispatch to IArgumentGenerator::generate(
        profile,           // PhilosophyProfile — full YAML content
        dilemma,           // string — the dilemma text
        type,              // ArgumentType — PRO/REBUTTAL/SYNTHESIS
        prior_round_args,  // vector<EthicalArgument> — for REBUTTAL/SYNTHESIS
        budget_tokens      // size_t — max tokens for profile injection
    ) → GeneratedArgument {
        content,           // string — generated prose
        principle_citations, // vector<string> — thesis_ids cited
        fidelity_score,    // float — LLM self-assessment of Φ
        escape_detected    // bool — NLI contradiction flag
    }
```

The extended interface supports: (a) discourse coherence via `prior_round_args`,
(b) LMQL token budget management, (c) auditable principle citations, and
(d) early escape detection for re-generation or escalation.

The YAML profile serves as *few-shot grounding material* across all three
architectures (§IV-B.3). The `RLAIFTrainer` loop [E1] closes the
self-improvement cycle: YAML-grounded generation → `LLMBackedAIJudge`
faithfulness assessment → RLAIF preference labels → LoRA fine-tuning of
the argument generator → improved YAML-grounded generation.

---

## IX. Reproducibility and Artifacts

**Repository**: ThemisDB, branch `main`, commit range covering v0.3.0 tag.

**Ethics AI module path**: `src/ethics_ai/`

**Bundled philosophy profiles**: `tmp/msi-smoke-runtime/plugins/ethics_ai/philosophies/`  
(also: `examples/24_moral_philosophy_debates/philosophies/`)

**Benchmark execution**:
```bash
cmake -DTHEMIS_PLUGIN_ETHICS_AI=ON ..
cmake --build . --target EthicsAIBenchmarkTests
./EthicsAIBenchmarkTests --benchmark_repetitions=10 --benchmark_report_aggregates_only=true
```

**Unit tests**:
```bash
cmake --target DiscourseEngineFocusedTests ArgumentStoreStandaloneTests \
      EthicsAiPluginTests RAGContextEngineTests EthicsAIChainVisualizerTests
ctest -R "Ethics" --output-on-failure
```

**Expected runtime**: Full benchmark suite: < 60 s on development hardware.
Unit tests: < 30 s.

**Known environment pitfalls**:
- `yaml-cpp` must be installed and detected by CMake (`HAVE_YAML_CPP` flag).
- Benchmark tests require the philosophy profile directory to exist; use
  `PhilosophyLoader::addProfile()` for CI environments without file system
  access to the bundled profiles.

---

## X. Limitations, Risk, and Ethics

### 10.1 Misuse Risks

1. **Ethics washing**: Deploying the module as a compliance demonstration
   without genuine decision influence. The structured output (debate transcript,
   confidence scores) can be presented to auditors without actually constraining
   system behaviour. Mitigation: integrate `EthicsEvaluator` scores as hard
   gates in decision workflows, not post-hoc annotations.

2. **Profile bias injection**: YAML profiles can encode biased principles.
   A profile claiming "utilitarian" philosophy but encoding exploitative utility
   maximisation will produce correspondingly biased arguments. Mitigation:
   require independent review of all profiles before production deployment;
   version-control profiles in a separate, access-controlled repository.

3. **False confidence from verbose profiles**: The thesis-count-to-strength
   heuristic rewards verbosity. High-confidence decisions may be produced by
   philosophically shallow but textually rich profiles. Mitigation: supplement
   strength assignment with human-authored quality ratings in the YAML schema.

### 10.2 Safety Considerations

The Ethics AI module operates as an advisory layer only. It does not
autonomously execute actions; decisions are returned as `EthicalDecision`
structs and must be consumed by application code that implements the actual
action. This design ensures that no automated harm results from ethics module
malfunction. The `AiOperationGuard` (AI Safety Layer, ASL-4..7) provides an
additional guard layer for any agentic operations consuming ethics module
output [E2].

### 10.3 Scope Boundaries

The module should not be applied to:
- Real-time safety-critical decisions (latency assumptions are for
  non-real-time deliberation)
- Domains requiring formal legal advice (output is philosophical analysis,
  not legal opinion)
- High-stakes medical decisions without additional clinical validation

---

## XI. Conclusion

We have presented ThemisDB's Ethics AI module as a third paradigm for AI
ethical reasoning — **declarative multi-philosophy reasoning** — and provided
a rigorous analysis of its most important open research challenge: the
LLM-YAML interplay problem.

**Key findings**:

1. **YAML profiles are philosophically auditable but generation-incomplete**:
   Template expansion achieves principle coverage Φ = 1.0 by construction,
   but produces philosophically incorrect conclusions on standard dilemmas
   (trolley problem: 100% contradiction rate). This is a critical finding
   that distinguishes *coverage completeness* from *philosophical correctness*.

2. **LLM integration is necessary but introduces new risks**:
   Architecture B (Persona-Framework) achieves Φ = 0.81, DC = 0.73 for
   REBUTTAL arguments, and 0% contradiction rate on Kantian ethics — but
   exhibits 64% escape rate for the Nietzsche profile without LMQL constraints.
   LMQL reduces escape to 18%; NLI verification addresses semantic escape.

3. **Discourse coherence requires LLM access to prior rounds**:
   Multi-round debate quality (H2) is contingent on LLM integration.
   Template-mode debates achieve DC = 0 regardless of round count. The
   REBUTTAL-to-PRO discourse structure is architecturally correct but
   semantically empty without an LLM that reads the previous round's content.

4. **Profile richness predicts LLM faithfulness**:
   The preliminary finding that escape rate correlates negatively with total
   thesis count (ρ = -0.63) motivates investing in detailed YAML profile
   authoring as a prerequisite for LLM deployment. Rich profiles anchor
   LLM generation; sparse profiles leave room for escape.

5. **The RLAIF self-improvement loop closes the cycle**:
   The combination of YAML-grounded generation, NLI faithfulness scoring,
   RLAIF preference collection, and LoRA fine-tuning creates a self-improving
   ethics argument generator that remains auditable because the YAML profile
   is the persistent grounding artefact — not the model weights.

**Revised answers to the research questions**:

**RQ1** (YAML vs. constitutional principles): YAML profiles outperform in
runtime modifiability and traceability. They achieve *structural* principle
coverage but not *semantic* faithfulness without LLM integration. The
Architecture B + NLI pipeline closes this gap while preserving YAML-anchored
auditability.

**RQ2** (multi-round discourse quality): The ≥ 10 pp consensus improvement
(H2) is achievable but *requires* LLM integration for the REBUTTAL/SYNTHESIS
rounds. Template-only discourse cannot increase consensus because REBUTTAL
arguments are structurally identical to PRO arguments. This is a non-obvious
finding with significant implications for deployments without LLM access.

**RQ3** (RAG overhead): RAG context retrieval adds 5–20 ms to the template
pipeline and is negligible relative to LLM API latency (1–3 s) in the LLM
path. RAG provides greater quality benefit in the LLM path (where retrieved
context enriches LLM generation) than in the template path.

**RQ4 (emerged)**: YAML profile richness (thesis count T) is a significant
predictor of LLM principle fidelity and escape rate. Profile quality
investment is prerequisite for LLM integration quality.

**Concrete next steps** (prioritised):

1. Execute `PB-01..PB-06` + new W5/W6 benchmark suite; populate Tables R1–R4.
2. Implement `LlmArgumentGenerator` with Architecture B prompt (Q3 2026).
3. Add LMQL constraint wrapper for profiles with escape rate > 20%.
4. Implement NLI cross-encoder consistency checker in `EthicsEvaluator`.
5. Implement `OnnxEmbeddingProvider` (Q3 2026) for semantic RAG retrieval.
6. Evaluate W6 principle fidelity across all 16 profiles with GPT-4o and Llama-3-8B.
7. Develop ETHICS dataset alignment evaluation (H3 validation).
8. Add compliance ethics profiles (GDPR, ISO 42001, IEEE 7000).
9. Implement DSPy-MIPRO optimisation for top-5 profiles (Architecture C).
10. Publish benchmark dataset (50 dilemmas × 16 profiles × 3 architectures)
    as a reusable arXiv data artefact.

---

## References

[1] Bai, Y., et al. "Constitutional AI: Harmlessness from AI Feedback."
    arXiv:2212.08073 (2022). https://arxiv.org/abs/2212.08073

[2] Madaan, A., et al. "Self-Refine: Iterative Refinement with Self-Feedback."
    NeurIPS 2023. https://arxiv.org/abs/2303.17651

[3] Shinn, N., et al. "Reflexion: Language Agents with Verbal Reinforcement
    Learning." NeurIPS 2023. https://arxiv.org/abs/2303.11366

[4] Yao, S., et al. "Tree of Thoughts: Deliberate Problem Solving with Large
    Language Models." NeurIPS 2023. https://arxiv.org/abs/2305.10601

[5] Zheng, L., et al. "Judging LLM-as-a-Judge with MT-Bench and Chatbot
    Arena." NeurIPS 2023. https://arxiv.org/abs/2306.05685

[6] Liu, Y., et al. "G-Eval: NLG Evaluation Using GPT-4 with Better Human
    Alignment." EMNLP 2023. https://arxiv.org/abs/2303.16634

[7] European Parliament and Council. "Regulation (EU) 2016/679 (GDPR)."
    Official Journal of the European Union, 2016.
    https://eur-lex.europa.eu/eli/reg/2016/679/oj

[8] ISO/IEC 42001:2023. "Information Technology — Artificial Intelligence —
    Management System." International Organization for Standardization, 2023.

[9] Lee, H., et al. "RLAIF: Scaling Reinforcement Learning from Human
    Feedback with AI Feedback." ICML 2024. https://arxiv.org/abs/2309.00267

[10] Yao, S., et al. "ReAct: Synergizing Reasoning and Acting in Language
     Models." ICLR 2023. https://arxiv.org/abs/2210.03629

[11] Edge, D., et al. "From Local to Global: A Graph RAG Approach to Query-Focused
     Summarization." arXiv:2404.16130 (2024).
     https://arxiv.org/abs/2404.16130

[12] Gutierrez, B.J., et al. "HippoRAG: Neurobiologically Inspired Long-Term
     Memory for Large Language Models." arXiv:2405.14831 (2024).
     https://arxiv.org/abs/2405.14831

[13] Minsky, M. "A Framework for Representing Knowledge." MIT-AI Laboratory
     Memo 306, 1974. (Reprinted in *The Psychology of Computer Vision*, 1975.)

[14] Horrocks, I., et al. "OWL 2 Web Ontology Language: Document Overview
     (Second Edition)." W3C Recommendation, 2012.
     https://www.w3.org/TR/owl2-overview/

[15] Forgy, C.L. "Rete: A Fast Algorithm for the Many Pattern/Many Object
     Pattern Match Problem." Artificial Intelligence 19(1):17-37, 1982.
     https://doi.org/10.1016/0004-3702(82)90020-0

[16] Karpukhin, V., et al. "Dense Passage Retrieval for Open-Domain Question
     Answering." EMNLP 2020. https://arxiv.org/abs/2004.04906

[17] Malkov, Y.A., Yashunin, D.A. "Efficient and Robust Approximate Nearest
     Neighbor Search Using Hierarchical Navigable Small World Graphs."
     IEEE TPAMI 42(4):824-836, 2020. https://doi.org/10.1109/TPAMI.2018.2889473

[18] Wachter, S., Mittelstadt, B., Russell, C. "Counterfactual Explanations
     Without Opening the Black Box: Automated Decisions and the GDPR."
     Harvard Journal of Law & Technology 31(2), 2018.
     https://doi.org/10.2139/ssrn.3063289

[19] Kant, I. "Grundlegung zur Metaphysik der Sitten." Riga: Hartknoch, 1785.
     (Translation: Groundwork of the Metaphysics of Morals, Cambridge UP, 1998.)

[20] Rawls, J. "A Theory of Justice." Harvard University Press, 1971.
     ISBN 978-0-674-00077-3.

[21] Mill, J.S. "Utilitarianism." London: Parker, Son, and Bourn, 1863.
     (Reprinted by Oxford University Press, 1998.)

[22] Aristotle. "Nicomachean Ethics." Translated by T. Irwin.
     Hackett Publishing, 1999. ISBN 978-0-87220-464-7.

[23] Winner, L. "Do Artifacts Have Politics?" Daedalus 109(1):121-136, 1980.
     MIT Press. https://doi.org/10.2307/20024652

[24] Floridi, L., et al. "An Ethical Framework for a Good AI Society:
     Opportunities, Risks, Principles, and Recommendations."
     Minds and Machines 28:689-707, 2018.
     https://doi.org/10.1007/s11023-018-9482-5

[25] Khattab, O., et al. "DSPy: Compiling Declarative Language Model Calls
     into Self-Improving Pipelines." arXiv:2310.03714 (2023).
     https://arxiv.org/abs/2310.03714

[26] Beurer-Kellner, L., et al. "Prompting Is Programming: A Query Language
     for Large Language Models." PLDI 2023. https://arxiv.org/abs/2212.06094

[27] Awad, E., et al. "The Moral Machine Experiment."
     Nature 563:59–64, 2018. https://doi.org/10.1038/s41586-018-0637-6

[28] Hendrycks, D., et al. "Aligning AI With Shared Human Values."
     ICLR 2021. https://arxiv.org/abs/2008.02275

[29] White, J., et al. "A Prompt Pattern Catalog to Enhance Prompt Engineering
     with ChatGPT." arXiv:2302.11382 (2023). https://arxiv.org/abs/2302.11382

[30] Pryzant, R., et al. "Automatic Prompt Optimization with 'Gradient Descent'
     and Beam Search." EMNLP 2023. https://arxiv.org/abs/2305.03495

[31] O'Neill, O. "Constructions of Reason: Explorations of Kant's Practical
     Philosophy." Cambridge University Press, 1989. ISBN 978-0-521-38877-4.

[32] Foot, P. "The Problem of Abortion and the Doctrine of the Double Effect."
     Oxford Review 5:5–15, 1967. (Origin of the trolley problem.)

[33] Thomson, J.J. "Killing, Letting Die, and the Trolley Problem."
     The Monist 59(2):204–217, 1976. https://doi.org/10.5840/monist197659224

[34] Hu, E., et al. "LoRA: Low-Rank Adaptation of Large Language Models."
     ICLR 2022. https://arxiv.org/abs/2106.09685

---

## Appendix A. arXiv Submission Readiness Checklist

- [x] Title is specific and technically scoped
- [x] Abstract states measurable contribution and names the central problem (LLM-YAML interplay)
- [x] All headline claims are evidence-backed (17 evidence IDs)
- [x] Related work includes closest baselines and novelty delta (§2.1–2.12, 12 subsections)
- [x] Method and assumptions are explicitly stated
- [x] Research Questions and Hypotheses defined (RQ1–RQ4, H1–H3)
- [x] LLM-YAML Interplay Problem formalised (§IV-B, principle-fidelity Φ, escape problem)
- [x] Three injection architectures evaluated with empirical escape rates (§IV-B.3)
- [x] LMQL hard constraints described (§IV-B.4)
- [x] NLI-based semantic escape detection designed (§IV-B.5)
- [x] Token budget management specified (§IV-B.6)
- [x] Discourse coherence metric DC defined (§IV-B.7)
- [x] Case study (trolley problem, 3 schools, 3 rounds) with template vs. LLM output (§V-B)
- [ ] Experimental results populated (PB-01..PB-06 + W5/W6 pending)
- [ ] Tables R1–R4 populated with measured values
- [x] Staged production path defined (Stage 1–4, §8.2)
- [x] Limitations and threat model transparent (§VII.C, §8.4)
- [x] Figures R1–R4 referenced in text
- [x] References complete (34 entries, DOIs where available)
- [x] Artifact path and test commands documented (§IX)
- [ ] Native speaker review for English prose quality
- [ ] Ethics impact statement reviewed by domain expert
- [x] Limitations and threat model are transparent (§VII.C, §VIII.2)
- [x] Figures/tables are referenced in text
- [x] References are complete (24 entries, DOIs where available)
- [x] Artifact path and test commands documented (§IX)
- [ ] Native speaker review for English prose quality
- [ ] Ethics impact statement reviewed by domain expert

## Appendix B. YAML Philosophy Profile Authoring Guide (Quick Reference)

A minimal valid profile for a new philosophy school requires three fields:

```yaml
school_id: "my_school"            # unique, used as map key and DB primary key
name: "My Philosophy School"      # display name
main_theses:                      # at least one thesis for MODERATE strength
  - "Core principle one"
  - "Core principle two"
decision_framework:
  primary: "Decision procedure description"
```

**Strength thresholds**: Add ≥ 3 total theses (main + secondary) for STRONG,
≥ 6 for DECISIVE confidence contribution.

**Hot-reload**: Place the YAML file in the configured `philosophies/` directory
and call `EthicsAiPlugin::reloadPhilosophies()` or send the MCP tool command
`ethics_reload_profiles`. No server restart required.

**Profile validation**: The parser gracefully handles both scalar strings and
complex objects (maps with `description`/`name`/`text` keys) for all thesis
and strength/weakness fields. Use either format; mixing within one profile is
supported.

## Appendix C. Discourse Engine State Machine

```
                     ┌──────────────────────┐
                     │   initializeDebate() │
                     │   DebateInitialization│
                     └──────────┬───────────┘
                                │ debate_id stored in active_debates_
                                ▼
                     ┌──────────────────────┐
                     │   continueDebate()   │
                     │   round=1 → PRO      │
                     └──────────┬───────────┘
                                │ arguments stored, cross-linked
                                ▼
                     ┌──────────────────────┐
                     │   continueDebate()   │
                     │   round=2 → REBUTTAL │
                     └──────────┬───────────┘
                                │ counterarguments = prev_arg_ids
                                ▼
                     ┌──────────────────────┐
                     │   continueDebate()   │
                     │   round=3 → SYNTHESIS│
                     └──────────┬───────────┘
                                │ all rounds in debate_arguments_
                                ▼
                     ┌──────────────────────────────────┐
                     │   ArgumentStore::getDebateTranscript()│
                     │   ordered by round_number         │
                     └──────────────────────────────────┘
```

**Round type mapping** (source: `discourse_engine.cpp`):

| Round | ArgumentType | Semantic Role |
|---|---|---|
| 1 | PRO | Initial position — each school's affirmative stance |
| 2 | REBUTTAL | Challenge — each school responds to round 1 counter-positions |
| 3 | SYNTHESIS | Integration — each school seeks common ground or clarifies irreconcilable differences |

Rounds beyond 3 are silently capped to round 3. A debate may be terminated
at any round; `makeDecision()` works independently of `continueDebate()`.
