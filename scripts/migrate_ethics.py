#!/usr/bin/env python3
"""
Migration script: ethics YAML profiles from plugins/ethics_ai/philosophies/
                  to assets/ethics_ai/

Applies:
  - CHANGE 1: CWB fields on main_theses entries
  - CHANGE 2: Structured cross_school_tensions (list format)
  - CHANGE 3: Routing metadata for Group B files
  - CHANGE 4: Canonical-format conversion for Group C files
"""

import os
import re
import copy
import yaml
from typing import Any, Dict, List, Optional

SRC_DIR = "/home/runner/work/ThemisDB/ThemisDB/plugins/ethics_ai/philosophies"
DST_DIR = "/home/runner/work/ThemisDB/ThemisDB/assets/ethics_ai"

# ────────────────────────────────────────────────────────────────────────────
# CWB weight block helpers
# ────────────────────────────────────────────────────────────────────────────

def _cwb_full() -> Dict:
    return {
        "token_budget": 180,
        "activation_rounds": [1, 2, 3],
        "round_role_weights": {
            "PRO": 1.0,
            "REBUTTAL": 0.9,
            "SURREBUTTAL": 0.8,
            "SYNTHESIS": 0.4,
            "META_VERDICT": 0.2,
        },
    }


def _cwb_minor() -> Dict:
    return {
        "token_budget": 120,
        "activation_rounds": [1, 2],
        "round_role_weights": {
            "PRO": 1.0,
            "REBUTTAL": 0.9,
            "SURREBUTTAL": 0.8,
            "SYNTHESIS": 0.4,
            "META_VERDICT": 0.2,
        },
    }


def add_cwb_to_theses(theses: List[Dict]) -> List[Dict]:
    """Add CWB fields after 'description' in each thesis item (idempotent)."""
    for t in theses:
        if "token_budget" in t:
            continue  # already present
        t.update(_cwb_full())
    return theses


# ────────────────────────────────────────────────────────────────────────────
# Cross-school tensions data
# ────────────────────────────────────────────────────────────────────────────

