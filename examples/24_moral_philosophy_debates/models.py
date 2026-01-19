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
            'current_round': self.current_round
        }
    
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
PHILOSOPHY_PROFILES = {
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
    )
}
