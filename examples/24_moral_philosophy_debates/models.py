"""
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            models.py                                          ║
  Version:         0.0.38                                             ║
  Last Modified:   2026-04-13 04:12:48                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     944                                            ║
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
Data models for the Moral Philosophy Debates system.

This module defines the core data structures for representing news articles,
philosophical perspectives, arguments, and debate sessions.
"""

from dataclasses import dataclass, field
from datetime import datetime
from typing import List, Optional, Dict, Any
from enum import Enum
import uuid


class PhilosophySchool(Enum):
    """Different schools of philosophy (practical and theoretical)."""
    # Practical Philosophy (Ethics, Political, Social)
    KANT = "kant"
    UTILITARIANISM = "utilitarianism"
    DEONTOLOGY = "deontology"
    CONTRACTUALISM = "contractualism"
    VIRTUE_ETHICS = "virtue_ethics"
    CARE_ETHICS = "care_ethics"
    DISCOURSE_ETHICS = "discourse_ethics"
    
    # Theoretical Philosophy (Epistemology, Metaphysics, Logic)
    RATIONALISM = "rationalism"  # Descartes, Spinoza, Leibniz
    EMPIRICISM = "empiricism"  # Locke, Hume, Berkeley
    CRITICAL_PHILOSOPHY = "critical_philosophy"  # Kant's theoretical philosophy
    PHENOMENOLOGY = "phenomenology"  # Husserl, Heidegger
    PRAGMATISM = "pragmatism"  # Peirce, James, Dewey
    ANALYTIC_PHILOSOPHY = "analytic_philosophy"  # Russell, Wittgenstein
    EXISTENTIALISM = "existentialism"  # Kierkegaard, Sartre, Camus
    
    # Lebensphilosophie (Philosophy of Life)
    LEBENSPHILOSOPHIE_NIETZSCHE = "lebensphilosophie_nietzsche"  # Friedrich Nietzsche - Will to Power, Übermensch
    LEBENSPHILOSOPHIE_SCHOPENHAUER = "lebensphilosophie_schopenhauer"  # Arthur Schopenhauer - Will and Representation
    LEBENSPHILOSOPHIE_DILTHEY = "lebensphilosophie_dilthey"  # Wilhelm Dilthey - Hermeneutics, Geisteswissenschaften
    
    # Political Philosophy
    MARXISM = "marxism"  # Karl Marx - Historical Materialism, Class Struggle
    ARENDTIAN = "arendtian"  # Hannah Arendt - Political Action, Plurality, Banality of Evil
    
    # Ancient Greek Philosophy
    SOCRATIC = "socratic"  # Sokrates - Sokratische Methode
    ARISTOTELIAN = "aristotelian"  # Aristoteles - Metaphysik, Logik, Naturphilosophie
    SOPHISM = "sophism"  # Sophisten - Protagoras, Gorgias
    
    # Meta-Ethics
    MORAL_REALISM = "moral_realism"  # Moral facts exist objectively
    MORAL_ANTI_REALISM = "moral_anti_realism"  # Non-cognitivism, Emotivism
    ERROR_THEORY = "error_theory"  # Mackie - all moral claims are false
    EXPRESSIVISM = "expressivism"  # Moral statements express attitudes
    PRESCRIPTIVISM = "prescriptivism"  # Hare - moral judgments are prescriptions
    
    # Historical Schools
    STOICISM = "stoicism"  # Stoic ethics - Seneca, Epictetus, Marcus Aurelius
    EPICUREANISM = "epicureanism"  # Epicurean ethics - pleasure as absence of pain
    CHRISTIAN_ETHICS = "christian_ethics"  # Augustine, Aquinas
    CONFUCIANISM = "confucianism"  # Confucian ethics
    BUDDHIST_ETHICS = "buddhist_ethics"  # Buddhist ethics
    
    # Modern Developments
    NATURALISM = "naturalism"  # Moral properties are natural properties
    INTUITIONISM = "intuitionism"  # Moore, Ross - moral intuition


class ArgumentDimension(Enum):
    """Different dimensions of philosophical analysis."""
    # Practical Philosophy Dimensions
    MORAL = "moral"
    SOCIAL = "social"
    POLITICAL = "political"
    ETHICAL = "ethical"
    ECONOMIC = "economic"
    LEGAL = "legal"
    
    # Theoretical Philosophy Dimensions
    EPISTEMOLOGICAL = "epistemological"  # Theory of knowledge
    METAPHYSICAL = "metaphysical"  # Nature of reality
    LOGICAL = "logical"  # Reasoning and argumentation
    ONTOLOGICAL = "ontological"  # Being and existence
    PHENOMENOLOGICAL = "phenomenological"  # Experience and consciousness
    
    # Meta-Ethics and Ethics Classification
    METAETHICAL = "metaethical"  # Nature of moral judgments, moral language
    NORMATIVE = "normative"  # What we ought to do, moral norms
    APPLIED = "applied"  # Application to specific domains
    DESCRIPTIVE = "descriptive"  # Empirical study of moral systems