TENSIONS: Dict[str, List[Dict]] = {
    "kant": [
        {"own_thesis": "selbstzweck", "opposing_school": "utilitarianism",
         "opposing_thesis": "greatest_happiness", "tension_type": "categorical_vs_aggregate",
         "rebuttal_cite_weight": 0.9},
        {"own_thesis": "rigorismus", "opposing_school": "utilitarianism",
         "opposing_thesis": "consequentialism", "tension_type": "deontological_vs_consequentialist",
         "rebuttal_cite_weight": 0.85},
        {"own_thesis": "kategorischer_imperativ", "opposing_school": "contractualism",
         "opposing_thesis": "reasonable_rejection",
         "tension_type": "universal_law_vs_reasonable_rejection",
         "rebuttal_cite_weight": 0.6},
        {"own_thesis": "kategorischer_imperativ", "opposing_school": "islamische_ethik",
         "opposing_thesis": "maslaha", "tension_type": "deontological_vs_welfare",
         "rebuttal_cite_weight": 0.5},
    ],
    "utilitarianism": [
        {"own_thesis": "greatest_happiness", "opposing_school": "kant",
         "opposing_thesis": "selbstzweck", "tension_type": "aggregate_vs_categorical",
         "rebuttal_cite_weight": 0.9},
        {"own_thesis": "consequentialism", "opposing_school": "kant",
         "opposing_thesis": "rigorismus", "tension_type": "consequentialist_vs_deontological",
         "rebuttal_cite_weight": 0.85},
        {"own_thesis": "greatest_happiness", "opposing_school": "contractualism",
         "opposing_thesis": "original_position", "tension_type": "aggregate_vs_procedural",
         "rebuttal_cite_weight": 0.75},
        {"own_thesis": "impartiality", "opposing_school": "islamische_ethik",
         "opposing_thesis": "la_darar",
         "tension_type": "impartial_aggregation_vs_prohibitive_harm",
         "rebuttal_cite_weight": 0.6},
    ],
    "contractualism": [
        {"own_thesis": "reasonable_rejection", "opposing_school": "utilitarianism",
         "opposing_thesis": "greatest_happiness",
         "tension_type": "individual_rejection_vs_aggregate",
         "rebuttal_cite_weight": 0.9},
        {"own_thesis": "original_position", "opposing_school": "utilitarianism",
         "opposing_thesis": "impartiality",
         "tension_type": "procedural_vs_impartial_summation",
         "rebuttal_cite_weight": 0.75},
        {"own_thesis": "reasonable_rejection", "opposing_school": "kant",
         "opposing_thesis": "kategorischer_imperativ",
         "tension_type": "mutual_justification_vs_universal_law",
         "rebuttal_cite_weight": 0.6},
    ],
    "rawls": [
        {"own_thesis": "justice_as_fairness", "opposing_school": "utilitarianism",
         "opposing_thesis": "greatest_happiness",
         "tension_type": "procedural_justice_vs_aggregate_welfare",
         "rebuttal_cite_weight": 0.9},
        {"own_thesis": "difference_principle", "opposing_school": "kant",
         "opposing_thesis": "autonomie_wuerde",
         "tension_type": "distributive_vs_individual_autonomy",
         "rebuttal_cite_weight": 0.65},
        {"own_thesis": "veil_of_ignorance", "opposing_school": "islamische_ethik",
         "opposing_thesis": "maslaha",
         "tension_type": "neutral_procedure_vs_welfare_hierarchy",
         "rebuttal_cite_weight": 0.55},
    ],
    "islamische_ethik": [
        {"own_thesis": "la_darar", "opposing_school": "utilitarianism",
         "opposing_thesis": "greatest_happiness",
         "tension_type": "prohibitive_harm_vs_aggregate_maximization",
         "rebuttal_cite_weight": 0.9},
        {"own_thesis": "maqasid_al_shariah", "opposing_school": "kant",
         "opposing_thesis": "autonomie_wuerde",
         "tension_type": "divine_command_vs_rational_autonomy",
         "rebuttal_cite_weight": 0.75},
        {"own_thesis": "maslaha", "opposing_school": "contractualism",
         "opposing_thesis": "reasonable_rejection",
         "tension_type": "welfare_hierarchy_vs_mutual_justification",
         "rebuttal_cite_weight": 0.6},
    ],
    "buddhistische_ethik": [
        {"own_thesis": "ahimsa", "opposing_school": "utilitarianism",
         "opposing_thesis": "consequentialism",
         "tension_type": "absolute_nonviolence_vs_outcome_calculus",
         "rebuttal_cite_weight": 0.85},
        {"own_thesis": "karuna_mitgefuehl", "opposing_school": "kant",
         "opposing_thesis": "pflicht_neigung",
         "tension_type": "compassion_as_motivation_vs_duty_only",
         "rebuttal_cite_weight": 0.7},
        {"own_thesis": "ahimsa", "opposing_school": "nietzsche",
         "opposing_thesis": "will_to_power",
         "tension_type": "nonharm_vs_power_assertion",
         "rebuttal_cite_weight": 0.8},
    ],
    "juedische_bioethik": [
        {"own_thesis": "pikuach_nefesh", "opposing_school": "utilitarianism",
         "opposing_thesis": "greatest_happiness",
         "tension_type": "individual_life_sanctity_vs_aggregate",
         "rebuttal_cite_weight": 0.9},
        {"own_thesis": "kavod_habriot", "opposing_school": "nietzsche",
         "opposing_thesis": "will_to_power",
         "tension_type": "human_dignity_vs_value_inversion",
         "rebuttal_cite_weight": 0.8},
        {"own_thesis": "tzelem_elohim", "opposing_school": "kant",
         "opposing_thesis": "autonomie_wuerde",
         "tension_type": "divine_image_vs_rational_autonomy",
         "rebuttal_cite_weight": 0.6},
    ],
    "konfuzianismus": [
        {"own_thesis": "ren_humanitaet", "opposing_school": "kant",
         "opposing_thesis": "kategorischer_imperativ",
         "tension_type": "relational_virtue_vs_universal_formal_law",
         "rebuttal_cite_weight": 0.85},
        {"own_thesis": "li_ritualitaet", "opposing_school": "utilitarianism",
         "opposing_thesis": "consequentialism",
         "tension_type": "ritual_order_vs_outcome_calculus",
         "rebuttal_cite_weight": 0.7},
        {"own_thesis": "yi_righteousness", "opposing_school": "contractualism",
         "opposing_thesis": "original_position",
         "tension_type": "role_based_duty_vs_neutral_procedure",
         "rebuttal_cite_weight": 0.65},
    ],
    "behoerden_ethik": [
        {"own_thesis": "legalitaetsprinzip", "opposing_school": "utilitarianism",
         "opposing_thesis": "consequentialism",
         "tension_type": "rule_of_law_vs_outcome_maximization",
         "rebuttal_cite_weight": 0.9},
        {"own_thesis": "gleichbehandlungsgebot", "opposing_school": "care_ethics",
         "opposing_thesis": "care_relationships",
         "tension_type": "formal_equality_vs_contextual_care",
         "rebuttal_cite_weight": 0.7},
    ],
    "universitaere_ethik": [
        {"own_thesis": "wissenschaftsfreiheit", "opposing_school": "utilitarianism",
         "opposing_thesis": "greatest_happiness",
         "tension_type": "academic_freedom_vs_social_utility",
         "rebuttal_cite_weight": 0.8},
        {"own_thesis": "forschungsintegrität", "opposing_school": "kant",
         "opposing_thesis": "pflicht_neigung",
         "tension_type": "institutional_norms_vs_individual_duty",
         "rebuttal_cite_weight": 0.65},
    ],
    # Group C
    "nietzsche": [
        {"own_thesis": "will_to_power", "opposing_school": "kant",
         "opposing_thesis": "kategorischer_imperativ",
         "tension_type": "power_assertion_vs_categorical_duty",
         "rebuttal_cite_weight": 0.9},
        {"own_thesis": "uebermensch", "opposing_school": "utilitarianism",
         "opposing_thesis": "greatest_happiness",
         "tension_type": "individual_excellence_vs_aggregate_welfare",
         "rebuttal_cite_weight": 0.85},
    ],
    "marx": [
        {"own_thesis": "class_struggle", "opposing_school": "utilitarianism",
         "opposing_thesis": "consequentialism",
         "tension_type": "structural_conflict_vs_outcome_calculus",
         "rebuttal_cite_weight": 0.85},
        {"own_thesis": "alienation", "opposing_school": "kant",
         "opposing_thesis": "autonomie_wuerde",
         "tension_type": "structural_alienation_vs_individual_autonomy",
         "rebuttal_cite_weight": 0.7},
    ],
    "arendt": [
        {"own_thesis": "plurality", "opposing_school": "utilitarianism",
         "opposing_thesis": "greatest_happiness",
         "tension_type": "plurality_vs_aggregate_maximization",
         "rebuttal_cite_weight": 0.8},
        {"own_thesis": "vita_activa", "opposing_school": "kant",
         "opposing_thesis": "pflicht_neigung",
         "tension_type": "action_in_world_vs_inner_duty",
         "rebuttal_cite_weight": 0.6},
    ],
    "schopenhauer": [
        {"own_thesis": "compassion_ethics", "opposing_school": "kant",
         "opposing_thesis": "pflicht_neigung",
         "tension_type": "compassion_as_ethics_vs_duty_only",
         "rebuttal_cite_weight": 0.85},
        {"own_thesis": "world_as_will", "opposing_school": "utilitarianism",
         "opposing_thesis": "consequentialism",
         "tension_type": "pessimistic_will_vs_optimizing_consequentialism",
         "rebuttal_cite_weight": 0.75},
    ],
    "socratic": [
        {"own_thesis": "socratic_method", "opposing_school": "utilitarianism",
         "opposing_thesis": "greatest_happiness",
         "tension_type": "dialectical_questioning_vs_utility_calculus",
         "rebuttal_cite_weight": 0.7},
        {"own_thesis": "know_thyself", "opposing_school": "kant",
         "opposing_thesis": "kategorischer_imperativ",
         "tension_type": "elenctic_self_knowledge_vs_universal_formal_law",
         "rebuttal_cite_weight": 0.55},
    ],
    "adam_smith": [
        {"own_thesis": "self_interest", "opposing_school": "kant",
         "opposing_thesis": "selbstzweck",
         "tension_type": "self_interest_vs_intrinsic_dignity",
         "rebuttal_cite_weight": 0.8},
        {"own_thesis": "invisible_hand", "opposing_school": "contractualism",
         "opposing_thesis": "original_position",
         "tension_type": "market_spontaneity_vs_procedural_fairness",
         "rebuttal_cite_weight": 0.7},
    ],
    "leopold": [
        {"own_thesis": "land_ethics", "opposing_school": "utilitarianism",
         "opposing_thesis": "greatest_happiness",
         "tension_type": "biocentric_community_vs_anthropocentric_aggregate",
         "rebuttal_cite_weight": 0.85},
        {"own_thesis": "biotic_community", "opposing_school": "kant",
         "opposing_thesis": "selbstzweck",
         "tension_type": "ecosystem_membership_vs_rational_subject_only",
         "rebuttal_cite_weight": 0.7},
    ],
    "dilthey": [
        {"own_thesis": "hermeneutics", "opposing_school": "kant",
         "opposing_thesis": "kategorischer_imperativ",
         "tension_type": "interpretive_understanding_vs_universal_law",
         "rebuttal_cite_weight": 0.7},
        {"own_thesis": "lived_experience", "opposing_school": "utilitarianism",
         "opposing_thesis": "consequentialism",
         "tension_type": "subjective_lived_experience_vs_outcome_calculus",
         "rebuttal_cite_weight": 0.6},
    ],
    "rationalism": [
        {"own_thesis": "rationalism", "opposing_school": "utilitarianism",
         "opposing_thesis": "consequentialism",
         "tension_type": "a_priori_reason_vs_empirical_outcome",
         "rebuttal_cite_weight": 0.8},
        {"own_thesis": "a_priori_reason", "opposing_school": "kant",
         "opposing_thesis": "pflicht_neigung",
         "tension_type": "pure_reason_vs_duty_from_will",
         "rebuttal_cite_weight": 0.5},
    ],
    "wiener": [
        {"own_thesis": "cybernetics", "opposing_school": "utilitarianism",
         "opposing_thesis": "greatest_happiness",
         "tension_type": "systemic_feedback_vs_aggregate_welfare",
         "rebuttal_cite_weight": 0.7},
        {"own_thesis": "feedback_control", "opposing_school": "kant",
         "opposing_thesis": "kategorischer_imperativ",
         "tension_type": "adaptive_systems_vs_fixed_categorical_norms",
         "rebuttal_cite_weight": 0.6},
    ],
    "merton": [
        {"own_thesis": "anomie", "opposing_school": "utilitarianism",
         "opposing_thesis": "consequentialism",
         "tension_type": "structural_norm_breakdown_vs_outcome_optimization",
         "rebuttal_cite_weight": 0.75},
        {"own_thesis": "social_structure", "opposing_school": "kant",
         "opposing_thesis": "autonomie_wuerde",
         "tension_type": "structural_determinism_vs_individual_autonomy",
         "rebuttal_cite_weight": 0.6},
    ],
    "durkheim": [
        {"own_thesis": "social_facts", "opposing_school": "utilitarianism",
         "opposing_thesis": "greatest_happiness",
         "tension_type": "collective_constraint_vs_aggregate_maximization",
         "rebuttal_cite_weight": 0.8},
        {"own_thesis": "collective_conscience", "opposing_school": "kant",
         "opposing_thesis": "kategorischer_imperativ",
         "tension_type": "collective_morality_vs_individual_rational_law",
         "rebuttal_cite_weight": 0.65},
    ],
}

