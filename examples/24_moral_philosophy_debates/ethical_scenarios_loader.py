"""
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            ethical_scenarios_loader.py                        ║
  Version:         0.0.43                                             ║
  Last Modified:   2026-04-15 04:08:33                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     297                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 2a1fb04231  2026-03-03  Merge branch 'develop' into copilot/audit-src-module-docu... ║
    • a629043ab2  2026-02-22  Audit: document gaps found - benchmarks and stale annotat... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
"""

"""
Ethical Scenarios Loader

This module loads ethical scenarios from YAML configuration files
instead of hardcoding them in Python. This allows for easier configuration
and modification of scenarios without code changes.
"""

import yaml
from pathlib import Path
from typing import List, Dict, Optional, Any
from dataclasses import dataclass, field
from enum import Enum


class DifficultyLevel(Enum):
    """Difficulty levels for ethical scenarios"""
    BASIC = "basic"
    INTERMEDIATE = "intermediate"
    ADVANCED = "advanced"
    EXPERT = "expert"


class EthicalDomain(Enum):
    """Domains for ethical scenarios"""
    MEDICAL = "medical"
    LEGAL = "legal"
    AUTONOMOUS_SYSTEMS = "autonomous_systems"
    PRIVACY = "privacy"
    BUSINESS = "business"
    ENVIRONMENTAL = "environmental"
    AI_ETHICS = "ai_ethics"
    SOCIAL = "social"
    POLITICAL = "political"
    RESEARCH = "research"


@dataclass
class Stakeholder:
    """Represents a stakeholder in an ethical scenario"""
    type: str
    count: int
    risk_level: float  # 0.0 to 1.0
    rights: List[str] = field(default_factory=list)
    interests: List[str] = field(default_factory=list)
    
    @classmethod
    def from_dict(cls, data: Dict[str, Any]) -> 'Stakeholder':
        """Create Stakeholder from dictionary"""
        return cls(
            type=data['type'],
            count=data['count'],
            risk_level=data['risk_level'],
            rights=data.get('rights', []),
            interests=data.get('interests', [])
        )


@dataclass
class PossibleAction:
    """Represents a possible action in ethical scenario"""
    id: str
    description: str
    estimated_outcomes: List[str] = field(default_factory=list)
    violates_principles: List[str] = field(default_factory=list)
    supports_principles: List[str] = field(default_factory=list)
    
    @classmethod
    def from_dict(cls, data: Dict[str, Any]) -> 'PossibleAction':
        """Create PossibleAction from dictionary"""
        return cls(
            id=data['id'],
            description=data['description'],
            estimated_outcomes=data.get('estimated_outcomes', []),
            violates_principles=data.get('violates_principles', []),
            supports_principles=data.get('supports_principles', [])
        )


@dataclass
class EthicalScenario:
    """Complete ethical scenario representation"""
    id: str
    title: str
    description: str
    domain: EthicalDomain
    difficulty: DifficultyLevel
    stakeholders: List[Stakeholder]
    possible_actions: List[PossibleAction]
    relevant_principles: List[str]
    context: Dict[str, Any] = field(default_factory=dict)
    best_philosophy: Optional[str] = None
    expected_outcome: Optional[str] = None
    real_world_precedent: Optional[str] = None
    
    @classmethod
    def from_dict(cls, data: Dict[str, Any]) -> 'EthicalScenario':
        """Create EthicalScenario from dictionary"""
        return cls(
            id=data['id'],
            title=data['title'],
            description=data['description'],
            domain=EthicalDomain(data['domain']),
            difficulty=DifficultyLevel(data['difficulty']),
            stakeholders=[Stakeholder.from_dict(s) for s in data['stakeholders']],
            possible_actions=[PossibleAction.from_dict(a) for a in data['possible_actions']],
            relevant_principles=data['relevant_principles'],
            context=data.get('context', {}),
            best_philosophy=data.get('best_philosophy'),
            expected_outcome=data.get('expected_outcome'),
            real_world_precedent=data.get('real_world_precedent')
        )


class EthicalScenariosLoader:
    """Loader for ethical scenarios from YAML configuration"""
    
    def __init__(self, config_path: Optional[str] = None):
        """
        Initialize the loader
        
        Args:
            config_path: Path to YAML config file. If None, uses default location.
        """
        if config_path is None:
            # Default to ethical_scenarios.yaml in same directory
            config_path = Path(__file__).parent / "ethical_scenarios.yaml"
        
        self.config_path = Path(config_path)
        self._scenarios: Dict[str, EthicalScenario] = {}
        self._load_scenarios()
    
    def _load_scenarios(self):
        """Load scenarios from YAML file"""
        if not self.config_path.exists():
            raise FileNotFoundError(f"Config file not found: {self.config_path}")
        
        with open(self.config_path, 'r', encoding='utf-8') as f:
            config = yaml.safe_load(f)
        
        if 'scenarios' not in config:
            raise ValueError("Config file must have 'scenarios' key")
        
        for scenario_id, scenario_data in config['scenarios'].items():
            scenario = EthicalScenario.from_dict(scenario_data)
            self._scenarios[scenario_id] = scenario
    
    def get_scenario(self, scenario_id: str) -> Optional[EthicalScenario]:
        """Get a specific scenario by ID"""
        return self._scenarios.get(scenario_id)
    
    def get_all_scenarios(self) -> List[EthicalScenario]:
        """Get all loaded scenarios"""
        return list(self._scenarios.values())
    
    def get_scenarios_by_domain(self, domain: EthicalDomain) -> List[EthicalScenario]:
        """Get all scenarios for a specific domain"""
        return [s for s in self._scenarios.values() if s.domain == domain]
    
    def get_scenarios_by_difficulty(self, difficulty: DifficultyLevel) -> List[EthicalScenario]:
        """Get all scenarios for a specific difficulty level"""
        return [s for s in self._scenarios.values() if s.difficulty == difficulty]
    
    def get_scenarios_by_philosophy(self, philosophy: str) -> List[EthicalScenario]:
        """Get scenarios best suited for a specific philosophy"""
        return [s for s in self._scenarios.values() if s.best_philosophy == philosophy]


