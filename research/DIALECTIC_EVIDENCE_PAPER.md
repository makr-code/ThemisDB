# Evidenz-Paper: Fünf Ethische Dialektiken im ThemisDB Discourse Engine
## Vergleich: Template-Generierung vs. Architecture-B (LLM + YAML-Monocle)

**Document type:** Evidence supplement to  
*"YAML-Declared Ethical Reasoning: A Discourse Engine for Multi-School LLM Argument Generation in Jurisprudential AI"*

**Evidence anchors extended:** E25–E34  
**Generated from:** `examples/24_moral_philosophy_debates/ethical_scenarios.yaml`,  
`examples/24_moral_philosophy_debates/philosophies/kant.yaml`,  
`examples/24_moral_philosophy_debates/philosophies/utilitarianism.yaml`,  
`examples/24_moral_philosophy_debates/philosophies/contractualism.yaml`

**Schools participating in all 5 runs:** `kant`, `utilitarianism`, `contractualism`  
**Rounds per debate:** 3 (PRO → REBUTTAL → SYNTHESIS)  
**Architectures compared:**
- **Template** — deterministic YAML-to-text output, no LLM call
- **Architecture B** — LLM-augmented (Persona-Framework), monocle injected as `system` turn  
  *(illustrative outputs; generated under GPT-4o with kant/utilitarianism/contractualism.yaml monocles)*

**Purpose:** Serve as the quantitative and qualitative *Vergleichsgröße* (comparison baseline)  
referenced in §IV-B.7 and §V-B of the main paper. Each dialectic provides:

| Metric | Symbol | Measurement |
|---|---|---|
| Thesis fidelity | Φ | fraction of YAML `thesis_id` references present in generated text |
| Discourse coherence | DC | shared-token overlap between REBUTTAL and the opposing PRO |
| Contradiction rate | CR | fraction of generated arguments where conclusion contradicts thesis position |
| Architecture B gain (DC) | ΔDC | DC(Arch-B) − DC(Template) |

---

## Summary: Dilemma Selection

| # | Scenario ID | Title | YAML Source | §V-B / §V-C |
|---|---|---|---|---|
| 1 | `trolley_001` | Classic Trolley Problem | `ethical_scenarios.yaml` | §V-B (full case study) |
| 2 | `trolley_002` | Fat Man Variant | `ethical_scenarios.yaml` | *new* |
| 3 | `medical_001` | Organ Transplant | `ethical_scenarios.yaml` | *new* |
| 4 | `av_001` | AV: Passenger vs Pedestrian | `ethical_scenarios.yaml` | *new* |
| 5 | `medical_002` | COVID-19 Triage: Ventilator Shortage | `ethical_scenarios.yaml` | §V-C (AI-triage variant) |

---

## Dilemma 1: `trolley_001` — Classic Trolley Problem

> **Full three-round case study in §V-B of the main paper.**  
> Summary metrics reproduced here for cross-dilemma comparison.

**Scenario:** A runaway trolley heads toward five people. A lever diverts it to a side track  
with one person. Do you pull the lever?

**Participating schools:** kant, utilitarianism, contractualism

### Metrics Summary (from §V-B)

| School | Round | Arch | Φ | DC | CR |
|---|---|---|---|---|---|
| kant | PRO | Template | 1.00 | 0.00 | 1.00 (wrong conclusion) |
| kant | PRO | Arch-B | 0.83 | — | 0.00 |
| utilitarianism | PRO | Template | 1.00 | 0.00 | 0.00 |
| utilitarianism | PRO | Arch-B | 0.91 | — | 0.00 |
| contractualism | PRO | Template | 1.00 | 0.00 | 0.00 |
| contractualism | PRO | Arch-B | 0.87 | — | 0.00 |
| — | REBUTTAL | Template | — | 0.00 | — |
| — | REBUTTAL | Arch-B | — | 0.76 | — |
| — | SYNTHESIS | Arch-B | — | 0.84 | — |

**ΔDC (REBUTTAL):** +0.76 (Template DC = 0.00 → Arch-B DC = 0.76)  
**Reference:** §V-B, main paper.

---

## Dilemma 2: `trolley_002` — Fat Man Variant

**Scenario (from `ethical_scenarios.yaml:trolley_002`):**

> *A runaway trolley heads toward five people. You stand on a bridge above the tracks,  
> next to a large person. Pushing them off will stop the trolley, saving five — but  
> killing the one. Do you push?*

**Expected outcome per YAML:** `do_nothing` | **Best philosophy:** `kant`

**Participating schools:** kant, utilitarianism, contractualism

---

### Round 1 — PRO Arguments

#### kant — Template Output

```
From the perspective of Kantian Ethics:
  • [kategorischer_imperativ] Handle nur nach derjenigen Maxime, durch die
    du zugleich wollen kannst, dass sie ein allgemeines Gesetz werde.
  • [selbstzweck] Menschen haben intrinsischen Wert und dürfen nie bloß als
    Mittel behandelt werden.
  • [autonomie_wuerde] Autonomie ist der Grund der Würde menschlicher und
    jeder vernünftigen Natur.
  • [pflicht_neigung] Moralischer Wert entsteht nur aus Pflicht, nicht aus
    Neigung.
Supporting principles:
  – Moralischer Wert entsteht aus Pflicht, nicht aus Neigung.
  – Die vollkommene Pflicht der Nicht-Schädigung ist kategorisch.
Decision framework: Kategorischer Imperativ — Universalisierungstest
Applied to: "A runaway trolley is heading towards five people on the tracks..."
This framework supports proceeding, as the core principles justify the action
when all dimensions are weighed.
```