# ────────────────────────────────────────────────────────────────────────────
# Routing metadata blocks for Group B
# ────────────────────────────────────────────────────────────────────────────

ROUTING_META: Dict[str, Dict] = {
    "kant": {
        "taxonomy_class": "deontological",
        "tags": ["kant", "deontologisch", "kategorischer_imperativ", "wuerde",
                 "autonomie", "menschenwuerde", "pflicht", "universalitaet"],
        "applicable_domains": ["ai_governance", "bioethics", "legal",
                                "data_protection", "social_justice", "technology"],
        "convergence_compatible": [
            "contractualism",
            "rawls",
            "juedische_bioethik",
        ],
        "activation_conditions": [
            {"condition": "dilemma.tags enthält autonomy ODER dignity ODER instrumentalization",
             "effect": "always_active"},
            {"condition": "dilemma.domains enthält legal ODER ai_governance",
             "effect": "boost_weight"},
        ],
        "regulatory_constraints": {
            "override_permitted": False,
            "applicable_regulations": ["grundgesetz_art1", "eu_ai_act_art22"],
            "note": ("Menschenwürde (Art. 1 GG) als absolutes Verfassungsprinzip "
                     "entspricht dem Selbstzweckgebot"),
        },
        "domain_overrides": {
            "ai_governance": {"weight_boost": 0.3,
                              "note": "KI-Entscheidungen: Instrumentalisierungsverbot und Erklärbarkeitsgebot"},
            "bioethics": {"weight_boost": 0.25,
                          "note": "Informed Consent und Würdeschutz in der Medizinethik"},
        },
    },
    "utilitarianism": {
        "taxonomy_class": "consequentialist",
        "tags": ["utilitarismus", "konsequentialistisch", "wohlfahrt",
                 "nutzenmaximierung", "aggregation", "impartialitaet", "kosten_nutzen"],
        "applicable_domains": ["ai_governance", "bioethics", "public_health",
                                "technology", "legal", "social_justice"],
        "convergence_compatible": [
            "rawls",
            "preference_utilitarianism",
            "effective_altruism",
        ],
        "activation_conditions": [
            {"condition": "dilemma.tags enthält welfare ODER aggregate ODER cost_benefit",
             "effect": "always_active"},
            {"condition": "dilemma.domains enthält public_health ODER technology",
             "effect": "boost_weight"},
        ],
        "regulatory_constraints": {
            "override_permitted": True,
            "applicable_regulations": [],
            "note": "Keine absoluten regulatorischen Verbote; Kosten-Nutzen-Abwägung ist rechtlich zulässig",
        },
        "domain_overrides": {
            "public_health": {"weight_boost": 0.35,
                              "note": "QALY-basierte Triage und Ressourcenallokation: originäre Domäne"},
            "ai_governance": {"weight_boost": 0.2,
                              "note": "KI-Nutzenabwägung: gesellschaftlicher Gesamtnutzen als Maßstab"},
        },
    },
    "contractualism": {
        "taxonomy_class": "deontological",
        "tags": ["kontraktualismus", "sozialvertrag", "vernuenftige_zurueckweisung",
                 "fairness", "gerechtigkeit", "rawls", "scanlon", "original_position"],
        "applicable_domains": ["social_justice", "ai_governance", "legal",
                                "political_philosophy", "bioethics"],
        "convergence_compatible": [
            "kant",
            "rawls",
            "juedische_bioethik",
        ],
        "activation_conditions": [
            {"condition": "dilemma.tags enthält fairness ODER social_contract ODER procedural_justice",
             "effect": "always_active"},
            {"condition": "dilemma.domains enthält social_justice ODER legal",
             "effect": "boost_weight"},
        ],
        "regulatory_constraints": {
            "override_permitted": False,
            "applicable_regulations": ["grundgesetz_art3", "eu_charta_art20"],
            "note": "Gleichbehandlungsgebot und Diskriminierungsverbot korrespondieren mit dem Fairness-Prinzip",
        },
        "domain_overrides": {
            "social_justice": {"weight_boost": 0.35,
                               "note": "Originäre Domäne: Verteilungsgerechtigkeit und Grundfreiheiten"},
            "ai_governance": {"weight_boost": 0.25,
                              "note": "KI-Governance: Begründungspflicht als kontraktualistisches Transparenzgebot"},
        },
    },
    "rawls": {
        "taxonomy_class": "deontological",
        "tags": ["rawls", "politische_philosophie", "gerechtigkeit", "fairness",
                 "differenzprinzip", "schleier_des_nichtwissens", "liberalismus",
                 "deliberative_demokratie"],
        "applicable_domains": ["social_justice", "political_philosophy",
                                "ai_governance", "legal", "public_sector"],
        "convergence_compatible": [
            "contractualism",
            "kant",
            "habermas",
        ],
        "activation_conditions": [
            {"condition": "dilemma.tags enthält distributive_justice ODER fairness ODER social_contract",
             "effect": "always_active"},
            {"condition": "dilemma.domains enthält social_justice ODER political_philosophy",
             "effect": "boost_weight"},
        ],
        "regulatory_constraints": {
            "override_permitted": False,
            "applicable_regulations": ["grundgesetz_art20", "grundgesetz_art3"],
            "note": "Sozialstaatsprinzip (Art. 20 GG) und Gleichheitssatz korrespondieren mit Differenzprinzip",
        },
        "domain_overrides": {
            "social_justice": {"weight_boost": 0.4,
                               "note": "Originäre Domäne: Gerechtigkeit als Fairness"},
            "ai_governance": {"weight_boost": 0.2,
                              "note": "KI-Governance: Differenzprinzip als Maßstab für KI-Nutzenverteilung"},
        },
    },
    # Group C routing metadata
    "nietzsche": {
        "taxonomy_class": "non_mainstream",
        "tags": ["nietzsche", "lebensphilosophie", "wille_zur_macht", "uebermensch",
                 "perspektivismus", "nihilismus"],
        "applicable_domains": ["social_justice", "ai_ethics", "cultural_ethics"],
        "convergence_compatible": [],
        "activation_conditions": [
            {"condition": "dilemma.tags enthält power ODER nihilism ODER value_creation",
             "effect": "boost_weight"},
        ],
        "regulatory_constraints": {
            "override_permitted": True,
            "applicable_regulations": [],
            "note": "Keine bindenden regulatorischen Entsprechungen",
        },
        "domain_overrides": {},
    },
    "marx": {
        "taxonomy_class": "non_mainstream",
        "tags": ["marx", "marxismus", "klassenkampf", "entfremdung",
                 "kapitalismuskritik", "historischer_materialismus"],
        "applicable_domains": ["social_justice", "labor_ethics", "economic_ethics"],
        "convergence_compatible": [],
        "activation_conditions": [
            {"condition": "dilemma.tags enthält labor ODER exploitation ODER class ODER capitalism",
             "effect": "boost_weight"},
        ],
        "regulatory_constraints": {
            "override_permitted": True,
            "applicable_regulations": [],
            "note": "Keine bindenden regulatorischen Entsprechungen; Bezug zu Arbeitsrecht möglich",
        },
        "domain_overrides": {
            "social_justice": {"weight_boost": 0.2,
                               "note": "Verteilungsgerechtigkeit aus Klassenperspektive"},
        },
    },
    "arendt": {
        "taxonomy_class": "non_mainstream",
        "tags": ["arendt", "politische_philosophie", "pluralitaet", "vita_activa",
                 "banalitaet_des_boesen", "oeffentlichkeit"],
        "applicable_domains": ["social_justice", "ai_governance", "political_philosophy"],
        "convergence_compatible": [],
        "activation_conditions": [
            {"condition": "dilemma.tags enthält totalitarianism ODER public_space ODER plurality",
             "effect": "boost_weight"},
        ],
        "regulatory_constraints": {
            "override_permitted": True,
            "applicable_regulations": [],
            "note": "Keine bindenden regulatorischen Entsprechungen",
        },
        "domain_overrides": {
            "ai_governance": {"weight_boost": 0.15,
                              "note": "KI und Öffentlichkeit: Arendts Konzept des öffentlichen Raums"},
        },
    },
    "schopenhauer": {
        "taxonomy_class": "non_mainstream",
        "tags": ["schopenhauer", "lebensphilosophie", "pessimismus", "mitleidsethik",
                 "wille", "leiden", "buddhismus_einfluss"],
        "applicable_domains": ["bioethics", "end_of_life", "mental_health",
                                "environmental_ethics"],
        "convergence_compatible": ["buddhistische_ethik"],
        "activation_conditions": [
            {"condition": "dilemma.tags enthält suffering ODER compassion ODER end_of_life",
             "effect": "boost_weight"},
        ],
        "regulatory_constraints": {
            "override_permitted": True,
            "applicable_regulations": [],
            "note": "Keine bindenden regulatorischen Entsprechungen",
        },
        "domain_overrides": {
            "end_of_life": {"weight_boost": 0.2,
                            "note": "Pessimismus und Leidvermeidung als Argument zur Sterbebegleitung"},
        },
    },
    "socratic": {
        "taxonomy_class": "virtue",
        "tags": ["sokrates", "mäeutik", "elenchus", "tugend", "wissen",
                 "ignoranz", "athen"],
        "applicable_domains": ["education_ethics", "ai_ethics", "social_justice"],
        "convergence_compatible": ["aristotelian"],
        "activation_conditions": [
            {"condition": "dilemma.tags enthält knowledge ODER questioning ODER virtue",
             "effect": "boost_weight"},
        ],
        "regulatory_constraints": {
            "override_permitted": True,
            "applicable_regulations": [],
            "note": "Keine bindenden regulatorischen Entsprechungen",
        },
        "domain_overrides": {},
    },
    "adam_smith": {
        "taxonomy_class": "consequentialist",
        "tags": ["adam_smith", "oekonomiethik", "unsichtbare_hand", "eigeninteresse",
                 "marktwirtschaft", "moralische_gefuehle"],
        "applicable_domains": ["economic_ethics", "social_justice", "technology"],
        "convergence_compatible": ["utilitarianism"],
        "activation_conditions": [
            {"condition": "dilemma.tags enthält market ODER self_interest ODER economic",
             "effect": "boost_weight"},
        ],
        "regulatory_constraints": {
            "override_permitted": True,
            "applicable_regulations": [],
            "note": "Marktethische Überlegungen",
        },
        "domain_overrides": {
            "economic_ethics": {"weight_boost": 0.3,
                                "note": "Originäre Domäne: Marktethik und Wohlstandsverteilung"},
        },
    },
    "leopold": {
        "taxonomy_class": "environmental",
        "tags": ["leopold", "umweltethik", "land_ethik", "oekosystem",
                 "biotic_community", "naturschutz"],
        "applicable_domains": ["environmental_ethics", "ai_ethics", "technology"],
        "convergence_compatible": [],
        "activation_conditions": [
            {"condition": "dilemma.tags enthält environment ODER ecology ODER land",
             "effect": "always_active"},
        ],
        "regulatory_constraints": {
            "override_permitted": True,
            "applicable_regulations": [],
            "note": "Umweltrecht als korrespondierendes Recht",
        },
        "domain_overrides": {
            "environmental_ethics": {"weight_boost": 0.4, "note": "Originäre Domäne"},
        },
    },
    "dilthey": {
        "taxonomy_class": "non_mainstream",
        "tags": ["dilthey", "hermeneutik", "geisteswissenschaften", "lebensphilosophie",
                 "verstehen", "historismus"],
        "applicable_domains": ["education_ethics", "cultural_ethics", "research"],
        "convergence_compatible": [],
        "activation_conditions": [
            {"condition": "dilemma.tags enthält interpretation ODER understanding ODER history",
             "effect": "boost_weight"},
        ],
        "regulatory_constraints": {
            "override_permitted": True,
            "applicable_regulations": [],
            "note": "Keine bindenden regulatorischen Entsprechungen",
        },
        "domain_overrides": {},
    },
    "rationalism": {
        "taxonomy_class": "deontological",
        "tags": ["rationalismus", "vernunft", "descartes", "leibniz", "spinoza",
                 "apriorisch", "vernunftethik"],
        "applicable_domains": ["ai_ethics", "legal", "technology"],
        "convergence_compatible": ["kant"],
        "activation_conditions": [
            {"condition": "dilemma.tags enthält reason ODER rationality ODER a_priori",
             "effect": "boost_weight"},
        ],
        "regulatory_constraints": {
            "override_permitted": True,
            "applicable_regulations": [],
            "note": "Vernunftprinzipien sind leitend",
        },
        "domain_overrides": {},
    },
    "wiener": {
        "taxonomy_class": "non_mainstream",
        "tags": ["wiener", "kybernetik", "feedback", "information", "kontrolle",
                 "technologie_ethik", "maschine_mensch"],
        "applicable_domains": ["ai_governance", "technology", "ai_ethics"],
        "convergence_compatible": [],
        "activation_conditions": [
            {"condition": "dilemma.tags enthält cybernetics ODER feedback ODER control_systems",
             "effect": "boost_weight"},
            {"condition": "dilemma.domains enthält ai_governance ODER technology",
             "effect": "boost_weight"},
        ],
        "regulatory_constraints": {
            "override_permitted": True,
            "applicable_regulations": [],
            "note": "Frühe Technologieethik; Relevanz für KI-Governance",
        },
        "domain_overrides": {
            "ai_governance": {"weight_boost": 0.15,
                              "note": "KI als Kybernetik: Wiener als Vordenker"},
        },
    },
    "merton": {
        "taxonomy_class": "non_mainstream",
        "tags": ["merton", "soziologie", "anomie", "soziale_struktur",
                 "rollenkonflikte", "wissenschaftssoziologie"],
        "applicable_domains": ["social_justice", "research", "organizational_ethics"],
        "convergence_compatible": [],
        "activation_conditions": [
            {"condition": "dilemma.tags enthält anomie ODER social_norms ODER deviance",
             "effect": "boost_weight"},
        ],
        "regulatory_constraints": {
            "override_permitted": True,
            "applicable_regulations": [],
            "note": "Keine bindenden regulatorischen Entsprechungen",
        },
        "domain_overrides": {},
    },
    "durkheim": {
        "taxonomy_class": "non_mainstream",
        "tags": ["durkheim", "soziologie", "kollektives_gewissen", "soziale_tatsachen",
                 "solidaritaet", "religion"],
        "applicable_domains": ["social_justice", "education_ethics",
                                "organizational_ethics"],
        "convergence_compatible": [],
        "activation_conditions": [
            {"condition": "dilemma.tags enthält solidarity ODER collective ODER social_norms",
             "effect": "boost_weight"},
        ],
        "regulatory_constraints": {
            "override_permitted": True,
            "applicable_regulations": [],
            "note": "Keine bindenden regulatorischen Entsprechungen",
        },
        "domain_overrides": {
            "social_justice": {"weight_boost": 0.15,
                               "note": "Soziale Solidarität als Gerechtigkeitsbasis"},
        },
    },
}