class MessageType(Enum):
    """Type of message in the debate chat."""
    STATEMENT = "statement"  # Initial position statement
    COUNTER = "counter"      # Counter-argument
    REBUTTAL = "rebuttal"    # Rebuttal to counter
    SYNTHESIS = "synthesis"  # Synthesis/agreement
    QUESTION = "question"    # Clarifying question


@dataclass
class NewsArticle:
    """Represents a news article for moral analysis."""
    
    id: str = field(default_factory=lambda: str(uuid.uuid4()))
    title: str = ""
    content: str = ""
    source: str = ""
    url: str = ""
    published_date: Optional[datetime] = None
    category: str = ""
    summary: str = ""
    ethical_topics: List[str] = field(default_factory=list)
    
    def to_dict(self) -> Dict[str, Any]:
        """Converts to dictionary for storage."""
        return {
            'id': self.id,
            'title': self.title,
            'content': self.content,
            'source': self.source,
            'url': self.url,
            'published_date': self.published_date.isoformat() if self.published_date else None,
            'category': self.category,
            'summary': self.summary,
            'ethical_topics': self.ethical_topics
        }
    
    @classmethod
    def from_dict(cls, data: Dict[str, Any]) -> 'NewsArticle':
        """Creates from dictionary."""
        article = cls()
        article.id = data.get('id', str(uuid.uuid4()))
        article.title = data.get('title', '')
        article.content = data.get('content', '')
        article.source = data.get('source', '')
        article.url = data.get('url', '')
        
        pub_date = data.get('published_date')
        if pub_date:
            article.published_date = datetime.fromisoformat(pub_date)
        
        article.category = data.get('category', '')
        article.summary = data.get('summary', '')
        article.ethical_topics = data.get('ethical_topics', [])
        return article


@dataclass
class ChatMessage:
    """Represents a message in the philosophical debate chat."""
    
    id: str = field(default_factory=lambda: str(uuid.uuid4()))
    philosophy_school: PhilosophySchool = PhilosophySchool.KANT
    message_type: MessageType = MessageType.STATEMENT
    dimension: ArgumentDimension = ArgumentDimension.MORAL
    content: str = ""
    responds_to: Optional[str] = None  # ID of message being responded to
    timestamp: datetime = field(default_factory=datetime.now)
    
    def to_dict(self) -> Dict[str, Any]:
        """Converts to dictionary."""
        return {
            'id': self.id,
            'philosophy_school': self.philosophy_school.value,
            'message_type': self.message_type.value,
            'dimension': self.dimension.value,
            'content': self.content,
            'responds_to': self.responds_to,
            'timestamp': self.timestamp.isoformat()
        }
    
    @classmethod
    def from_dict(cls, data: Dict[str, Any]) -> 'ChatMessage':
        """Creates from dictionary."""
        msg = cls()
        msg.id = data.get('id', str(uuid.uuid4()))
        msg.philosophy_school = PhilosophySchool(data.get('philosophy_school', 'kant'))
        msg.message_type = MessageType(data.get('message_type', 'statement'))
        msg.dimension = ArgumentDimension(data.get('dimension', 'moral'))
        msg.content = data.get('content', '')
        msg.responds_to = data.get('responds_to')
        
        timestamp = data.get('timestamp')
        if timestamp:
            msg.timestamp = datetime.fromisoformat(timestamp)
        
        return msg


@dataclass
class PhilosophicalArgument:
    """Represents an argument from a specific philosophical perspective."""
    
    id: str = field(default_factory=lambda: str(uuid.uuid4()))
    philosophy_school: PhilosophySchool = PhilosophySchool.KANT
    position: str = ""  # The moral stance
    reasoning: str = ""  # The philosophical reasoning
    key_principles: List[str] = field(default_factory=list)
    counterarguments: List[str] = field(default_factory=list)
    strength: float = 0.0  # 0.0 to 1.0
    dimension: ArgumentDimension = ArgumentDimension.MORAL
    created_at: datetime = field(default_factory=datetime.now)
    
    def to_dict(self) -> Dict[str, Any]:
        """Converts to dictionary."""
        return {
            'id': self.id,
            'philosophy_school': self.philosophy_school.value,
            'position': self.position,
            'reasoning': self.reasoning,
            'key_principles': self.key_principles,
            'counterarguments': self.counterarguments,
            'strength': self.strength,
            'dimension': self.dimension.value,
            'created_at': self.created_at.isoformat()
        }
    
    @classmethod
    def from_dict(cls, data: Dict[str, Any]) -> 'PhilosophicalArgument':
        """Creates from dictionary."""
        arg = cls()
        arg.id = data.get('id', str(uuid.uuid4()))
        arg.philosophy_school = PhilosophySchool(data.get('philosophy_school', 'kant'))
        arg.position = data.get('position', '')
        arg.reasoning = data.get('reasoning', '')
        arg.key_principles = data.get('key_principles', [])
        arg.counterarguments = data.get('counterarguments', [])
        arg.strength = data.get('strength', 0.0)
        arg.dimension = ArgumentDimension(data.get('dimension', 'moral'))
        
        created = data.get('created_at')
        if created:
            arg.created_at = datetime.fromisoformat(created)
        
        return arg


