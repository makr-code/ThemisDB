"""
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            argument_models.py                                 ║
  Version:         0.0.42                                             ║
  Last Modified:   2026-04-14 18:36:02                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     326                                            ║
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
Argument Models for Ethical Discourse Engine

This module provides data structures for representing ethical arguments,
debate chains, and dialectical reasoning within the ThemisDB framework.

Author: ThemisDB Ethics AI Framework
License: MIT
"""

from dataclasses import dataclass, field
from datetime import datetime
from typing import List, Optional, Dict, Any
from enum import Enum
import uuid


class ArgumentType(Enum):
    """Types of arguments in ethical discourse."""
    PRO = "pro"
    CONTRA = "contra"
    REBUTTAL = "rebuttal"
    SYNTHESIS = "synthesis"
    QUESTION = "question"
    CLARIFICATION = "clarification"


class ArgumentStrength(Enum):
    """Strength assessment for arguments."""
    WEAK = "weak"
    MODERATE = "moderate"
    STRONG = "strong"
    DECISIVE = "decisive"


@dataclass
class EthicalArgument:
    """
    Represents a single ethical argument in a philosophical debate.
    
    Attributes:
        id: Unique identifier
        philosophy_school: School of philosophy (e.g., 'kant', 'utilitarianism')
        argument_type: Type of argument (pro, contra, rebuttal, etc.)
        content: The actual argument text
        principle_basis: Core philosophical principle(s) being invoked
        strength: Assessed strength of the argument
        counterarguments: List of IDs of arguments that counter this one
        supports: List of IDs of arguments this one supports
        created_at: Timestamp of creation
        metadata: Additional metadata (confidence, domain, etc.)
    """
    
    id: str = field(default_factory=lambda: str(uuid.uuid4()))
    philosophy_school: str = ""
    argument_type: ArgumentType = ArgumentType.PRO
    content: str = ""
    principle_basis: List[str] = field(default_factory=list)
    strength: ArgumentStrength = ArgumentStrength.MODERATE
    counterarguments: List[str] = field(default_factory=list)
    supports: List[str] = field(default_factory=list)
    created_at: datetime = field(default_factory=datetime.now)
    metadata: Dict[str, Any] = field(default_factory=dict)
    
    def to_dict(self) -> Dict[str, Any]:
        """Convert to dictionary for storage."""
        return {
            'id': self.id,
            'philosophy_school': self.philosophy_school,
            'argument_type': self.argument_type.value,
            'content': self.content,
            'principle_basis': self.principle_basis,
            'strength': self.strength.value,
            'counterarguments': self.counterarguments,
            'supports': self.supports,
            'created_at': self.created_at.isoformat(),
            'metadata': self.metadata
        }
    
    @classmethod
    def from_dict(cls, data: Dict[str, Any]) -> 'EthicalArgument':
        """Create from dictionary."""
        return cls(
            id=data.get('id', str(uuid.uuid4())),
            philosophy_school=data.get('philosophy_school', ''),
            argument_type=ArgumentType(data.get('argument_type', 'pro')),
            content=data.get('content', ''),
            principle_basis=data.get('principle_basis', []),
            strength=ArgumentStrength(data.get('strength', 'moderate')),
            counterarguments=data.get('counterarguments', []),
            supports=data.get('supports', []),
            created_at=datetime.fromisoformat(data.get('created_at', datetime.now().isoformat())),
            metadata=data.get('metadata', {})
        )


@dataclass
class ArgumentChain:
    """
    Represents a chain of arguments in dialectical reasoning.
    
    A chain typically follows: Thesis → Antithesis → Synthesis pattern,
    or Pro → Contra → Rebuttal → Resolution pattern.
    
    Attributes:
        id: Unique identifier
        dilemma_id: ID of the ethical dilemma being debated
        arguments: Ordered list of argument IDs in the chain
        chain_type: Type of dialectical chain
        conclusion: Final synthesis or conclusion (if reached)
        confidence_score: Confidence in the chain's reasoning
        created_at: Timestamp
        metadata: Additional information
    """
    
    id: str = field(default_factory=lambda: str(uuid.uuid4()))
    dilemma_id: str = ""
    arguments: List[str] = field(default_factory=list)
    chain_type: str = "dialectical"  # dialectical, pro_contra, rebuttal_chain
    conclusion: Optional[str] = None
    confidence_score: float = 0.0
    created_at: datetime = field(default_factory=datetime.now)
    metadata: Dict[str, Any] = field(default_factory=dict)
    
    def add_argument(self, argument_id: str) -> None:
        """Add an argument to the chain."""
        self.arguments.append(argument_id)
    
    def to_dict(self) -> Dict[str, Any]:
        """Convert to dictionary."""
        return {
            'id': self.id,
            'dilemma_id': self.dilemma_id,
            'arguments': self.arguments,
            'chain_type': self.chain_type,
            'conclusion': self.conclusion,
            'confidence_score': self.confidence_score,
            'created_at': self.created_at.isoformat(),
            'metadata': self.metadata
        }
    
    @classmethod
    def from_dict(cls, data: Dict[str, Any]) -> 'ArgumentChain':
        """Create from dictionary."""
        return cls(
            id=data.get('id', str(uuid.uuid4())),
            dilemma_id=data.get('dilemma_id', ''),
            arguments=data.get('arguments', []),
            chain_type=data.get('chain_type', 'dialectical'),
            conclusion=data.get('conclusion'),
            confidence_score=data.get('confidence_score', 0.0),
            created_at=datetime.fromisoformat(data.get('created_at', datetime.now().isoformat())),
            metadata=data.get('metadata', {})
        )