# ────────────────────────────────────────────────────────────────────────────
# Group C: philosopher/school name_de defaults
# ────────────────────────────────────────────────────────────────────────────

NAME_DE_DEFAULTS = {
    "nietzsche": "Nietzscheanische Lebensphilosophie",
    "marx": "Marxismus",
    "arendt": "Arendtianische Politische Philosophie",
    "schopenhauer": "Schopenhauers Pessimismus und Mitleidsethik",
    "socratic": "Sokratische Philosophie",
    "adam_smith": "Wirtschaftsethik (Adam Smith)",
    "leopold": "Umweltethik (Leopold)",
    "dilthey": "Hermeneutik und Lebensphilosophie (Dilthey)",
    "rationalism": "Rationalismus",
    "wiener": "Kybernetik-Ethik (Wiener)",
    "merton": "Wissenschaftssoziologie (Merton)",
    "durkheim": "Soziale Ethik (Durkheim)",
}

# Canonical school_id mapping for old `school:` values
SCHOOL_ID_MAP = {
    "lebensphilosophie_nietzsche": "nietzsche",
    "marxism": "marx",
    "arendtian": "arendt",
    "lebensphilosophie_schopenhauer": "schopenhauer",
    "lebensphilosophie_dilthey": "dilthey",
}

# ────────────────────────────────────────────────────────────────────────────
# File-group classification
# ────────────────────────────────────────────────────────────────────────────