@dataclass
class DebateSession:
    """Represents a complete debate session on a news topic."""
    
    id: str = field(default_factory=lambda: str(uuid.uuid4()))
    news_article: Optional[NewsArticle] = None
    arguments: List[PhilosophicalArgument] = field(default_factory=list)
    chat_messages: List[ChatMessage] = field(default_factory=list)
    debate_topic: str = ""
    ethical_question: str = ""
    active_dimensions: List[ArgumentDimension] = field(default_factory=list)
    consensus_reached: bool = False
    consensus_summary: str = ""
    started_at: datetime = field(default_factory=datetime.now)
    completed_at: Optional[datetime] = None
    current_round: int = 0  # Track debate rounds
    max_duration_minutes: int = 60  # Maximum debate duration (default: 1 hour)
    metadata: Optional[Dict[str, Any]] = None  # For knowledge context, AI synthesis data, etc.
    
    def to_dict(self) -> Dict[str, Any]:
        """Converts to dictionary."""
        return {
            'id': self.id,
            'news_article': self.news_article.to_dict() if self.news_article else None,
            'arguments': [arg.to_dict() for arg in self.arguments],
            'chat_messages': [msg.to_dict() for msg in self.chat_messages],
            'debate_topic': self.debate_topic,
            'ethical_question': self.ethical_question,
            'active_dimensions': [d.value for d in self.active_dimensions],
            'consensus_reached': self.consensus_reached,
            'consensus_summary': self.consensus_summary,
            'started_at': self.started_at.isoformat(),
            'completed_at': self.completed_at.isoformat() if self.completed_at else None,
            'current_round': self.current_round,
            'max_duration_minutes': self.max_duration_minutes,
            'metadata': self.metadata
        }
    
    def get_elapsed_time_minutes(self) -> float:
        """
        Returns the elapsed time of the debate in minutes.
        
        Returns:
            Elapsed time in minutes
        """
        if self.completed_at:
            end_time = self.completed_at
        else:
            end_time = datetime.now()
        
        elapsed = end_time - self.started_at
        return elapsed.total_seconds() / 60.0
    
    def is_time_limit_exceeded(self) -> bool:
        """
        Checks if the debate has exceeded its time limit.
        
        Returns:
            True if time limit exceeded, False otherwise
        """
        return self.get_elapsed_time_minutes() > self.max_duration_minutes
    
    def get_remaining_time_minutes(self) -> float:
        """
        Returns the remaining time for the debate in minutes.
        
        Returns:
            Remaining time in minutes (can be negative if exceeded)
        """
        return self.max_duration_minutes - self.get_elapsed_time_minutes()
    
    @classmethod
    def from_dict(cls, data: Dict[str, Any]) -> 'DebateSession':
        """Creates from dictionary."""
        session = cls()
        session.id = data.get('id', str(uuid.uuid4()))
        
        article_data = data.get('news_article')
        if article_data:
            session.news_article = NewsArticle.from_dict(article_data)
        
        session.arguments = [
            PhilosophicalArgument.from_dict(arg_data)
            for arg_data in data.get('arguments', [])
        ]
        
        session.chat_messages = [
            ChatMessage.from_dict(msg_data)
            for msg_data in data.get('chat_messages', [])
        ]
        
        session.debate_topic = data.get('debate_topic', '')
        session.ethical_question = data.get('ethical_question', '')
        session.active_dimensions = [
            ArgumentDimension(d) for d in data.get('active_dimensions', [])
        ]
        session.consensus_reached = data.get('consensus_reached', False)
        session.consensus_summary = data.get('consensus_summary', '')
        session.current_round = data.get('current_round', 0)
        session.max_duration_minutes = data.get('max_duration_minutes', 60)
        session.metadata = data.get('metadata')
        
        started = data.get('started_at')
        if started:
            session.started_at = datetime.fromisoformat(started)
        
        completed = data.get('completed_at')
        if completed:
            session.completed_at = datetime.fromisoformat(completed)
        
        return session


@dataclass
class PhilosophyProfile:
    """Profile defining a philosophical school's approach."""
    
    school: PhilosophySchool
    name: str
    philosopher_name: str  # Name of the philosopher (e.g., "Immanuel Kant")
    description: str
    core_principles: List[str]
    decision_framework: str
    example_application: str
    
    def to_dict(self) -> Dict[str, Any]:
        """Converts to dictionary."""
        return {
            'school': self.school.value,
            'name': self.name,
            'philosopher_name': self.philosopher_name,
            'description': self.description,
            'core_principles': self.core_principles,
            'decision_framework': self.decision_framework,
            'example_application': self.example_application
        }
    
    @classmethod
    def from_dict(cls, data: Dict[str, Any]) -> 'PhilosophyProfile':
        """Creates from dictionary."""
        return cls(
            school=PhilosophySchool(data['school']),
            name=data['name'],
            philosopher_name=data['philosopher_name'],
            description=data['description'],
            core_principles=data['core_principles'],
            decision_framework=data['decision_framework'],
            example_application=data['example_application']
        )


# Pre-defined philosophy profiles
# Diese werden nun dynamisch aus YAML-Dateien geladen
# Siehe philosophy_loader.py für den Lade-Mechanismus