@dataclass
class PhilosophyProfile:
    """
    Extended philosophy profile with internal debate structure.
    
    Attributes:
        school_id: Unique identifier (e.g., 'kant', 'utilitarianism')
        name: Display name
        main_theses: Core philosophical theses
        secondary_theses: Supporting theses
        internal_debate: Pro/contra/rebuttal arguments for self-critique
        decision_framework: Framework for applying philosophy
        strengths: List of strengths
        weaknesses: List of weaknesses
        philosophical_positioning: Alliances and oppositions
    """
    
    school_id: str = ""
    name: str = ""
    main_theses: List[Dict[str, Any]] = field(default_factory=list)
    secondary_theses: List[Dict[str, Any]] = field(default_factory=list)
    internal_debate: Dict[str, List[str]] = field(default_factory=dict)  # pro, contra, rebuttal
    decision_framework: Dict[str, Any] = field(default_factory=dict)
    strengths: List[str] = field(default_factory=list)
    weaknesses: List[str] = field(default_factory=list)
    philosophical_positioning: Dict[str, List[str]] = field(default_factory=dict)  # allies, opponents
    
    def to_dict(self) -> Dict[str, Any]:
        """Convert to dictionary."""
        return {
            'school_id': self.school_id,
            'name': self.name,
            'main_theses': self.main_theses,
            'secondary_theses': self.secondary_theses,
            'internal_debate': self.internal_debate,
            'decision_framework': self.decision_framework,
            'strengths': self.strengths,
            'weaknesses': self.weaknesses,
            'philosophical_positioning': self.philosophical_positioning
        }


@dataclass
class DebateInitialization:
    """
    Initialization state for a new ethical debate.
    
    Attributes:
        dilemma_id: ID of the ethical dilemma
        dilemma_description: Description of the dilemma
        participating_schools: List of philosophy schools participating
        initial_arguments: Pre-loaded arguments from philosophy profiles
        context: Additional context (news article, scenario, etc.)
        timestamp: When debate was initialized
    """
    
    dilemma_id: str = field(default_factory=lambda: str(uuid.uuid4()))
    dilemma_description: str = ""
    participating_schools: List[str] = field(default_factory=list)
    initial_arguments: List[EthicalArgument] = field(default_factory=list)
    context: Dict[str, Any] = field(default_factory=dict)
    timestamp: datetime = field(default_factory=datetime.now)
    
    def add_school(self, school_id: str) -> None:
        """Add a philosophy school to the debate."""
        if school_id not in self.participating_schools:
            self.participating_schools.append(school_id)
    
    def add_initial_argument(self, argument: EthicalArgument) -> None:
        """Add an initial argument from a philosophy profile."""
        self.initial_arguments.append(argument)
    
    def to_dict(self) -> Dict[str, Any]:
        """Convert to dictionary."""
        return {
            'dilemma_id': self.dilemma_id,
            'dilemma_description': self.dilemma_description,
            'participating_schools': self.participating_schools,
            'initial_arguments': [arg.to_dict() for arg in self.initial_arguments],
            'context': self.context,
            'timestamp': self.timestamp.isoformat()
        }


@dataclass
class EthicalDecision:
    """
    Represents a final ethical decision after deliberation.
    
    Attributes:
        id: Unique identifier
        dilemma_id: ID of the dilemma
        decision: The decision text
        primary_philosophy: Primary philosophy school informing the decision
        supporting_philosophies: Other schools that support this decision
        argument_chain_ids: IDs of argument chains leading to this decision
        confidence: Confidence score (0-1)
        consensus_level: Level of philosophical consensus
        dissenting_views: Summary of dissenting philosophical positions
        created_at: Timestamp
        outcome_tracking: Data for tracking real-world outcomes
    """
    
    id: str = field(default_factory=lambda: str(uuid.uuid4()))
    dilemma_id: str = ""
    decision: str = ""
    primary_philosophy: str = ""
    supporting_philosophies: List[str] = field(default_factory=list)
    argument_chain_ids: List[str] = field(default_factory=list)
    confidence: float = 0.0
    consensus_level: float = 0.0
    dissenting_views: Dict[str, str] = field(default_factory=dict)
    created_at: datetime = field(default_factory=datetime.now)
    outcome_tracking: Dict[str, Any] = field(default_factory=dict)
    
    def to_dict(self) -> Dict[str, Any]:
        """Convert to dictionary."""
        return {
            'id': self.id,
            'dilemma_id': self.dilemma_id,
            'decision': self.decision,
            'primary_philosophy': self.primary_philosophy,
            'supporting_philosophies': self.supporting_philosophies,
            'argument_chain_ids': self.argument_chain_ids,
            'confidence': self.confidence,
            'consensus_level': self.consensus_level,
            'dissenting_views': self.dissenting_views,
            'created_at': self.created_at.isoformat(),
            'outcome_tracking': self.outcome_tracking
        }
    
    @classmethod
    def from_dict(cls, data: Dict[str, Any]) -> 'EthicalDecision':
        """Create from dictionary."""
        return cls(
            id=data.get('id', str(uuid.uuid4())),
            dilemma_id=data.get('dilemma_id', ''),
            decision=data.get('decision', ''),
            primary_philosophy=data.get('primary_philosophy', ''),
            supporting_philosophies=data.get('supporting_philosophies', []),
            argument_chain_ids=data.get('argument_chain_ids', []),
            confidence=data.get('confidence', 0.0),
            consensus_level=data.get('consensus_level', 0.0),
            dissenting_views=data.get('dissenting_views', {}),
            created_at=datetime.fromisoformat(data.get('created_at', datetime.now().isoformat())),
            outcome_tracking=data.get('outcome_tracking', {})
        )