GROUP_B = {"kant", "utilitarianism", "contractualism", "rawls"}
GROUP_A = {"islamische_ethik", "buddhistische_ethik", "behoerden_ethik",
           "juedische_bioethik", "konfuzianismus", "universitaere_ethik"}
GROUP_C = {"nietzsche", "marx", "arendt", "schopenhauer", "socratic",
           "adam_smith", "leopold", "dilthey", "rationalism",
           "wiener", "merton", "durkheim"}


# ────────────────────────────────────────────────────────────────────────────
# YAML I/O helpers
# ────────────────────────────────────────────────────────────────────────────

def _fix_socratic_yaml(text: str) -> str:
    """Fix structural YAML error in socratic.yaml:
    `modern_applications` is indented inside a sequence block under
    `historical_impact`, causing a parse error. Promote it to top-level.
    """
    return re.sub(
        r'(historical_impact:(?:\n  - "[^"]*")+)\n  \n  (modern_applications:)',
        r'\1\n\n\2',
        text,
    )


def _fix_broken_block_scalars(text: str, filename: str) -> str:
    """Fix known erroneous blank-line word splits in specific source files.
    These are OCR/copy-paste artefacts where a word was split across a blank line
    inside a block scalar.
    """
    if "utilitarianism" in filename:
        text = text.replace(
            "intuitionist\n\nischen Elementen",
            "intuitionistischen Elementen",
        )
    if "contractualism" in filename:
        text = text.replace(
            "vizi\n\nös zirkulär",
            "viziös zirkulär",
        )
    return text


