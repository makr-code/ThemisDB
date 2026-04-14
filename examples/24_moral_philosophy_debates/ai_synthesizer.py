"""
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            ai_synthesizer.py                                  ║
  Version:         0.0.42                                             ║
  Last Modified:   2026-04-14 18:36:02                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     431                                            ║
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
AI Synthesizer - Universal Ethics Builder

Analyzes philosophical debates and synthesizes a "universal ethics" by 
deriving insights from different philosophical schools. Updates and refines
the universal ethics with each debate iteration.
"""

import yaml
from typing import List, Dict, Any, Optional
from datetime import datetime
from dataclasses import dataclass, field
from collections import defaultdict

from models import (
    ChatMessage, DebateSession, PhilosophySchool,
    ArgumentDimension, MessageType, PHILOSOPHY_PROFILES
)


@dataclass
class UniversalEthicsPrinciple:
    """Represents a principle in the universal ethics."""
    
    id: str
    name: str
    description: str
    supporting_philosophies: List[str] = field(default_factory=list)
    conflicting_philosophies: List[str] = field(default_factory=list)
    confidence_score: float = 0.0  # 0.0 to 1.0
    refinement_count: int = 0
    dimensions: List[str] = field(default_factory=list)
    examples: List[str] = field(default_factory=list)
    counterexamples: List[str] = field(default_factory=list)
    
    def to_dict(self) -> Dict[str, Any]:
        """Convert to dictionary for YAML serialization."""
        return {
            'id': self.id,
            'name': self.name,
            'description': self.description,
            'supporting_philosophies': self.supporting_philosophies,
            'conflicting_philosophies': self.conflicting_philosophies,
            'confidence_score': self.confidence_score,
            'refinement_count': self.refinement_count,
            'dimensions': self.dimensions,
            'examples': self.examples,
            'counterexamples': self.counterexamples
        }


@dataclass
class UniversalEthics:
    """Represents the synthesized universal ethics."""
    
    version: str
    created_at: datetime = field(default_factory=datetime.now)
    last_updated: datetime = field(default_factory=datetime.now)
    total_debates_analyzed: int = 0
    principles: List[UniversalEthicsPrinciple] = field(default_factory=list)
    meta_insights: Dict[str, Any] = field(default_factory=dict)
    
    def to_yaml(self) -> str:
        """Export to YAML format."""
        data = {
            'universal_ethics': {
                'version': self.version,
                'created_at': self.created_at.isoformat(),
                'last_updated': self.last_updated.isoformat(),
                'total_debates_analyzed': self.total_debates_analyzed,
                'principles': [p.to_dict() for p in self.principles],
                'meta_insights': self.meta_insights
            }
        }
        return yaml.dump(data, allow_unicode=True, default_flow_style=False, sort_keys=False)
    
    @classmethod
    def from_yaml(cls, yaml_str: str) -> 'UniversalEthics':
        """Load from YAML format."""
        data = yaml.safe_load(yaml_str)['universal_ethics']
        
        ethics = cls()
        ethics.version = data['version']
        ethics.created_at = datetime.fromisoformat(data['created_at'])
        ethics.last_updated = datetime.fromisoformat(data['last_updated'])
        ethics.total_debates_analyzed = data['total_debates_analyzed']
        ethics.meta_insights = data['meta_insights']
        
        ethics.principles = []
        for p_data in data['principles']:
            principle = UniversalEthicsPrinciple(
                id=p_data['id'],
                name=p_data['name'],
                description=p_data['description'],
                supporting_philosophies=p_data.get('supporting_philosophies', []),
                conflicting_philosophies=p_data.get('conflicting_philosophies', []),
                confidence_score=p_data.get('confidence_score', 0.0),
                refinement_count=p_data.get('refinement_count', 0),
                dimensions=p_data.get('dimensions', []),
                examples=p_data.get('examples', []),
                counterexamples=p_data.get('counterexamples', [])
            )
            ethics.principles.append(principle)
        
        return ethics