def _load_philosophy_profiles_from_yaml() -> Dict[PhilosophySchool, PhilosophyProfile]:
    """
    Lädt Philosophy-Profile aus YAML-Dateien.
    Fallback zu hardcodierten Profilen wenn YAML-Dateien nicht verfügbar.
    """
    try:
        # Versuche YAML-Dateien zu laden
        from philosophy_loader import load_philosophy_profiles
        profiles = load_philosophy_profiles()
        if profiles:
            print(f"✓ {len(profiles)} Philosophy-Profile aus YAML geladen")
            return profiles
    except Exception as e:
        print(f"⚠ Konnte YAML-Profile nicht laden: {e}")
    
    # Fallback: Hart-codierte Profile
    print("ℹ Verwende hart-codierte Philosophy-Profile als Fallback")
    return _get_hardcoded_profiles()


def _get_hardcoded_profiles() -> Dict[PhilosophySchool, PhilosophyProfile]:
    """Fallback: Hart-codierte Profile wenn YAML nicht verfügbar."""
    return {
    PhilosophySchool.KANT: PhilosophyProfile(
        school=PhilosophySchool.KANT,
        name="Kantian Ethics",
        philosopher_name="Immanuel Kant",
        description="Based on Immanuel Kant's categorical imperative and duty-based ethics",
        core_principles=[
            "Act only according to maxims that could become universal laws",
            "Treat people as ends in themselves, never merely as means",
            "Moral worth comes from duty and good will, not consequences",
            "Autonomy and rational agency are fundamental"
        ],
        decision_framework="Evaluate actions based on whether they respect human dignity and could be universalized",
        example_application="Lying is wrong because if everyone lied, communication would be impossible"
    ),
    PhilosophySchool.UTILITARIANISM: PhilosophyProfile(
        school=PhilosophySchool.UTILITARIANISM,
        name="Utilitarianism",
        philosopher_name="John Stuart Mill",
        description="Consequentialist ethics focused on maximizing overall happiness and well-being",
        core_principles=[
            "The right action maximizes utility (happiness/well-being)",
            "Consider consequences for all affected parties equally",
            "Greatest happiness for the greatest number",
            "Outcomes matter more than intentions or rules"
        ],
        decision_framework="Calculate expected utility by weighing pleasure against pain for all stakeholders",
        example_application="A policy is justified if it produces more net benefit than harm across society"
    ),
    PhilosophySchool.DEONTOLOGY: PhilosophyProfile(
        school=PhilosophySchool.DEONTOLOGY,
        name="Deontological Ethics",
        philosopher_name="W.D. Ross",
        description="Duty-based ethics emphasizing moral rules and obligations",
        core_principles=[
            "Some actions are intrinsically right or wrong",
            "Moral duties and rules must be followed",
            "Rights and obligations take precedence over consequences",
            "Justice and fairness are paramount"
        ],
        decision_framework="Identify relevant moral duties and rights, then determine which apply",
        example_application="Breaking a promise is wrong even if it leads to better outcomes"
    ),
    PhilosophySchool.CONTRACTUALISM: PhilosophyProfile(
        school=PhilosophySchool.CONTRACTUALISM,
        name="Contractualism",
        philosopher_name="John Rawls",
        description="Ethics based on mutual agreement and what reasonable people would accept",
        core_principles=[
            "Moral rules are those no one could reasonably reject",
            "Focus on justifiability to others",
            "Equal consideration of all perspectives",
            "Fairness and reciprocity are central"
        ],
        decision_framework="Ask: Could anyone reasonably object to this principle governing our conduct?",
        example_application="A policy is fair if it respects everyone's reasonable claims and interests"
    ),
    PhilosophySchool.VIRTUE_ETHICS: PhilosophyProfile(
        school=PhilosophySchool.VIRTUE_ETHICS,
        name="Virtue Ethics",
        philosopher_name="Aristoteles",
        description="Character-based ethics emphasizing moral virtues and human flourishing",
        core_principles=[
            "Focus on developing good character traits (virtues)",
            "Pursue eudaimonia (human flourishing)",
            "Practical wisdom (phronesis) guides moral judgment",
            "Community and relationships are important"
        ],
        decision_framework="Ask: What would a virtuous person do in this situation?",
        example_application="Acting courageously, honestly, and compassionately reflects good character"
    ),
    PhilosophySchool.CARE_ETHICS: PhilosophyProfile(
        school=PhilosophySchool.CARE_ETHICS,
        name="Care Ethics",
        philosopher_name="Carol Gilligan",
        description="Feminist ethics emphasizing care relationships and contextual moral reasoning",
        core_principles=[
            "Moral reasoning emerges from relationships and care",
            "Context and particularity matter in ethical judgment",
            "Emotions and empathy are central to morality",
            "Interconnectedness and interdependence are fundamental"
        ],
        decision_framework="Consider relationships, needs, and the context of care in moral situations",
        example_application="Caring for vulnerable others is a primary moral obligation"
    ),
    PhilosophySchool.DISCOURSE_ETHICS: PhilosophyProfile(
        school=PhilosophySchool.DISCOURSE_ETHICS,
        name="Discourse Ethics",
        philosopher_name="Jürgen Habermas",
        description="Ethics based on rational discourse and communicative action",
        core_principles=[
            "Valid norms must be acceptable to all in rational discourse",
            "Discourse requires ideal speech conditions",
            "Moral validity comes from consensus",
            "Communicative rationality underlies ethics"
        ],
        decision_framework="Seek consensus through open, rational dialogue among all affected parties",
        example_application="Democratic deliberation reveals morally valid norms"
    ),
    
    # Theoretical Philosophy
    PhilosophySchool.RATIONALISM: PhilosophyProfile(
        school=PhilosophySchool.RATIONALISM,
        name="Rationalism",
        philosopher_name="René Descartes",
        description="Knowledge derives from reason and innate ideas, not sensory experience",
        core_principles=[
            "Clear and distinct ideas are the foundation of knowledge",
            "Reason is the primary source of knowledge",
            "Some truths are knowable a priori (independent of experience)",
            "Innate ideas exist in the mind from birth"
        ],
        decision_framework="Analyze through rational deduction from self-evident first principles",
        example_application="Mathematical truths are known through pure reason, not observation"
    ),
    PhilosophySchool.EMPIRICISM: PhilosophyProfile(
        school=PhilosophySchool.EMPIRICISM,
        name="Empiricism",
        philosopher_name="David Hume",
        description="All knowledge originates from sensory experience",
        core_principles=[
            "Experience is the sole source of knowledge",
            "No innate ideas - mind is a 'tabula rasa' at birth",
            "Ideas are copies of impressions (sense data)",
            "Causation is habitual association, not necessary connection"
        ],
        decision_framework="Trace all concepts back to original sensory impressions",
        example_application="We believe the sun will rise because of past experience, not logical necessity"
    ),
    PhilosophySchool.CRITICAL_PHILOSOPHY: PhilosophyProfile(
        school=PhilosophySchool.CRITICAL_PHILOSOPHY,
        name="Critical Philosophy",
        philosopher_name="Immanuel Kant",
        description="Knowledge requires both sensory experience and rational concepts",
        core_principles=[
            "Synthetic a priori knowledge is possible",
            "Mind actively structures experience through categories",
            "Phenomena (appearances) vs. noumena (things-in-themselves)",
            "Limits of reason - what can we know?"
        ],
        decision_framework="Examine conditions of possibility for knowledge and experience",
        example_application="Space and time are forms of intuition, not properties of things themselves"
    ),
    PhilosophySchool.PHENOMENOLOGY: PhilosophyProfile(
        school=PhilosophySchool.PHENOMENOLOGY,
        name="Phenomenology",
        philosopher_name="Edmund Husserl",
        description="Study of consciousness and direct experience as it appears",
        core_principles=[
            "Return to 'the things themselves' - direct experience",
            "Intentionality - consciousness is always consciousness of something",
            "Bracketing (epoché) - suspend assumptions about external reality",
            "Essential structures of consciousness"
        ],
        decision_framework="Describe phenomena as they appear in consciousness without presuppositions",
        example_application="Analyze the experience of perceiving a tree, not whether the tree really exists"
    ),
    PhilosophySchool.PRAGMATISM: PhilosophyProfile(
        school=PhilosophySchool.PRAGMATISM,
        name="Pragmatism",
        philosopher_name="William James",
        description="Truth is what works in practice; ideas are tools for action",
        core_principles=[
            "Truth is verified through practical consequences",
            "Ideas are instruments for dealing with reality",
            "Meaning is defined by practical effects",
            "Inquiry is problem-solving"
        ],
        decision_framework="Evaluate beliefs by their practical utility and consequences",
        example_application="A belief is true if it leads to successful action and prediction"
    ),
    PhilosophySchool.ANALYTIC_PHILOSOPHY: PhilosophyProfile(
        school=PhilosophySchool.ANALYTIC_PHILOSOPHY,
        name="Analytic Philosophy",
        philosopher_name="Ludwig Wittgenstein",
        description="Philosophy through logical and linguistic analysis",
        core_principles=[
            "Clarity through logical analysis of language",
            "Many philosophical problems are linguistic confusions",
            "Meaning is use in language games",
            "Limits of language are limits of thought"
        ],
        decision_framework="Analyze logical structure of propositions and linguistic usage",
        example_application="'What is the meaning of life?' may be a pseudo-problem arising from misuse of 'meaning'"
    ),
    PhilosophySchool.EXISTENTIALISM: PhilosophyProfile(
        school=PhilosophySchool.EXISTENTIALISM,
        name="Existentialism",
        philosopher_name="Jean-Paul Sartre",
        description="Existence precedes essence; humans create their own meaning through free choice",
        core_principles=[
            "Existence precedes essence - we define ourselves through choices",
            "Radical freedom and responsibility",
            "Authenticity vs. bad faith",
            "Absurdity of existence without inherent meaning"
        ],
        decision_framework="Acknowledge freedom, take responsibility, live authentically",
        example_application="We are 'condemned to be free' - no predetermined human nature determines our choices"
    ),
    
    # Ancient Greek Philosophy
    PhilosophySchool.SOCRATIC: PhilosophyProfile(
        school=PhilosophySchool.SOCRATIC,
        name="Socratic Philosophy",
        philosopher_name="Sokrates",
        description="Philosophy through dialectical questioning (Socratic method) to expose contradictions and reach truth",
        core_principles=[
            "Know thyself (Erkenne dich selbst)",
            "The unexamined life is not worth living",
            "Virtue is knowledge - no one does wrong willingly",
            "Dialectical method through questioning (Elenchus)"
        ],
        decision_framework="Question assumptions systematically until contradictions emerge; true knowledge requires self-awareness",
        example_application="Through questioning, reveal that someone claiming to know courage actually contradicts themselves"
    ),
    PhilosophySchool.ARISTOTELIAN: PhilosophyProfile(
        school=PhilosophySchool.ARISTOTELIAN,
        name="Aristotelian Philosophy",
        philosopher_name="Aristoteles",
        description="Comprehensive system spanning metaphysics, logic, natural philosophy, and ethics",
        core_principles=[
            "Substance and essence - what makes a thing what it is",
            "Four causes: material, formal, efficient, final",
            "Potentiality and actuality (dynamis and energeia)",
            "Logic as organon (tool for reasoning)"
        ],
        decision_framework="Analyze through categories, identify essence and purpose (telos), apply syllogistic logic",
        example_application="A thing is understood through its matter, form, maker, and purpose"
    ),
    PhilosophySchool.SOPHISM: PhilosophyProfile(
        school=PhilosophySchool.SOPHISM,
        name="Sophism",
        philosopher_name="Protagoras",
        description="Relativist philosophy emphasizing rhetoric, persuasion, and practical wisdom over absolute truth",
        core_principles=[
            "Man is the measure of all things (Homo mensura)",
            "Truth is relative to the individual or culture",
            "Rhetoric and persuasion are central to knowledge",
            "Practical success over theoretical certainty"
        ],
        decision_framework="Consider multiple perspectives, use persuasive argument, focus on practical outcomes",
        example_application="What seems true to a person is true for that person - no absolute truth exists"
    ),
    
    # Meta-Ethics
    PhilosophySchool.MORAL_REALISM: PhilosophyProfile(
        school=PhilosophySchool.MORAL_REALISM,
        name="Moral Realism",
        philosopher_name="G.E. Moore",
        description="Moral facts exist objectively and can be known through reason or intuition",
        core_principles=[
            "Moral properties exist independently of human beliefs",
            "Moral statements can be true or false",
            "'Good' is a simple, unanalyzable property (Moore)",
            "Moral knowledge is possible through rational intuition"
        ],
        decision_framework="Identify objective moral facts through careful moral reasoning and intuition",
        example_application="'Torture is wrong' is objectively true, not merely an expression of disapproval"
    ),
    PhilosophySchool.MORAL_ANTI_REALISM: PhilosophyProfile(
        school=PhilosophySchool.MORAL_ANTI_REALISM,
        name="Moral Anti-Realism / Emotivism",
        philosopher_name="A.J. Ayer",
        description="Moral statements do not describe facts but express emotions or attitudes",
        core_principles=[
            "Moral statements are neither true nor false (non-cognitive)",
            "'Murder is wrong' means 'Murder - boo!' (Ayer)",
            "Moral language expresses feelings, not beliefs",
            "No objective moral facts exist"
        ],
        decision_framework="Recognize moral discourse as expression of sentiment rather than factual claims",
        example_application="'Stealing is wrong' expresses my disapproval, doesn't state a fact"
    ),
    PhilosophySchool.ERROR_THEORY: PhilosophyProfile(
        school=PhilosophySchool.ERROR_THEORY,
        name="Error Theory",
        philosopher_name="J.L. Mackie",
        description="Moral statements make claims about objective values, but all such claims are false",
        core_principles=[
            "Moral discourse presupposes objective values",
            "But no objective values exist",
            "Therefore all moral claims are systematically false",
            "Argument from queerness - moral facts would be metaphysically odd"
        ],
        decision_framework="Acknowledge that while we speak as if morality is objective, we are systematically mistaken",
        example_application="When I say 'charity is good', I'm making a false claim about objective goodness that doesn't exist"
    ),
    PhilosophySchool.EXPRESSIVISM: PhilosophyProfile(
        school=PhilosophySchool.EXPRESSIVISM,
        name="Expressivism",
        philosopher_name="Simon Blackburn",
        description="Moral statements express pro or con attitudes toward actions without describing facts",
        core_principles=[
            "Moral judgments express attitudes, not beliefs",
            "Quasi-realism - explain why moral discourse seems factual",
            "Can accommodate moral reasoning and disagreement",
            "Projectivism - we project values onto the world"
        ],
        decision_framework="Express attitudes while recognizing the projective nature of moral discourse",
        example_application="Moral disagreement is disagreement in attitude, not factual disagreement"
    ),
    PhilosophySchool.PRESCRIPTIVISM: PhilosophyProfile(
        school=PhilosophySchool.PRESCRIPTIVISM,
        name="Prescriptivism / Universal Prescriptivism",
        philosopher_name="R.M. Hare",
        description="Moral judgments are universal prescriptions - commands that apply to all similar cases",
        core_principles=[
            "Moral statements are prescriptive (action-guiding), not descriptive",
            "Universalizability - moral judgments apply to all relevantly similar cases",
            "Overridingness - moral judgments override other considerations",
            "Two levels: intuitive (everyday rules) and critical (utilitarian reasoning)"
        ],
        decision_framework="Make prescriptions you're willing to universalize and apply even if you were in others' positions",
        example_application="'Lying is wrong' means 'Don't lie!' - a universal prescription for all similar situations"
    ),
    
    # Historical Schools
    PhilosophySchool.STOICISM: PhilosophyProfile(
        school=PhilosophySchool.STOICISM,
        name="Stoicism",
        philosopher_name="Seneca",
        description="Ancient philosophy emphasizing virtue, reason, and acceptance of what we cannot control",
        core_principles=[
            "Live according to nature and reason (logos)",
            "Virtue is the only true good",
            "Control what you can control (your judgments), accept what you cannot",
            "Apatheia - freedom from destructive passions through reason"
        ],
        decision_framework="Distinguish what is in your control from what is not; act virtuously on what you control",
        example_application="Focus on your character and choices, not on external events beyond your control"
    ),
    PhilosophySchool.EPICUREANISM: PhilosophyProfile(
        school=PhilosophySchool.EPICUREANISM,
        name="Epicureanism",
        philosopher_name="Epikur",
        description="Philosophy aiming at ataraxia (tranquility) through moderate pleasure and absence of pain",
        core_principles=[
            "Pleasure (hedone) as absence of pain (aponia) and mental disturbance (ataraxia)",
            "Simple pleasures are best - avoid excessive desires",
            "Death is nothing to fear - annihilation means no suffering",
            "Friendship and philosophy lead to happiness"
        ],
        decision_framework="Seek simple pleasures, avoid pain, cultivate friendships, and practice philosophy",
        example_application="Choose modest pleasures that don't lead to greater pain; a simple meal with friends over luxury"
    ),
    PhilosophySchool.CHRISTIAN_ETHICS: PhilosophyProfile(
        school=PhilosophySchool.CHRISTIAN_ETHICS,
        name="Christian Ethics",
        philosopher_name="Thomas von Aquin",
        description="Ethics based on divine command, natural law, and virtues grounded in God's will",
        core_principles=[
            "Love of God and neighbor (agape)",
            "Natural law - reason reflects God's eternal law",
            "Theological virtues: faith, hope, charity",
            "Human dignity as image of God (imago Dei)"
        ],
        decision_framework="Seek God's will through scripture, reason, and natural law; act with love",
        example_application="The moral law is knowable through reason and revelation; loving one's neighbor is paramount"
    ),
    PhilosophySchool.CONFUCIANISM: PhilosophyProfile(
        school=PhilosophySchool.CONFUCIANISM,
        name="Confucianism",
        philosopher_name="Konfuzius",
        description="Ethical system emphasizing social harmony, ritual propriety, and cultivation of virtue",
        core_principles=[
            "Ren (humaneness/benevolence) - care for others",
            "Li (ritual propriety) - proper conduct in social roles",
            "Filial piety (xiao) - respect for parents and ancestors",
            "Junzi (exemplary person) - cultivated moral character"
        ],
        decision_framework="Cultivate virtue through learning, ritual, and fulfilling social roles properly",
        example_application="Act according to your social role with benevolence, maintaining harmony through proper conduct"
    ),
    PhilosophySchool.BUDDHIST_ETHICS: PhilosophyProfile(
        school=PhilosophySchool.BUDDHIST_ETHICS,
        name="Buddhist Ethics",
        philosopher_name="Buddha",
        description="Ethics aimed at ending suffering through the Noble Eightfold Path and compassion",
        core_principles=[
            "Four Noble Truths - suffering, its cause, its cessation, the path",
            "Eightfold Path - right view, intention, speech, action, livelihood, effort, mindfulness, concentration",
            "Compassion (karuna) and loving-kindness (metta) for all beings",
            "Non-harm (ahimsa) - avoid causing suffering"
        ],
        decision_framework="Follow the Eightfold Path, practice compassion, minimize harm to all sentient beings",
        example_application="Act with mindfulness and compassion to reduce suffering for yourself and others"
    ),
    PhilosophySchool.NATURALISM: PhilosophyProfile(
        school=PhilosophySchool.NATURALISM,
        name="Ethical Naturalism",
        philosopher_name="Philippa Foot",
        description="Moral properties are natural properties; ethics can be grounded in human nature and flourishing",
        core_principles=[
            "Moral facts are natural facts about human well-being",
            "Ethics based on human nature and function",
            "Virtues are traits that promote human flourishing",
            "No 'naturalistic fallacy' - 'ought' can derive from 'is'"
        ],
        decision_framework="Analyze human nature and needs to determine what promotes flourishing",
        example_application="Courage is a virtue because humans with this trait genuinely flourish better"
    ),
    PhilosophySchool.INTUITIONISM: PhilosophyProfile(
        school=PhilosophySchool.INTUITIONISM,
        name="Ethical Intuitionism",
        philosopher_name="W.D. Ross",
        description="Moral truths are self-evident and known through rational intuition",
        core_principles=[
            "Prima facie duties are intuitively known",
            "Moral properties are non-natural but knowable",
            "Rational intuition grasps moral truths",
            "Multiple duties may conflict - requires judgment"
        ],
        decision_framework="Intuit prima facie duties, weigh them in context, and act on strongest duty",
        example_application="We intuitively know promise-keeping is a duty, but it may be overridden by preventing great harm"
    ),
    PhilosophySchool.MARXISM: PhilosophyProfile(
        school=PhilosophySchool.MARXISM,
        name="Marxism",
        philosopher_name="Karl Marx",
        description="Historical materialism analyzing society through class struggle and economic relations; critique of capitalism",
        core_principles=[
            "Historical materialism - economic base determines social superstructure",
            "Class struggle as driver of historical change",
            "Alienation under capitalism - workers alienated from labor, product, species-being",
            "Exploitation through surplus value extraction",
            "Communist society as resolution of contradictions",
            "Dialectical development of history"
        ],
        decision_framework="Analyze through lens of class relations, material conditions, and economic interests; work toward classless society",
        example_application="Evaluate ethical issues by examining underlying economic structures and class interests that produce them"
    ),
    PhilosophySchool.LEBENSPHILOSOPHIE_NIETZSCHE: PhilosophyProfile(
        school=PhilosophySchool.LEBENSPHILOSOPHIE_NIETZSCHE,
        name="Nietzschean Lebensphilosophie",
        philosopher_name="Friedrich Nietzsche",
        description="Philosophy of life emphasizing will to power, perspectivism, and transvaluation of values; critique of traditional morality",
        core_principles=[
            "Will to power - fundamental drive of life",
            "Übermensch (Overman) - self-overcoming and creation of values",
            "Eternal recurrence - amor fati, love of fate",
            "Perspectivism - no absolute truth, only interpretations",
            "Critique of slave morality vs master morality",
            "Transvaluation of all values - beyond good and evil",
            "Life-affirmation over nihilism"
        ],
        decision_framework="Affirm life in all its aspects; create your own values; strive for self-overcoming and excellence",
        example_application="Rather than following herd morality, the strong individual creates values that affirm and enhance life"
    ),
    PhilosophySchool.LEBENSPHILOSOPHIE_SCHOPENHAUER: PhilosophyProfile(
        school=PhilosophySchool.LEBENSPHILOSOPHIE_SCHOPENHAUER,
        name="Schopenhauerian Lebensphilosophie",
        philosopher_name="Arthur Schopenhauer",
        description="Philosophy of life viewing world as will and representation; pessimistic view of existence and ethics of compassion",
        core_principles=[
            "World as Will and Representation - underlying reality is blind will",
            "Life is suffering - will leads to endless striving and dissatisfaction",
            "Principium individuationis - illusion of separation between beings",
            "Compassion (Mitleid) - recognition of shared suffering",
            "Aesthetic contemplation and asceticism as escape from will",
            "Denial of will-to-live as path to liberation",
            "Pessimism about human condition"
        ],
        decision_framework="Recognize shared suffering; act with compassion; minimize harm; seek aesthetic and ascetic transcendence",
        example_application="All beings share the same underlying will; causing suffering to others is harming oneself"
    ),
    PhilosophySchool.LEBENSPHILOSOPHIE_DILTHEY: PhilosophyProfile(
        school=PhilosophySchool.LEBENSPHILOSOPHIE_DILTHEY,
        name="Diltheyan Lebensphilosophie",
        philosopher_name="Wilhelm Dilthey",
        description="Hermeneutic philosophy emphasizing understanding of lived experience (Erlebnis) and human sciences",
        core_principles=[
            "Geisteswissenschaften (human sciences) vs Naturwissenschaften (natural sciences)",
            "Verstehen (understanding) vs Erklären (explaining)",
            "Erlebnis (lived experience) as basis of understanding",
            "Historical consciousness - humans are historical beings",
            "Hermeneutic circle - whole and parts mutually illuminate",
            "Life expresses itself in structured forms",
            "Meaning arises from lived context"
        ],
        decision_framework="Understand actions through their historical and lived context; interpret meaning holistically",
        example_application="Ethical judgments must be understood within their historical life-context, not as abstract universals"
    ),
    PhilosophySchool.ARENDTIAN: PhilosophyProfile(
        school=PhilosophySchool.ARENDTIAN,
        name="Arendtian Political Philosophy",
        philosopher_name="Hannah Arendt",
        description="Political philosophy emphasizing plurality, public action, and the vita activa; analysis of totalitarianism and the banality of evil",
        core_principles=[
            "Vita activa - labor, work, action as fundamental human activities",
            "Plurality - humans are distinct yet equal; condition of political life",
            "Action in the public sphere - where freedom and meaning emerge",
            "Natality - capacity to begin something new",
            "Banality of evil - thoughtlessness can lead to great evil (Eichmann)",
            "Distinction between public and private realms",
            "Judgment - faculty to think from standpoint of others",
            "Politics as space of appearance and speech"
        ],
        decision_framework="Act in public sphere with others; think from multiple perspectives; take responsibility for the world",
        example_application="Evil arises not from monsters but from thoughtless bureaucrats who fail to judge and take responsibility"
    )
}


# Initialisiere PHILOSOPHY_PROFILES durch Laden aus YAML
PHILOSOPHY_PROFILES = _load_philosophy_profiles_from_yaml()