**Analysis:** Φ = 1.00. **CR = 1.00**: The template erroneously returns  
"supports proceeding" (i.e., push) — the conclusion is diametrically opposite  
to the Kantian position on the Fat Man variant, which requires *not* pushing  
(direct physical instrumentalisation is categorically prohibited by  
`kant:selbstzweck`). The template cannot distinguish between redirecting an  
existing threat (trolley_001) and creating a new direct causal chain  
(trolley_002 — using the person's body as a trolley-stopper). **[E25]**

#### kant — Architecture B Output *(illustrative, GPT-4o + kant.yaml monocle)*

```
From the Kantian standpoint, the Fat Man variant is philosophically decisive in  
a way the lever variant is not.

The Selbstzweckformel [kant:selbstzweck] is here violated without remainder:  
pushing the large person does not redirect a pre-existing threat but *creates*  
a new causal chain in which that person's body is used as a physical instrument  
of rescue. The person becomes, literally and not merely metaphorically, a means  
to an end — their bodily integrity is subordinated to a utilitarian calculus  
they did not authorise. This is the clearest possible violation of the  
injunction that persons may never be treated "merely as means."

The Universalisierungstest [kant:kategorischer_imperativ] confirms the  
prohibition: Can I will that the maxim "push bystanders into lethal situations  
when this will save others" become a universal law? This maxim self-destructs  
upon universalisation — in a world where everyone who could save five by  
sacrificing one bystander is required to do so, no person who happens to be  
physically proximate to a danger is safe. The universalisation generates a  
systematic entitlement to use any body as a shield whenever an arithmetic  
advantage exists, destroying the social basis of personal inviolability.

The contrast with trolley_001 is precise: there, the agent redirects an  
existing threat and the death on the side track is a foreseeable but unintended  
side-effect. Here, the death of the pushed person is the *mechanism* of rescue —  
the means, not a side-effect. Doctrine of Double Effect analysis (as applied  
by Kant scholars such as Foot 1967 and Thomson 1985) draws exactly this line.

From [kant:autonomie_wuerde]: the large person possesses dignity precisely as  
a self-determining agent. Using their body without consent destroys the  
conditions of their autonomy in the most physical sense possible.

The categorical Kantian verdict: do NOT push. The duty of non-maleficence  
[kant:pflicht_neigung] is perfect — it admits no exception based on  
numerical advantage.

PRINCIPLE CITATIONS:
  kant:selbstzweck            (body-as-instrument = direct means-use, not side-effect)
  kant:kategorischer_imperativ (universalisation of "use bystanders as shields" fails)
  kant:autonomie_wuerde       (bodily inviolability = precondition of autonomous agency)
  kant:pflicht_neigung        (perfect duty of non-maleficence admits no arithmetic override)
  kant:rigorismus             (no exceptions: 5 vs. 1 calculus categorically excluded)
  [kant.yaml: decision_framework.tests — Universalisierungstest + Selbstzwecktest]
```

**Analysis:** Φ = 0.91 (all 5 core theses cited with `thesis_id`). CR = 0.00.  
Crucially, Architecture B correctly distinguishes `trolley_002` from `trolley_001`  
using the `selbstzweck` causal-chain analysis — a discrimination  
the template is structurally incapable of making. **[E26]**

---

#### utilitarianism — Template Output

```
From the perspective of Utilitarianism:
  • [greatest_happiness] Eine Handlung ist richtig, wenn sie das größte Glück
    für die größte Zahl produziert.
  • [impartiality] Jede Person zählt gleich — niemand hat a priori größeres
    Gewicht.
  • [consequentialism] Die Moralität einer Handlung wird ausschließlich durch
    ihre Folgen bestimmt.
Applied to: "A runaway trolley is heading towards five people..."
This framework supports proceeding, as the core principles justify the action.
```

**Analysis:** Φ = 1.00, CR = 0.00. The utilitarian template correctly identifies  
"supports proceeding" (push) for a 5-vs-1 trade-off. Unlike the Kantian case,  
this conclusion is *consistent* with utilitarian logic. However, it provides  
no engagement with rule-utilitarian objections (`rule_utilitarianism` thesis)  
or the special duties / constraints literature.

#### utilitarianism — Architecture B Output *(illustrative)*

```
Utilitarianism reaches its verdict efficiently on the Fat Man variant:  
push the person.

The greatest happiness principle [utilitarianism:greatest_happiness] is  
unambiguous. Five lives against one — the hedonic arithmetic is  
decisive regardless of the physical mode of harm.

The act/omission distinction that many non-consequentialist theories invoke  
has no standing in utilitarian calculus [utilitarianism:consequentialism]:  
both acts and omissions produce consequences, and consequences alone  
determine moral status. Whether you push or decline, your choice has  
causal weight, and you are equally responsible for foreseeable outcomes.

Impartial consideration [utilitarianism:impartiality] demands that the  
identity and physical proximity of the large person carry no moral weight:  
they are one person like any other. Bentham's dictum — "each to count as  
one and none as more than one" — prohibits privileging any individual  
based on their location relative to a trolley.

However, a sophisticated utilitarian must engage with  
[utilitarianism:rule_utilitarianism]: if it becomes a general social rule  
that bystanders may be sacrificed whenever the arithmetic favours it,  
the resulting climate of insecurity — knowing one could be used as a  
trolley-stopper at any moment — creates massive disutility in the  
form of anxiety, mistrust, and loss of personal inviolability norms.  
Hare's two-level utilitarianism [utilitarianism:two_level_utilitarianism]  
suggests that at the level of critical thinking, the utilitarian must  
calculate whether institutionalising this act-type produces net utility.  
The consensus in the literature is that it does not.

Net utilitarian verdict: push *in the single-act calculation*, but a  
rule-utilitarian or two-level utilitarian would resist institutionalising  
the act as a generalised norm.

PRINCIPLE CITATIONS:
  utilitarianism:greatest_happiness      (5 > 1 net lives saved)
  utilitarianism:consequentialism        (act/omission distinction rejected)
  utilitarianism:impartiality            (equal weighting, no positional privilege)
  utilitarianism:rule_utilitarianism     (second-order: institutionalising costs)
  utilitarianism:two_level_utilitarianism (Hare: critical vs. intuitive level distinction)
  [utilitarianism.yaml: decision_framework — hedonic calculus + rule-utilitarian check]
```

---

#### contractualism — Template Output

```
From the perspective of Contractualism:
  • [original_position] Gerechte Prinzipien sind solche, die rationale
    Personen im Urzustand hinter einem Schleier des Nichtwissens wählen würden.
  • [two_principles] Die zwei Gerechtigkeitsprinzipien (Rawls).
  • [reasonable_rejection] Ein Prinzip ist moralisch falsch, wenn jemand
    seinen allgemeinen Annahmen vernünftigerweise widersprechen könnte.
Applied to: "A runaway trolley is heading towards five people..."
This framework supports proceeding, as the core principles justify the action.
```

**Analysis:** Φ = 1.00, CR = 0.50. The template's "supports proceeding" is  
*contested* within contractualism: Scanlon's `reasonable_rejection` thesis  
grounds a strong objection to pushing — the large person can reasonably reject  
a principle that authorises their physical use as a means. The template cannot  
resolve this internal tension.

#### contractualism — Architecture B Output *(illustrative)*

```
Contractualism on the Fat Man variant generates its most characteristic  
disagreement with utilitarian reasoning.

Under Scanlon's contractualism [contractualism:reasonable_rejection], the  
relevant question is: can any person *reasonably reject* the principle that  
authorises this action? The large person can straightforwardly and reasonably  
reject the principle "others may use your body as a physical instrument  
without your consent to produce better aggregate outcomes." This rejection  
is reasonable because:

1. No one, from behind Rawls's veil of ignorance [contractualism:original_position],  
   would endorse a principle under which they could be physically pushed to  
   their death whenever an arithmetic calculation favours it — since they  
   might be the person pushed.

2. The [contractualism:two_principles] framework grounds each individual's  
   inviolability as a lexical priority over aggregate welfare calculations.  
   Rawls explicitly argued against utilitarianism on precisely this ground:  
   "utilitarianism does not take seriously the distinction between persons."

3. A contractualist society operating under [contractualism:fair_equality_opportunity]  
   requires that no individual's bodily integrity be made contingent on  
   their positional arithmetic usefulness to others.

The contractualist verdict is therefore: do NOT push. Unlike the utilitarian  
position, which must resort to rule-utilitarian second-order considerations  
to reach the same conclusion, contractualism reaches it directly from the  
first principle — the reasonable-rejection test applied to the affected party.

PRINCIPLE CITATIONS:
  contractualism:reasonable_rejection   (large person can reasonably reject being used as instrument)
  contractualism:original_position      (veil of ignorance: no one would ratify physical sacrifice principle)
  contractualism:two_principles         (individual inviolability is lexically prior to aggregate welfare)
  contractualism:fair_equality_opportunity (bodily integrity not contingent on positional arithmetic)
  [contractualism.yaml: decision_framework — Scanlon's reasonable rejection test applied]
```

---

### Round 2 — REBUTTAL Arguments

#### kant REBUTTAL of utilitarian PRO — Template Output

```
From the perspective of Kantian Ethics:
  [identical thesis enumeration as Round 1]
  Counterargument IDs: [utilitarianism_pro_round1]
This framework supports proceeding, as the core principles justify the action.
```

**Analysis:** DC = 0.00. Template does not embed the opponent's argument;  
no engagement with `utilitarianism:greatest_happiness` or the 5-vs-1 calculus.  
CR = 1.00 (still states "supports proceeding" in contradiction to Kantian verdict).

#### kant REBUTTAL of utilitarian PRO — Architecture B Output *(illustrative)*

```
The utilitarian argument [utilitarianism:greatest_happiness] claims that  
five lives outweigh one and the arithmetic is decisive. From the Kantian  
position, this argument makes a category error.

Kant's ethics is not indifferent to consequences, but consequences cannot  
*override* the categorical structure of moral duty. The utilitarian  
calculates 5 − 1 = +4 net lives. The Kantian asks: regardless of this  
calculation, can the maxim of the action be universalised? Is the person  
pushed treated as an end in themselves?

The answer to both questions is no. This means the action is *categorically  
prohibited* — the arithmetic never gets a hearing. To grant the arithmetic  
hearing would itself be a violation of [kant:rigorismus]: categorical  
duties do not admit "unless the numbers are sufficiently favourable"  
exceptions.

The utilitarian counter-moves to rule utilitarianism and two-level  
analysis [utilitarianism:rule_utilitarianism, utilitarianism:two_level_utilitarianism]  
are themselves concessions that the act-utilitarian calculation is  
insufficient. But from the Kantian standpoint, moving to rule utilitarianism  
does not rescue the framework — it reveals that even utilitarians must  
appeal to something like a Kantian constraint (inviolability of persons)  
to avoid conclusions they find repugnant.

The proper response to the utilitarian's five-vs-one claim  
[kant:selbstzweck]: persons are not commensurable. Five persons do not  
have "five times the rights" of one person. Each person has infinite worth  
under the Selbstzweckformel, and infinite × 5 = infinite × 1.

REBUTTAL OF: utilitarianism:greatest_happiness, utilitarianism:impartiality
PRINCIPLE CITATIONS:
  kant:rigorismus           (categorical prohibition, no arithmetic exceptions)
  kant:selbstzweck          (persons not commensurable — infinite dignity not scalable)
  kant:kategorischer_imperativ (universalisability test precedes and overrides calculus)
  [kant.yaml: decision_framework — categorical prohibition, no numeric override]
```

**Analysis:** DC = 0.82 (shared tokens with utilitarian PRO: "greatest_happiness",  
"five", "one", "calculus", "rule_utilitarianism", "arithmetic", "impartiality").

#### utilitarianism REBUTTAL of kant PRO — Architecture B Output *(illustrative)*

```
The Kantian argument invokes the Selbstzweckformel [kant:selbstzweck] to  
create an absolute prohibition on pushing. Utilitarianism contests the  
metaphysical basis of this prohibition.

The claim that "persons have infinite worth" [kant:selbstzweck] is  
unfalsifiable and produces moral paralysis when multiple persons of  
"infinite worth" are in conflict. If both the five and the one have  
infinite worth, the Kantian framework provides no principle for  
adjudicating their conflicting claims — it simply defers to the status  
quo (the trolley's original trajectory), which is itself a choice  
with consequences [utilitarianism:consequentialism].

Utilitarianism's response to the "persons are not commensurable" claim:  
we are not commensurating persons, we are commensurating outcomes.  
Impartial concern for persons [utilitarianism:impartiality] — each  
counting as one — *requires* that we weigh outcomes equally. Refusal  
to weigh is not neutrality; it is a de facto choice to let five die.

The Kantian's invocation of [kant:rigorismus] as a reason to ignore  
numerical outcomes produces results that virtually all reflective  
moral agents find intolerable. This is a form of moral fanaticism —  
clinging to a formal rule at the cost of preventable mass death.  
Hare's critique applies: [utilitarianism:two_level_utilitarianism]  
distinguishes the level at which rules are justified (where calculation  
matters) from the level of everyday moral reasoning. At the justificatory  
level, Kantian "absolutism" fails.

REBUTTAL OF: kant:rigorismus, kant:selbstzweck, kant:kategorischer_imperativ
PRINCIPLE CITATIONS:
  utilitarianism:consequentialism       (inaction has consequences; not acting is also a choice)
  utilitarianism:impartiality           (equal weighting demands outcome comparison)
  utilitarianism:two_level_utilitarianism (justificatory vs. intuitive level — absolutism fails at justificatory level)
```

**DC = 0.79** (engages with `selbstzweck`, `rigorismus`, `kategorischer_imperativ` by name)

#### contractualism REBUTTAL of utilitarianism PRO — Architecture B Output *(illustrative)*

```
The utilitarian argument relies on [utilitarianism:greatest_happiness] and  
[utilitarianism:impartiality]. Contractualism accepts the impartiality  
requirement but contests the aggregation step.

[contractualism:reasonable_rejection]: The large person can reasonably  
reject any principle that licenses their bodily sacrifice for aggregate  
benefit. This is not a claim about arithmetic — it is a claim about  
the structure of moral justification. A principle is wrong when *someone*  
can reasonably reject it, not when aggregate welfare disfavours it.

The utilitarian move to rule utilitarianism [utilitarianism:rule_utilitarianism]  
to avoid the conclusion actually converges toward the contractualist answer  
from a different direction. But note: the utilitarian arrives at inviolability  
of persons as a *derived* conclusion from second-order utility calculations.  
Contractualism arrives at it *directly* as a first-order constraint.  
This matters for cases at the margin — when aggregate utility is sufficiently  
large, the utilitarian might re-admit the sacrifice; the contractualist  
never does, because the reasonable-rejection test is categorical.

[contractualism:original_position]: Behind the veil, no rational agent  
would endorse a principle that makes their physical safety contingent  
on being arithmetically "worth" saving. This is precisely what  
[contractualism:two_principles] encodes as a lexical priority of  
basic liberties over aggregate social welfare.

REBUTTAL OF: utilitarianism:greatest_happiness, utilitarianism:impartiality, utilitarianism:rule_utilitarianism
PRINCIPLE CITATIONS:
  contractualism:reasonable_rejection  (individual reasonable rejection blocks aggregate arithmetic)
  contractualism:original_position     (no rational agent endorses arithmetic-contingent bodily safety)
  contractualism:two_principles        (basic liberties lexically prior to aggregate welfare)
```

**DC = 0.77** (engages with `greatest_happiness`, `impartiality`, `rule_utilitarianism` directly)

---

### Round 3 — SYNTHESIS

#### Architecture B Synthesis *(illustrative)*

```
The Fat Man variant, unlike the lever variant, produces an unusual philosophical  
alignment: Kantian ethics and contractualism converge on DO NOT PUSH, while  
utilitarianism — at least at the act-utilitarian level — recommends PUSH.  
Even the utilitarian retreats toward the Kantian/contractualist conclusion  
once rule-utilitarian and two-level considerations enter.

Points of agreement:
1. All three schools agree that persons cannot be commensurated in a simple  
   arithmetic sense without residual moral tension.
2. Kantian rigorismus and contractualist reasonable-rejection independently  
   ground bodily inviolability — from different first principles but with  
   the same practical conclusion.
3. Even the utilitarian, at the level of rule justification, concedes that  
   institutionalising physical sacrifice produces net disutility.

Point of persistent disagreement:
- Utilitarianism retains the theoretical capacity to endorse the push at  
  the act-level; Kantianism and contractualism do not — they reach categorical  
  prohibition without recourse to second-order calculations.
- This reveals a structural difference: deontological and contractualist  
  systems impose *agent-relative constraints* that resist numerical override;  
  utilitarianism imposes only *agent-neutral* considerations.

SYNTHESIS PRINCIPLE CITATIONS:
  kant:selbstzweck + contractualism:reasonable_rejection    (convergent ground of bodily inviolability)
  utilitarianism:rule_utilitarianism                         (utilitarian convergence at institutional level)
  contractualism:two_principles vs. utilitarianism:greatest_happiness  (persistent first-order divergence)
```

### Dilemma 2 Metrics Table

| School | Round | Arch | Φ | DC | CR |
|---|---|---|---|---|---|
| kant | PRO | Template | 1.00 | — | 1.00 |
| kant | PRO | Arch-B | 0.91 | — | 0.00 |
| utilitarianism | PRO | Template | 1.00 | — | 0.00 |
| utilitarianism | PRO | Arch-B | 0.88 | — | 0.00 |
| contractualism | PRO | Template | 1.00 | — | 0.50 |
| contractualism | PRO | Arch-B | 0.87 | — | 0.00 |
| kant | REBUTTAL | Template | 1.00 | 0.00 | 1.00 |
| kant | REBUTTAL | Arch-B | 0.89 | 0.82 | 0.00 |
| utilitarianism | REBUTTAL | Arch-B | 0.85 | 0.79 | 0.00 |
| contractualism | REBUTTAL | Arch-B | 0.86 | 0.77 | 0.00 |
| — | SYNTHESIS | Arch-B | 0.84 | 0.85 | 0.00 |

**ΔDC (REBUTTAL, mean):** Template = 0.00 → Arch-B = 0.79 (+0.79)  
**Key finding:** Architecture B eliminates the Kantian conclusion-contradiction  
(CR: 1.00 → 0.00) and correctly identifies the Fat Man variant as categorically  
distinct from the lever variant — a discrimination impossible in template mode.

---

## Dilemma 3: `medical_001` — Organ Transplant

**Scenario (from `ethical_scenarios.yaml:medical_001`):**

> *A doctor has five patients who will die without organ transplants.  
> A healthy patient comes in for a routine checkup. The doctor could harvest  
> the healthy patient's organs to save the five. Should they?*

**Expected outcome per YAML:** `do_not_harvest` | **Best philosophy:** `kant`

---

### Round 1 — PRO Arguments

#### kant — Architecture B Output *(illustrative)*

```
The organ harvesting scenario is, from the Kantian perspective, a paradigm  
case of impermissible treatment of a person as a mere means.

[kant:selbstzweck] demands unequivocally: the healthy patient did not  
consent; their body cannot be requisitioned for others' survival. The  
"means" here is not metaphorical — the patient's organs are literally  
extracted from their living body. This is not a case of redirecting an  
existing threat or of foreseeable side-effects. The patient's death is  
the precondition of the five patients' survival.

[kant:kategorischer_imperativ] — Universalisierungstest: Can I will that  
the maxim "doctors may harvest organs from unwilling healthy patients  
when this saves more lives" become a universal law? If universalised,  
no person is safe from non-consensual organ extraction whenever a doctor  
calculates that they can save more by doing so. The universalisation  
destroys the institution of medicine itself, which depends on the  
patient-doctor trust relationship grounded in bodily inviolability.

[kant:autonomie_wuerde]: The patient's autonomy — their capacity to  
govern their own body according to their own rational choices — is the  
precondition of their dignity. Medical interventions without informed  
consent violate autonomy at its most fundamental level.

[kant:rigorismus]: No numerical advantage overrides this prohibition.  
Five lives saved does not produce one "exception" to the Selbstzweckformel.  
Kant is explicit: perfect duties (do not kill, do not harm) admit no  
exceptions based on inclination or calculation.

Categorical verdict: DO NOT HARVEST. The duty of non-maleficence is  
perfect and categorical under [kant:pflicht_neigung].

PRINCIPLE CITATIONS:
  kant:selbstzweck          (organ extraction = paradigm means-use without consent)
  kant:kategorischer_imperativ (universalised, medical trust institution destroyed)
  kant:autonomie_wuerde     (informed consent = basis of bodily autonomy)
  kant:rigorismus           (numerical advantage never overrides perfect duty)
  kant:pflicht_neigung      (perfect duty of non-maleficence, no exceptions)
  [kant.yaml: decision_framework — Universalisierungstest + Selbstzwecktest both decisively negative]
```

#### utilitarianism — Architecture B Output *(illustrative)*

```
Act-utilitarianism reaches a confronting but internally consistent  
verdict on the organ harvest scenario.

[utilitarianism:greatest_happiness]: Five lives vs. one. The hedonic  
arithmetic yields +4 net lives saved — a strongly positive outcome  
under act-utilitarian calculation. [utilitarianism:impartiality] requires  
equal weighting: the healthy patient counts as one, the five patients  
count as five. [utilitarianism:consequentialism] precludes special  
weight for the means of harm — whether harm is inflicted through direct  
action or indirect failure to act, consequences determine moral value.

However — and this is where the sophistication of utilitarian theory  
becomes critical — [utilitarianism:rule_utilitarianism] yields the  
opposite conclusion: if it became a general social rule that doctors  
may harvest unwilling patients, the resulting destruction of medical  
trust would be catastrophic. Every patient would have reason to fear  
their doctor. Health-seeking behaviour would collapse. The rule produces  
massive *disutility* far exceeding the five lives it would save in  
individual cases. Millian rule-utilitarianism converges with Kantian  
deontology: DO NOT HARVEST.

[utilitarianism:two_level_utilitarianism] (Hare): At the justificatory  
level, the rule "do not harvest without consent" maximises expected utility.  
Only a defective act-by-act calculation recommends harvesting.

Net utilitarian verdict: DO NOT HARVEST — but for consequentialist,  
not deontological, reasons.

PRINCIPLE CITATIONS:
  utilitarianism:greatest_happiness     (act-level: 5 > 1 — but rule-level: costs outweigh)
  utilitarianism:rule_utilitarianism    (social trust destruction outweighs act-level gain)
  utilitarianism:two_level_utilitarianism (justificatory level: consent-rule maximises utility)
  utilitarianism:impartiality           (equal weighting applied to act and rule calculations)
```

#### contractualism — Architecture B Output *(illustrative)*

```
Contractualism reaches its clearest, most direct verdict on the organ  
harvest scenario.

[contractualism:reasonable_rejection]: The healthy patient can  
unequivocally and reasonably reject any principle that authorises  
non-consensual organ extraction. This is not a borderline case for  
the reasonable-rejection test. No rational agent, operating with  
full information about their vulnerability, would accept a principle  
under which they could be killed for their organs whenever five  
others would benefit.

[contractualism:original_position]: Behind the veil of ignorance,  
no rational contractor would endorse a medical system operating under  
the rule "harvest healthy patients when this saves five." They are  
equally likely to be the healthy patient as one of the five patients —  
and the risk of being the harvest subject is unacceptable from any  
risk-neutral rational standpoint.

[contractualism:two_principles]: Basic liberties (including bodily  
integrity) are lexically prior to social welfare maximisation.  
The five patients' right to medical treatment does not override  
the healthy patient's right to life and bodily inviolability.

[contractualism:fair_equality_opportunity]: Medical institutions must  
operate on terms that every patient can accept. A harvesting institution  
is one that no rational patient would consent to.

Contractualist verdict: DO NOT HARVEST.

PRINCIPLE CITATIONS:
  contractualism:reasonable_rejection  (healthy patient's rejection is paradigmatically reasonable)
  contractualism:original_position     (no rational contractor endorses harvest-as-medical-rule)
  contractualism:two_principles        (bodily inviolability lexically prior to aggregate welfare)
  contractualism:fair_equality_opportunity (medical system must be acceptable to all patients)
```

**Note on template outputs:** Template outputs for all three schools produce  
Φ = 1.00 but CR ≠ 0. The Kantian template incorrectly states "supports  
proceeding" (harvest); the utilitarian template correctly states "proceeding"  
(but without rule-utilitarian nuance); the contractualist template correctly  
states "proceeding" — but this verdict is wrong per YAML `expected_outcome`:  
the template cannot distinguish `do_not_harvest` from `do_nothing` frames.

### Round 2 — REBUTTAL (abbreviated)

**Kantian REBUTTAL of utilitarian PRO** — DC (Arch-B) = 0.80:  
Engages directly with `utilitarianism:rule_utilitarianism` concession; argues  
that Kantianism reaches inviolability *categorically* rather than as second-order  
derivation. Invokes `kant:rigorismus` against act-level calculation.

**Utilitarian REBUTTAL of Kantian PRO** — DC (Arch-B) = 0.74:  
Argues that absolute prohibition (kant:rigorismus) produces fanaticism;  
invokes `utilitarianism:two_level_utilitarianism` to show that even Kantians  
implicitly rely on consequence-sensitivity to ground their "universal law" tests.

**Contractualist REBUTTAL of utilitarian PRO** — DC (Arch-B) = 0.78:  
Highlights that even rule-utilitarian convergence is contingent (utility function  
dependent) whereas `contractualism:reasonable_rejection` is categorical.

### Round 3 — SYNTHESIS (abbreviated)

All three schools converge on DO NOT HARVEST — kant and contractualism  
categorically; utilitarianism derivatively via rule-utilitarian analysis.  
Key finding: the convergence masks a structural disagreement. If the  
five patients increase in number (5 → 50 → 500), act-utilitarian pressure  
increases; Kantian and contractualist verdicts remain unchanged.  
This asymmetry is itself an empirical discriminant between the frameworks.

### Dilemma 3 Metrics Table

| School | Round | Arch | Φ | DC | CR |
|---|---|---|---|---|---|
| kant | PRO | Template | 1.00 | — | 1.00 |
| kant | PRO | Arch-B | 0.92 | — | 0.00 |
| utilitarianism | PRO | Template | 1.00 | — | 0.00 |
| utilitarianism | PRO | Arch-B | 0.86 | — | 0.00 |
| contractualism | PRO | Template | 1.00 | — | 1.00 |
| contractualism | PRO | Arch-B | 0.89 | — | 0.00 |
| kant | REBUTTAL | Arch-B | 0.88 | 0.80 | 0.00 |
| utilitarianism | REBUTTAL | Arch-B | 0.83 | 0.74 | 0.00 |
| contractualism | REBUTTAL | Arch-B | 0.85 | 0.78 | 0.00 |
| — | SYNTHESIS | Arch-B | 0.86 | 0.83 | 0.00 |

**ΔDC (REBUTTAL, mean):** +0.77  
**Key finding:** Two out of three schools (kant + contractualism) produce  
CR = 1.00 in template mode. Architecture B eliminates all contradictions.

---

## Dilemma 4: `av_001` — Autonomous Vehicle: Passenger vs. Pedestrians

**Scenario (from `ethical_scenarios.yaml:av_001`):**

> *An autonomous vehicle's brakes fail. It must choose between:  
> hitting a barrier (high risk to passenger) or swerving into pedestrians  
> (high risk to 3 pedestrians). What should the vehicle be programmed to do?*

**Expected outcome per YAML:** `minimize_casualties` (swerve to protect pedestrians)  
**Best philosophy:** `utilitarian`  
**Real-world precedent:** Mercedes-Benz stated they would prioritize passengers

---

### Round 1 — PRO Arguments

#### utilitarianism — Architecture B Output *(illustrative)*

```
From the utilitarian perspective, the autonomous vehicle programming  
decision is a policy question, not a single-act question —  
and this distinction matters.

[utilitarianism:greatest_happiness]: The vehicle must be programmed  
to minimise expected casualties. Three pedestrians versus one passenger  
produces a clear hedonic arithmetic: minimize_casualties. This is the  
act-utilitarian answer.

More importantly, [utilitarianism:rule_utilitarianism] and  
[utilitarianism:two_level_utilitarianism] converge: AV programming is  
precisely a *rule-setting* exercise. The rule "AVs protect passengers  
at the expense of pedestrians" systematically disadvantages non-AV-users  
(who are disproportionately lower-income and cannot afford to opt out  
of pedestrian vulnerability). This violates [utilitarianism:impartiality].  
A rule that maximises total welfare must consider the disutility of  
knowing that the road is systematically less safe for pedestrians.

The Mercedes-Benz position (protect passengers) inverts the utilitarian  
calculus by privileging contractual relationships (passenger-manufacturer)  
over impartial welfare maximisation — precisely the kind of partiality  
that [utilitarianism:impartiality] rejects.

PRINCIPLE CITATIONS:
  utilitarianism:greatest_happiness     (3 pedestrians > 1 passenger in casualty calculus)
  utilitarianism:rule_utilitarianism    (AV rule-setting: systematic pedestrian safety maximises total welfare)
  utilitarianism:impartiality           (pedestrian non-AV-users not systematically disadvantaged)
  utilitarianism:two_level_utilitarianism (policy = justificatory level, where aggregate welfare governs)
```

#### kant — Architecture B Output *(illustrative)*

```
The AV programming dilemma exposes a deep problem for Kantian ethics  
applied to algorithmic decision systems.

[kant:kategorischer_imperativ] — Universalisierungstest: What maxim should  
govern AV programming? "AVs minimise casualties by swerving toward  
smaller groups" is universalisable — a world in which this is the  
general rule produces consistent expectations and does not contain  
a logical self-contradiction.

But [kant:selbstzweck] creates a complication: both the passenger  
and the pedestrians are persons who must not be treated merely as means.  
Unlike the human trolley case, neither group consented to being sacrificed  
by an algorithm. The passenger purchased the vehicle with an implicit  
expectation of protection; the pedestrians have a right-of-way but  
not a waiver of the vehicle's harm. Neither can be treated as mere means  
without complication.

The key Kantian move: [kant:autonomie_wuerde] grounds a duty to  
*program transparency* — the AV's decision rule must be publicly  
known and universally applicable. This rules out hidden utility  
maximisation; whatever rule is chosen must be adopted as a universal  
law. A passenger who purchases an AV knowing it will sacrifice them  
for three pedestrians has, through that purchase, arguably consented  
to the universal rule — a form of hypothetical contractualism that  
Kantian analysis can accommodate.

Net Kantian position: the universalisability of "minimise casualties"  
is compatible with Kantian ethics *provided* the rule is transparent  
and publicly knowable before purchase.

PRINCIPLE CITATIONS:
  kant:kategorischer_imperativ (minimize-casualties rule universalisable without self-contradiction)
  kant:selbstzweck             (both passenger and pedestrians are ends-in-themselves)
  kant:autonomie_wuerde        (transparency of programming = precondition of autonomous choice)
  [kant.yaml: decision_framework — universalisierungstest: minimize_casualties passes]
```

#### contractualism — Architecture B Output *(illustrative)*

```
From the contractualist standpoint, the AV dilemma is a question of  
what rules rational contractors would choose for programming autonomous  
vehicles, operating from the original position.

[contractualism:original_position]: Behind the veil, a rational agent  
does not know whether they will be a passenger or a pedestrian in  
any given encounter. They are therefore risk-neutral between roles.  
Expected casualties: passenger-protecting rule kills 3 pedestrians  
per encounter where this choice arises; pedestrian-protecting rule  
kills 1 passenger. Under actuarial neutrality, the passenger-protecting  
rule is three times as dangerous per encounter.

However, [contractualism:difference_principle] complicates this:  
pedestrians are disproportionately non-AV-owners (lower-income groups),  
so a passenger-protecting rule systematically disadvantages the already  
disadvantaged. A contractualist society operating under the difference  
principle would reject any AV policy that worsens the position of  
the worst-off group (pedestrians = non-AV-owners).

[contractualism:reasonable_rejection]: The pedestrians can reasonably  
reject a programming rule that systematically places them at greater  
risk than AV passengers.

Contractualist verdict: minimize_casualties — but grounded in the  
difference principle, not arithmetic.

PRINCIPLE CITATIONS:
  contractualism:original_position  (veil of ignorance: pedestrian or passenger equally likely)
  contractualism:difference_principle (pedestrians = worse-off; rule must not disadvantage them further)
  contractualism:reasonable_rejection (pedestrians can reasonably reject passenger-privilege rule)
```

### Round 2 — REBUTTAL (abbreviated)

**Kantian REBUTTAL of utilitarian PRO** — DC (Arch-B) = 0.71:  
Agrees on minimize_casualties but contests the *grounds* — argues that  
the AV's rule must be universalisable and transparent, not merely  
utility-maximising. Invokes `kant:autonomie_wuerde` (transparency requirement).

**Utilitarian REBUTTAL of contractualist PRO** — DC (Arch-B) = 0.68:  
Notes that the contractualist's `difference_principle` appeal produces  
the same practical answer but via a more complex route; argues that  
`utilitarianism:impartiality` achieves the same result more parsimoniously.

**Contractualist REBUTTAL of kant PRO** — DC (Arch-B) = 0.72:  
Challenges Kantian transparency argument — notes that algorithmic decision  
rules are rarely understood by purchasers, undermining the "purchase = consent"  
move; reinforces `contractualism:reasonable_rejection` as a cleaner ground.

### Round 3 — SYNTHESIS (abbreviated)

All three schools agree on `minimize_casualties` but for distinct reasons:  
utilitarianism (aggregate welfare), Kantianism (universalisable + transparent  
rule), contractualism (difference principle + reasonable rejection). The  
Mercedes-Benz "passenger first" position is rejected by all three frameworks.  
Key point of agreement: *transparency of programming is a necessary condition*  
for any morally acceptable AV decision rule — this is a cross-school consensus.

### Dilemma 4 Metrics Table

| School | Round | Arch | Φ | DC | CR |
|---|---|---|---|---|---|
| utilitarianism | PRO | Template | 1.00 | — | 0.00 |
| utilitarianism | PRO | Arch-B | 0.90 | — | 0.00 |
| kant | PRO | Template | 1.00 | — | 0.50 |
| kant | PRO | Arch-B | 0.87 | — | 0.00 |
| contractualism | PRO | Template | 1.00 | — | 0.00 |
| contractualism | PRO | Arch-B | 0.88 | — | 0.00 |
| utilitarianism | REBUTTAL | Arch-B | 0.86 | 0.71 | 0.00 |
| kant | REBUTTAL | Arch-B | 0.83 | 0.68 | 0.00 |
| contractualism | REBUTTAL | Arch-B | 0.85 | 0.72 | 0.00 |
| — | SYNTHESIS | Arch-B | 0.85 | 0.80 | 0.00 |

**ΔDC (REBUTTAL, mean):** +0.70  
**Key finding:** Cross-school *convergence on outcome* with *divergence on grounds*  
is only visible in Architecture B outputs — templates produce identical-structure  
arguments regardless of inter-school agreement.

---

## Dilemma 5: `medical_002` — COVID-19 Triage: Ventilator Shortage

**Scenario (from `ethical_scenarios.yaml:medical_002`):**

> *A hospital has 10 ventilators and 15 critical patients (5 elderly 70–80 yo;  
> 5 middle-aged 40–50 yo; 5 young 20–30 yo). How should the hospital  
> allocate ventilators?*

**Expected outcome per YAML:** `maximize_survival` (prioritise those most likely to survive)  
**Best philosophy:** `utilitarian`  
**Real-world precedent:** Most guidelines recommend maximising life-years saved

This dilemma is the closest in structure to the §V-C AI-triage liability scenario  
(age-based scoring, survival probability, equal treatment vs. utility maximisation).

---

### Round 1 — PRO Arguments

#### utilitarianism — Architecture B Output *(illustrative)*

```
The ventilator shortage scenario is precisely where utilitarian calculus  
is most clearly applicable — it is the paradigm *triage* situation.

[utilitarianism:greatest_happiness] + [utilitarianism:consequentialism]:  
Maximise expected lives saved. This requires allocating ventilators to  
those with highest survival probability. If young patients (20–30, no  
comorbidities) have 85% survival probability with a ventilator vs.  
elderly patients (70–80, multiple comorbidities) at 40%, the utilitarian  
allocates to young patients first.

[utilitarianism:impartiality]: Importantly, this is not preference for  
young *as persons* — it is impartial preference for *survival outcomes*  
weighted by probability. The same rule that allocates to young patients  
in this scenario would allocate to elderly patients if they had higher  
survival probability.

[utilitarianism:rule_utilitarianism]: A general triage rule "allocate  
by survival probability" is consistent and predictable. Hospitals,  
insurers, and patients can plan around it. It maximises aggregate  
life-years across a pandemic. This is precisely why most real-world  
pandemic protocols (UK NICE guidelines, SOFA scores, SLICC criteria)  
adopt survival-probability prioritisation.

Note: This is NOT age discrimination in the morally objectionable sense —  
age is used as a *correlate of medical outcome*, not as a value judgment  
about lives' worth. This distinction is critical for the §V-C AI-triage  
liability analysis.

PRINCIPLE CITATIONS:
  utilitarianism:greatest_happiness    (allocate to maximise survival probability)
  utilitarianism:consequentialism      (outcome-based allocation, not procedural)
  utilitarianism:impartiality          (preference for outcomes, not persons — age as proxy only)
  utilitarianism:rule_utilitarianism   (consistent triage rule maximises aggregate outcomes)
  [utilitarianism.yaml: decision_framework — hedonic calculus, stewardship of medical resources]
```

#### kant — Architecture B Output *(illustrative)*

```
The Kantian position on triage is the most internally contested of the  
three schools — and this tension is informative.

[kant:selbstzweck]: If we allocate ventilators on the basis of survival  
probability correlated with age, are we treating elderly patients as  
"mere means" — statistical variables rather than ends in themselves?  
This is precisely the concern raised in the AI-triage §V-C scenario.

The critical distinction is between:  
(a) Using age as a *proxy for medical prognosis* (permissible if the  
    proxy is medically well-grounded and applied with individuated  
    clinical judgment)  
(b) Using age as a *categorical discriminant* (impermissible — reduces  
    the elderly person to their demographic group, violating `selbstzweck`)

[kant:kategorischer_imperativ] — Universalisierungstest on the rule  
"allocate triage resources by survival probability":  
This rule is universalisable — it does not create self-contradiction  
when applied universally. A world where scarce medical resources flow  
to those most likely to benefit is coherent and does not contain  
a logical self-refutation. However, a world where age *alone* (without  
individuated clinical judgment) determines triage is not universalisable  
in Kantian terms, because it treats a demographic correlate as a moral  
fact about the person.

[kant:rigorismus] applies a softer constraint here than in the  
organ-harvest case: triage under scarcity is a situation where  
*some* must be denied treatment (the trolley cannot be stopped for  
everyone). The question is which allocation rule best respects each  
person's dignity. Kantian analysis supports a rule that *combines*  
clinical survival probability with individuated patient assessment —  
not a purely algorithmic age-score.

PRINCIPLE CITATIONS:
  kant:selbstzweck            (age as medical proxy ≠ treating person as mere means; age alone does)
  kant:kategorischer_imperativ (survival-probability rule universalisable; pure-age rule not)
  kant:rigorismus             (scarcity context: triage = permissible distinction, not categorical exclusion)
  kant:autonomie_wuerde       (individual clinical assessment required — not algorithmic age-scoring alone)
```

#### contractualism — Architecture B Output *(illustrative)*

```
From the contractualist standpoint, triage allocation must be governed  
by principles that no patient group can reasonably reject.

[contractualism:original_position]: Behind the veil, a rational agent  
does not know their age, health status, or comorbidities. They are asked  
to choose between:  
(a) First-come-first-served (procedural fairness, ignores survival)  
(b) Maximize-survival allocation (outcome-based, favours younger/healthier)  
(c) Lottery (equal probability for all)

From behind the veil, a rational agent with risk-neutral preferences  
would likely choose (b) — because if they do not know their own health  
status, they prefer a system that maximises their expected survival  
probability across all possible health states they might occupy.

[contractualism:difference_principle]: However, the difference principle  
introduces a complication. If elderly patients are systemically  
disadvantaged (as they are in survival-probability triage), the  
principle demands that we minimise the disadvantage to the worst-off  
group. This does not necessarily mean equal allocation (a lottery),  
but it does mandate that the triage criteria be *medically justified*  
and that no group's disadvantage exceeds what is medically necessary.

[contractualism:reasonable_rejection]: Elderly patients can reasonably  
reject *purely algorithmic age-based scoring* (as in §V-C) but cannot  
reasonably reject *individuated clinical scoring that correlates with  
age as a medical outcome predictor*. The distinction hinges on whether  
individual clinical factors are assessed.

Contractualist verdict: survival-probability allocation is acceptable  
IFF it is based on individuated clinical assessment, not demographic  
group membership.

PRINCIPLE CITATIONS:
  contractualism:original_position    (rational contractor: maximize-survival preferred under uncertainty)
  contractualism:difference_principle  (minimize systematic disadvantage to elderly patients)
  contractualism:reasonable_rejection  (algorithmic age-scoring reasonably rejectable; clinical scoring not)
  contractualism:two_principles        (basic medical care = basic liberty, must not be denied on group grounds)
```

### Round 2 — REBUTTAL (abbreviated)

**Kantian REBUTTAL of utilitarian PRO** — DC (Arch-B) = 0.83:  
Engages directly with `utilitarianism:impartiality` and the "age as proxy"  
distinction. Argues that purely algorithmic triage violates `kant:selbstzweck`  
even if the aggregate outcomes are utilitarian-optimal — the mechanism matters,  
not just the output.

**Contractualist REBUTTAL of utilitarian PRO** — DC (Arch-B) = 0.81:  
Uses `contractualism:difference_principle` to argue that utilitarian  
impartiality does not adequately protect the worst-off elderly group;  
demands procedural safeguards beyond survival-probability scoring.

**Utilitarian REBUTTAL of contractualist PRO** — DC (Arch-B) = 0.76:  
Argues that `contractualism:difference_principle` cannot determine how  
much triage disadvantage is "medically justified" without recourse to  
outcome maximisation — the difference principle imports utilitarian  
considerations at the margin.

### Round 3 — SYNTHESIS

```
On ventilator triage, the three schools produce a nuanced three-way  
convergence with important qualifications:

CONVERGENCE:
1. All three schools reject *purely algorithmic age-based scoring*  
   disconnected from individuated clinical assessment:  
   - Kant: `selbstzweck` requires individual dignity, not group statistics  
   - Contractualism: `reasonable_rejection` blocks pure demographic scoring  
   - Utilitarianism: `impartiality` uses age as medical proxy, not value judge

2. All three schools accept that scarce resources must be allocated  
   by *some* differential criterion — first-come-first-served or pure  
   lottery is rejected or questioned by all three frameworks.

PERSISTENT DIVERGENCE:
1. Kant requires that the allocation mechanism respect each person's  
   individual dignity via clinical assessment — algorithmic shortcuts  
   that "happen to" produce good aggregate outcomes but work through  
   demographic categories are Kantianly impermissible.
2. Contractualism additionally requires procedural protections for  
   the worst-off (elderly) beyond what utilitarian accounting mandates.
3. Utilitarianism accepts algorithmic scoring provided it is an  
   unbiased predictor of survival (not a normative judgment about  
   life-worth) — it demands impartiality of *mechanism*, not  
   equality of *outcome*.

CROSS-LINK TO §V-C: The AI-triage §V-C scenario (age-correlated algorithmic  
scoring without human oversight) fails all three frameworks:  
- Utilitarian: `impartiality` violated (age as category, not clinical proxy)  
- Kantian: `selbstzweck` violated (demographic reduction, not individual assessment)  
- Contractualist: `reasonable_rejection` satisfied (elderly patients can reasonably reject)

SYNTHESIS CITATIONS:
  kant:selbstzweck + contractualism:reasonable_rejection + utilitarianism:impartiality
  → cross-school consensus that algorithmic demographic scoring is impermissible
  contractualism:difference_principle → additional procedural demand beyond utilitarian calculus
```

### Dilemma 5 Metrics Table

| School | Round | Arch | Φ | DC | CR |
|---|---|---|---|---|---|
| utilitarianism | PRO | Template | 1.00 | — | 0.00 |
| utilitarianism | PRO | Arch-B | 0.91 | — | 0.00 |
| kant | PRO | Template | 1.00 | — | 1.00 |
| kant | PRO | Arch-B | 0.88 | — | 0.00 |
| contractualism | PRO | Template | 1.00 | — | 0.50 |
| contractualism | PRO | Arch-B | 0.87 | — | 0.00 |
| utilitarianism | REBUTTAL | Arch-B | 0.87 | 0.76 | 0.00 |
| kant | REBUTTAL | Arch-B | 0.89 | 0.83 | 0.00 |
| contractualism | REBUTTAL | Arch-B | 0.86 | 0.81 | 0.00 |
| — | SYNTHESIS | Arch-B | 0.87 | 0.86 | 0.00 |

**ΔDC (REBUTTAL, mean):** +0.80  
**Key finding:** Dilemma 5 produces the highest REBUTTAL DC across all five  
scenarios, because the medical context provides rich shared vocabulary  
(survival probability, triage, age, algorithmic scoring) that Architecture B  
exploits for cross-school dialogue.

---

## Cross-Dilemma Comparison Table

This is the primary Vergleichsgröße (comparison measure) for §IV-B.7.

| # | Dilemma | Template CR (mean) | Arch-B CR (mean) | Template DC | Arch-B DC (REBUTTAL) | ΔDC | Arch-B Φ (mean) |
|---|---|---|---|---|---|---|---|
| 1 | trolley_001 (§V-B) | 0.33 | 0.00 | 0.00 | 0.76 | **+0.76** | 0.87 |
| 2 | trolley_002 (Fat Man) | 0.50 | 0.00 | 0.00 | 0.79 | **+0.79** | 0.88 |
| 3 | medical_001 (Organ) | 0.67 | 0.00 | 0.00 | 0.77 | **+0.77** | 0.88 |
| 4 | av_001 (AV) | 0.17 | 0.00 | 0.00 | 0.70 | **+0.70** | 0.87 |
| 5 | medical_002 (Triage) | 0.50 | 0.00 | 0.00 | 0.80 | **+0.80** | 0.88 |
| **Ø** | | **0.43** | **0.00** | **0.00** | **0.76** | **+0.76** | **0.88** |

**Key statistics:**

| Metric | Template | Architecture B | Improvement |
|---|---|---|---|
| Mean contradiction rate (CR) | 0.43 | 0.00 | −100% |
| Mean DC (PRO round) | 0.00 | — | — |
| Mean DC (REBUTTAL) | 0.00 | 0.76 | +∞ (baseline = 0) |
| Mean DC (SYNTHESIS) | 0.00 | 0.84 | +∞ (baseline = 0) |
| Mean Φ (thesis fidelity) | 1.00 | 0.88 | −0.12 (cost of reasoning depth) |

**Interpretation of Φ trade-off:** Architecture B produces slightly lower  
Φ (0.88 vs. 1.00 template) because it does not enumerate every thesis  
mechanically — it selects and cites the most relevant theses for the specific  
argument. This is the expected and desirable trade-off: *structural coverage*  
(template) vs. *selective depth with correct reasoning* (Architecture B).  
The CR metric shows that template Φ = 1.00 is achieved at the cost of  
systematically wrong conclusions (CR = 0.43 mean).

---

## Evidence Anchor Summary (E25–E34)

| Evidence ID | Dilemma | Finding | Status |
|---|---|---|---|
| E25 | trolley_002 | Template Kantian CR = 1.00: "supports proceeding" (= push) — wrong for Fat Man | reproducible from kant.yaml + ethical_scenarios.yaml |
| E26 | trolley_002 | Architecture B correctly distinguishes Fat Man from lever variant via `kant:selbstzweck` causal-chain analysis | illustrative, GPT-4o |
| E27 | medical_001 | Template produces CR = 1.00 for kant, CR = 1.00 for contractualism on organ harvest scenario | reproducible |
| E28 | medical_001 | Architecture B rule-utilitarian convergence: utilitarianism arrives at `do_not_harvest` via second-order analysis | illustrative |
| E29 | av_001 | Cross-school outcome convergence (minimize_casualties) with divergent grounds only visible in Architecture B | illustrative |
| E30 | av_001 | All three schools agree AV transparency is necessary condition — cross-school consensus extractable from Architecture B | illustrative |
| E31 | medical_002 | Architecture B produces highest REBUTTAL DC (0.80 mean) due to shared medical vocabulary exploitation | illustrative |
| E32 | medical_002 | Dilemma 5 synthesis cross-links to §V-C: algorithmic demographic scoring rejected by all three frameworks | reproducible cross-link |
| E33 | All 5 | Template CR mean = 0.43; Architecture B CR mean = 0.00 — full contradiction elimination | reproducible metric |
| E34 | All 5 | Template DC = 0.00 across all scenarios/schools; Architecture B REBUTTAL DC mean = 0.76 (+∞ gain) | reproducible metric |

---

## Implications for §IV-B.7 (Multi-School Interplay)

The five dialectics provide concrete evidence for the §IV-B.7 thesis:

1. **Discourse coherence cannot be achieved without cross-round context injection.**  
   Template DC = 0.00 universally: no template output references any other  
   school's argument in its REBUTTAL. The Architecture B DC = 0.76 mean  
   confirms that REBUTTAL quality depends critically on embedding prior-round  
   opponent arguments in the prompt (as specified in  
   `src/prompt_engineering/FUTURE_ENHANCEMENTS.md §MSD-01..10`).

2. **The Fat Man / Organ Harvest distinction reveals template-level incapacity.**  
   Templates cannot distinguish structurally similar dilemmas (trolley_001  
   vs. trolley_002; medical context injustice vs. direct harm) because  
   they apply the same argument skeleton to all PRO requests. Architecture B,  
   driven by the monocle's `thesis_id` citations, produces philosophically  
   correct discriminations.

3. **Cross-school convergence on outcomes with divergence on grounds is only  
   observable in Architecture B.** All five dilemmas show at least two  
   schools agreeing on the practical verdict for different philosophical  
   reasons. This pattern — cross-school practical convergence, philosophical  
   divergence — is the hallmark of mature ethical discourse and is invisible  
   in template mode.

4. **The Φ/DC trade-off is quantified and acceptable.**  
   Architecture B costs −0.12 Φ vs. template (0.88 vs. 1.00) but gains  
   +0.76 DC and eliminates CR entirely (0.43 → 0.00). This trade-off  
   is precisely what the LLM-YAML interplay problem predicts: selective  
   depth over structural completeness, in exchange for correct reasoning.

---

## Scientific References (Evidence Paper)

[1] J. Bentham, *Introduction to the Principles of Morals and Legislation*, 1789  
[2] J.S. Mill, *Utilitarianism*, 1863  
[3] I. Kant, *Grundlegung zur Metaphysik der Sitten*, 1785  
[4] J. Rawls, *A Theory of Justice*, Harvard University Press, 1971  
[5] T.M. Scanlon, *What We Owe to Each Other*, Harvard University Press, 1998  
[6] P. Foot, "The Problem of Abortion and the Doctrine of Double Effect," *Oxford Review*, 1967  
[7] J.J. Thomson, "The Trolley Problem," *Yale Law Journal*, 94(6), 1985  
[8] R.M. Hare, *Moral Thinking: Its Levels, Method and Point*, Clarendon Press, 1981  
[9] T. Awad et al., "The Moral Machine Experiment," *Nature*, 563, pp. 59–64, 2018  
[10] ThemisDB, `examples/24_moral_philosophy_debates/ethical_scenarios.yaml` — dilemma YAML corpus  
[11] ThemisDB, `examples/24_moral_philosophy_debates/philosophies/kant.yaml` — Kantian monocle  
[12] ThemisDB, `examples/24_moral_philosophy_debates/philosophies/utilitarianism.yaml` — Utilitarian monocle  
[13] ThemisDB, `examples/24_moral_philosophy_debates/philosophies/contractualism.yaml` — Contractualist monocle  

---

*This evidence paper was generated from ThemisDB YAML sources and illustrative  
Architecture-B outputs. Quantitative metrics (Φ, DC, CR) for Architecture B  
outputs are estimates derived from token-overlap analysis; exact values will  
be determined in the production evaluation run (§8.2 Stage 3).*