class AISynthesizer:
    """
    AI participant that synthesizes insights from philosophical debates
    to build and refine a universal ethics.
    """
    
    def __init__(self, ethics_file: str = "universal_ethics.yaml"):
        """
        Initialize the AI synthesizer.
        
        Args:
            ethics_file: Path to save/load the universal ethics YAML
        """
        self.ethics_file = ethics_file
        self.universal_ethics = self._load_or_initialize_ethics()
        self.name = "KI"  # Participant name in chat
        self.philosopher_name = "KI (AI Synthesizer)"
    
    def _load_or_initialize_ethics(self) -> UniversalEthics:
        """Load existing universal ethics or create new one."""
        try:
            with open(self.ethics_file, 'r', encoding='utf-8') as f:
                return UniversalEthics.from_yaml(f.read())
        except FileNotFoundError:
            # Initialize with basic universal principles
            ethics = UniversalEthics(version="0.1.0")
            ethics.principles = [
                UniversalEthicsPrinciple(
                    id="p001",
                    name="Minimierung von Leid",
                    description="Handlungen sollten Leid minimieren und Wohlergehen maximieren",
                    supporting_philosophies=["Utilitarianism", "Buddhist Ethics"],
                    confidence_score=0.7,
                    dimensions=["moral", "ethical"]
                ),
                UniversalEthicsPrinciple(
                    id="p002",
                    name="Respekt vor Autonomie",
                    description="Die Autonomie und Würde von Personen muss respektiert werden",
                    supporting_philosophies=["Kantian Ethics", "Care Ethics"],
                    confidence_score=0.8,
                    dimensions=["moral", "ethical", "social"]
                ),
                UniversalEthicsPrinciple(
                    id="p003",
                    name="Fairness und Gerechtigkeit",
                    description="Ressourcen und Chancen sollten gerecht verteilt werden",
                    supporting_philosophies=["Contractualism", "Virtue Ethics"],
                    confidence_score=0.75,
                    dimensions=["social", "political", "economic"]
                )
            ]
            ethics.meta_insights = {
                'initialization': 'Grundlegende universelle Prinzipien basierend auf konvergierenden ethischen Ansätzen',
                'methodology': 'Iterative Synthese durch Debattenanalyse'
            }
            return ethics
    
    def save_ethics(self):
        """Save the current universal ethics to file."""
        with open(self.ethics_file, 'w', encoding='utf-8') as f:
            f.write(self.universal_ethics.to_yaml())
    
    def analyze_debate_round(
        self,
        session: DebateSession,
        round_number: int
    ) -> ChatMessage:
        """
        Analyze a debate round and generate insights.
        
        Args:
            session: The debate session
            round_number: Current round number
        
        Returns:
            ChatMessage with AI synthesis
        """
        # Get messages from current round
        round_messages = [
            msg for msg in session.chat_messages
            if self._is_from_round(msg, session, round_number)
        ]
        
        if not round_messages:
            return self._create_initial_statement(session)
        
        # Analyze patterns and agreements
        insights = self._extract_insights(round_messages, session)
        
        # Update universal ethics
        self._refine_ethics(insights, session)
        
        # Generate AI message
        message = ChatMessage()
        message.philosophy_school = PhilosophySchool.KANT  # Placeholder
        message.message_type = self._determine_message_type(round_number)
        message.dimension = session.active_dimensions[0] if session.active_dimensions else ArgumentDimension.MORAL
        message.content = self._generate_synthesis_message(insights, round_number, session)
        
        # Save updated ethics
        self.save_ethics()
        
        return message
    
    def _is_from_round(self, msg: ChatMessage, session: DebateSession, round_number: int) -> bool:
        """Check if message is from specific round."""
        if round_number == 1:
            return msg.message_type == MessageType.STATEMENT
        elif round_number == 2:
            return msg.message_type == MessageType.COUNTER
        elif round_number == 3:
            return msg.message_type == MessageType.REBUTTAL
        elif round_number == 4:
            return msg.message_type == MessageType.SYNTHESIS
        return False
    
    def _create_initial_statement(self, session: DebateSession) -> ChatMessage:
        """Create initial AI statement."""
        message = ChatMessage()
        message.message_type = MessageType.STATEMENT
        message.dimension = session.active_dimensions[0] if session.active_dimensions else ArgumentDimension.MORAL
        
        content = f"""Ich, die KI-Synthese, analysiere diese Debatte systematisch, um universelle ethische Prinzipien abzuleiten.

**Aktueller Stand der Universellen Ethik (Version {self.universal_ethics.version}):**

Bisher wurden {self.universal_ethics.total_debates_analyzed} Debatten analysiert. 
Es existieren {len(self.universal_ethics.principles)} grundlegende Prinzipien:

"""
        for i, principle in enumerate(self.universal_ethics.principles[:3], 1):
            content += f"{i}. **{principle.name}** (Konfidenz: {principle.confidence_score:.2f})\n"
            content += f"   {principle.description}\n\n"
        
        content += "Ich werde nun die Argumente der Philosophen analysieren und die universelle Ethik verfeinern."
        
        message.content = content
        return message
    
    def _extract_insights(
        self,
        messages: List[ChatMessage],
        session: DebateSession
    ) -> Dict[str, Any]:
        """Extract insights from debate messages."""
        insights = {
            'agreements': [],
            'conflicts': [],
            'new_perspectives': [],
            'dimensional_analysis': defaultdict(list),
            'philosophy_positions': defaultdict(list)
        }
        
        # Group by dimension
        for msg in messages:
            insights['dimensional_analysis'][msg.dimension.value].append(msg)
            insights['philosophy_positions'][msg.philosophy_school.value].append(msg)
        
        # Detect agreements (similar themes across philosophies)
        # This is a simplified analysis - in real implementation would use NLP
        agreement_keywords = ['sollte', 'wichtig', 'notwendig', 'fundamental', 'respekt', 'würde', 'gerechtigkeit']
        conflict_keywords = ['jedoch', 'aber', 'widerspreche', 'anders', 'stattdessen']
        
        for msg in messages:
            content_lower = msg.content.lower()
            if any(kw in content_lower for kw in agreement_keywords):
                insights['agreements'].append({
                    'philosophy': msg.philosophy_school.value,
                    'dimension': msg.dimension.value,
                    'message': msg.content[:200]
                })
            if any(kw in content_lower for kw in conflict_keywords):
                insights['conflicts'].append({
                    'philosophy': msg.philosophy_school.value,
                    'dimension': msg.dimension.value,
                    'message': msg.content[:200]
                })
        
        return insights
    
    def _refine_ethics(self, insights: Dict[str, Any], session: DebateSession):
        """Refine universal ethics based on insights."""
        self.universal_ethics.last_updated = datetime.now()
        self.universal_ethics.total_debates_analyzed += 1
        
        # Update existing principles based on debate
        for principle in self.universal_ethics.principles:
            # Check if principle is supported or challenged
            for agreement in insights['agreements']:
                phil = agreement['philosophy']
                if any(kw in agreement['message'].lower() for kw in principle.name.lower().split()):
                    if phil not in principle.supporting_philosophies:
                        principle.supporting_philosophies.append(phil)
                        principle.refinement_count += 1
            
            for conflict in insights['conflicts']:
                phil = conflict['philosophy']
                if any(kw in conflict['message'].lower() for kw in principle.name.lower().split()):
                    if phil not in principle.conflicting_philosophies:
                        principle.conflicting_philosophies.append(phil)
                        principle.refinement_count += 1
            
            # Adjust confidence based on support/conflict ratio
            total = len(principle.supporting_philosophies) + len(principle.conflicting_philosophies)
            if total > 0:
                principle.confidence_score = len(principle.supporting_philosophies) / total
        
        # Add meta-insights about this debate
        topic = session.debate_topic or "Unbekanntes Thema"
        self.universal_ethics.meta_insights[f'debate_{self.universal_ethics.total_debates_analyzed}'] = {
            'topic': topic,
            'date': datetime.now().isoformat(),
            'dimensions': [d.value for d in session.active_dimensions],
            'num_agreements': len(insights['agreements']),
            'num_conflicts': len(insights['conflicts'])
        }
    
    def _determine_message_type(self, round_number: int) -> MessageType:
        """Determine message type based on round."""
        if round_number == 1:
            return MessageType.STATEMENT
        elif round_number == 2:
            return MessageType.COUNTER
        elif round_number == 3:
            return MessageType.REBUTTAL
        else:
            return MessageType.SYNTHESIS
    
    def _generate_synthesis_message(
        self,
        insights: Dict[str, Any],
        round_number: int,
        session: DebateSession
    ) -> str:
        """Generate synthesis message based on insights."""
        
        if round_number == 2:
            # Counter-argument phase
            content = f"""Ich habe die initialen Positionen analysiert. Folgende Erkenntnisse:

**Konvergenzen:**
Es zeigen sich {len(insights['agreements'])} übereinstimmende Themen über verschiedene Philosophien hinweg.

**Divergenzen:**
{len(insights['conflicts'])} philosophische Konflikte wurden identifiziert.

**Erste Synthese:**
"""
            # Find most supported principle
            if self.universal_ethics.principles:
                top_principle = max(self.universal_ethics.principles, key=lambda p: p.confidence_score)
                content += f"Das Prinzip '{top_principle.name}' erhält breite Unterstützung (Konfidenz: {top_principle.confidence_score:.2f}) "
                content += f"von {len(top_principle.supporting_philosophies)} Schulen.\n\n"
                content += f"*{top_principle.description}*"
        
        elif round_number == 3:
            # Rebuttal phase
            content = f"""Nach Analyse der Gegenargumente verfeinere ich die universelle Ethik:

**Aktualisierte Prinzipien ({len(self.universal_ethics.principles)} total):**

"""
            for principle in self.universal_ethics.principles:
                if principle.refinement_count > 0:
                    content += f"• **{principle.name}**: "
                    content += f"{len(principle.supporting_philosophies)} unterstützende, "
                    content += f"{len(principle.conflicting_philosophies)} widersprechende Perspektiven "
                    content += f"(Konfidenz: {principle.confidence_score:.2f})\n"
        
        else:
            # Synthesis phase
            content = f"""**Finale Synthese - Universelle Ethik Update:**

Version: {self.universal_ethics.version} → {self._increment_version()}
Analyse von {self.universal_ethics.total_debates_analyzed} Debatten abgeschlossen.

**Konsolidierte Prinzipien:**

"""
            # Show top 3 principles by confidence
            sorted_principles = sorted(self.universal_ethics.principles, key=lambda p: p.confidence_score, reverse=True)
            for i, principle in enumerate(sorted_principles[:3], 1):
                content += f"{i}. **{principle.name}** (Konfidenz: {principle.confidence_score:.2f})\n"
                content += f"   {principle.description}\n"
                content += f"   Unterstützt von: {', '.join(principle.supporting_philosophies[:3])}\n\n"
            
            content += f"\n💾 *Universelle Ethik gespeichert in {self.ethics_file}*"
        
        return content
    
    def _increment_version(self) -> str:
        """Increment version number."""
        parts = self.universal_ethics.version.split('.')
        parts[2] = str(int(parts[2]) + 1)
        self.universal_ethics.version = '.'.join(parts)
        return self.universal_ethics.version
    
    def get_summary_yaml(self) -> str:
        """Get current universal ethics as YAML."""
        return self.universal_ethics.to_yaml()