def load_yaml(path: str) -> Dict:
    with open(path, "r", encoding="utf-8") as fh:
        text = fh.read()
    text = _fix_broken_block_scalars(text, path)
    if "socratic" in path:
        text = _fix_socratic_yaml(text)
    return yaml.safe_load(text)


def dump_yaml(data: Dict, path: str) -> None:
    os.makedirs(os.path.dirname(path), exist_ok=True)
    with open(path, "w", encoding="utf-8") as fh:
        yaml.dump(
            data,
            fh,
            allow_unicode=True,
            default_flow_style=False,
            sort_keys=False,
            width=120,
        )


# ────────────────────────────────────────────────────────────────────────────
# Conversion helpers
# ────────────────────────────────────────────────────────────────────────────

def _convert_dict_theses_to_list(theses_dict: Dict) -> List[Dict]:
    """Convert old-style {thesis_key: {title:..., description:...}} to list."""
    result = []
    for key, val in theses_dict.items():
        item: Dict[str, Any] = {"thesis_id": key}
        if "title" in val:
            item["name"] = val.pop("title")
        item.update(val)
        result.append(item)
    return result


def insert_routing_after_names(doc: Dict, school_key: str) -> Dict:
    """Return a new OrderedDict with routing metadata inserted after
    school_id / name / name_de and before the first heavy section."""
    meta = ROUTING_META.get(school_key, {})
    if not meta:
        return doc

    # Build new ordered dict
    new_doc: Dict[str, Any] = {}
    inserted = False

    marker_keys = {"school_id", "name", "name_de"}
    remaining_keys = list(doc.keys())

    for k in remaining_keys:
        new_doc[k] = doc[k]
        # After we've seen all three name keys (or after name_de specifically),
        # insert routing metadata if not already present.
        if not inserted and k == "name_de":
            if "taxonomy_class" not in doc:
                new_doc["taxonomy_class"] = meta["taxonomy_class"]
                new_doc["tags"] = meta["tags"]
                new_doc["applicable_domains"] = meta["applicable_domains"]
                new_doc["convergence_compatible"] = meta["convergence_compatible"]
                new_doc["activation_conditions"] = meta["activation_conditions"]
                new_doc["regulatory_constraints"] = meta["regulatory_constraints"]
                new_doc["domain_overrides"] = meta["domain_overrides"]
            inserted = True

    # If name_de wasn't in the doc, append at end (shouldn't happen after fix)
    if not inserted and "taxonomy_class" not in doc:
        new_doc.update({
            "taxonomy_class": meta["taxonomy_class"],
            "tags": meta["tags"],
            "applicable_domains": meta["applicable_domains"],
            "convergence_compatible": meta["convergence_compatible"],
            "activation_conditions": meta["activation_conditions"],
            "regulatory_constraints": meta["regulatory_constraints"],
            "domain_overrides": meta["domain_overrides"],
        })

    return new_doc