# Global loader instance for convenience
_default_loader: Optional[EthicalScenariosLoader] = None


def get_default_loader() -> EthicalScenariosLoader:
    """Get the default loader instance (singleton pattern)"""
    global _default_loader
    if _default_loader is None:
        _default_loader = EthicalScenariosLoader()
    return _default_loader


def get_scenario_by_id(scenario_id: str) -> Optional[EthicalScenario]:
    """Get a specific scenario by ID (convenience function)"""
    return get_default_loader().get_scenario(scenario_id)


def get_all_scenarios() -> List[EthicalScenario]:
    """Get all scenarios (convenience function)"""
    return get_default_loader().get_all_scenarios()


def get_scenarios_by_domain(domain: EthicalDomain) -> List[EthicalScenario]:
    """Get scenarios by domain (convenience function)"""
    return get_default_loader().get_scenarios_by_domain(domain)


def get_scenarios_by_difficulty(difficulty: DifficultyLevel) -> List[EthicalScenario]:
    """Get scenarios by difficulty (convenience function)"""
    return get_default_loader().get_scenarios_by_difficulty(difficulty)


def get_scenarios_by_philosophy(philosophy: str) -> List[EthicalScenario]:
    """Get scenarios by philosophy (convenience function)"""
    return get_default_loader().get_scenarios_by_philosophy(philosophy)


# For backward compatibility with old hardcoded module
ALL_SCENARIOS = []
SCENARIOS_BY_DOMAIN = {}
SCENARIOS_BY_DIFFICULTY = {}
SCENARIOS_BY_PHILOSOPHY = {}

def _initialize_compat_globals():
    """Initialize backward compatibility globals"""
    global ALL_SCENARIOS, SCENARIOS_BY_DOMAIN, SCENARIOS_BY_DIFFICULTY, SCENARIOS_BY_PHILOSOPHY
    
    loader = get_default_loader()
    ALL_SCENARIOS = loader.get_all_scenarios()
    
    # Build domain dictionary
    for domain in EthicalDomain:
        SCENARIOS_BY_DOMAIN[domain] = loader.get_scenarios_by_domain(domain)
    
    # Build difficulty dictionary
    for difficulty in DifficultyLevel:
        SCENARIOS_BY_DIFFICULTY[difficulty] = loader.get_scenarios_by_difficulty(difficulty)
    
    # Build philosophy dictionary - collect all unique philosophies
    all_philosophies = set()
    for scenario in ALL_SCENARIOS:
        if scenario.best_philosophy:
            all_philosophies.add(scenario.best_philosophy)
    
    for philosophy in all_philosophies:
        SCENARIOS_BY_PHILOSOPHY[philosophy] = loader.get_scenarios_by_philosophy(philosophy)


# Initialize on module load
try:
    _initialize_compat_globals()
except FileNotFoundError:
    # Config file not found, keep empty collections
    pass


if __name__ == "__main__":
    # Test the loader
    print("Ethical Scenarios Loader")
    print("=" * 70)
    
    loader = EthicalScenariosLoader()
    scenarios = loader.get_all_scenarios()
    
    print(f"\nLoaded {len(scenarios)} scenarios")
    print("\nScenarios by domain:")
    for domain in EthicalDomain:
        domain_scenarios = loader.get_scenarios_by_domain(domain)
        if domain_scenarios:
            print(f"  {domain.value}: {len(domain_scenarios)} scenarios")
    
    print("\nScenarios by difficulty:")
    for difficulty in DifficultyLevel:
        diff_scenarios = loader.get_scenarios_by_difficulty(difficulty)
        if diff_scenarios:
            print(f"  {difficulty.value}: {len(diff_scenarios)} scenarios")
    
    print("\nFirst scenario:")
    if scenarios:
        first = scenarios[0]
        print(f"  ID: {first.id}")
        print(f"  Title: {first.title}")
        print(f"  Domain: {first.domain.value}")
        print(f"  Difficulty: {first.difficulty.value}")
        print(f"  Stakeholders: {len(first.stakeholders)}")
        print(f"  Actions: {len(first.possible_actions)}")