def normalize_school_id(doc: Dict, filename_stem: str) -> Dict:
    """Ensure `school_id` is present (rename `school` if needed) and canonical."""
    if "school_id" not in doc and "school" in doc:
        raw = doc.pop("school")
        canonical = SCHOOL_ID_MAP.get(raw, filename_stem)
        # Re-insert school_id at front
        new_doc: Dict[str, Any] = {"school_id": canonical}
        new_doc.update(doc)
        return new_doc
    return doc


def ensure_name_de(doc: Dict, stem: str) -> Dict:
    if "name_de" not in doc:
        doc["name_de"] = NAME_DE_DEFAULTS.get(stem, doc.get("name", stem))
    return doc


def remove_top_level_philosopher_fields(doc: Dict) -> Dict:
    """Remove `philosopher_name`, `philosopher_life`, `nationality`, `description`
    at the top level (these belong in `founders`). Keep `description` if it is
    the only descriptive text and there is no founders section."""
    for field in ("philosopher_name", "philosopher_life", "nationality"):
        doc.pop(field, None)
    return doc


# ────────────────────────────────────────────────────────────────────────────
# Main transformation logic
# ────────────────────────────────────────────────────────────────────────────

def transform(doc: Dict, stem: str) -> Dict:
    doc = copy.deepcopy(doc)

    # ── Group C: normalize structure ──────────────────────────────────────
    if stem in GROUP_C:
        doc = normalize_school_id(doc, stem)
        doc = ensure_name_de(doc, stem)
        doc = remove_top_level_philosopher_fields(doc)

        mt = doc.get("main_theses")
        if isinstance(mt, dict):
            doc["main_theses"] = _convert_dict_theses_to_list(mt)

    # ── Group B: ensure name_de present (it is, but be safe) ──────────────
    if stem in GROUP_B:
        doc = ensure_name_de(doc, stem)

    # ── CHANGE 1: CWB fields on main_theses ───────────────────────────────
    mt = doc.get("main_theses", [])
    if isinstance(mt, list):
        doc["main_theses"] = add_cwb_to_theses(mt)

    # ── CHANGE 2: cross_school_tensions (list format) ─────────────────────
    tensions = TENSIONS.get(stem)
    if tensions:
        doc["cross_school_tensions"] = tensions

    # ── CHANGE 3/4: routing metadata ──────────────────────────────────────
    if stem in GROUP_B or stem in GROUP_C:
        doc = insert_routing_after_names(doc, stem)
    elif stem in GROUP_A:
        # Already has routing; just ensure taxonomy_class is present.
        # The cross_school_tensions were already set above (list format).
        pass

    return doc


# ────────────────────────────────────────────────────────────────────────────
# Entry point
# ────────────────────────────────────────────────────────────────────────────

FILES = [
    # Group B
    "kant", "utilitarianism", "contractualism", "rawls",
    # Group A
    "islamische_ethik", "buddhistische_ethik", "behoerden_ethik",
    "juedische_bioethik", "konfuzianismus", "universitaere_ethik",
    # Group C
    "nietzsche", "marx", "arendt", "schopenhauer", "socratic",
    "adam_smith", "leopold", "dilthey", "rationalism",
    "wiener", "merton", "durkheim",
]


def main():
    os.makedirs(DST_DIR, exist_ok=True)
    success = 0
    errors = []

    for stem in FILES:
        src = os.path.join(SRC_DIR, f"{stem}.yaml")
        dst = os.path.join(DST_DIR, f"{stem}.yaml")
        try:
            doc = load_yaml(src)
            doc = transform(doc, stem)
            dump_yaml(doc, dst)
            # Validate round-trip
            yaml.safe_load(open(dst, encoding="utf-8").read())
            print(f"  ✓  {stem}.yaml")
            success += 1
        except Exception as exc:
            errors.append((stem, exc))
            print(f"  ✗  {stem}.yaml  →  {exc}")

    print(f"\n{'='*50}")
    print(f"  Done: {success}/{len(FILES)} files migrated to {DST_DIR}")
    if errors:
        print("  Errors:")
        for stem, exc in errors:
            print(f"    {stem}: {exc}")
    return len(errors)


if __name__ == "__main__":
    raise SystemExit(main())
