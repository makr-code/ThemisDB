"""
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            standalone_moral_dialectic.py                      ║
  Version:         0.0.38                                             ║
  Last Modified:   2026-04-13 04:12:49                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     812                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 2a1fb04231  2026-03-03  Merge branch 'develop' into copilot/audit-src-module-docu... ║
    • a629043ab2  2026-02-22  Audit: document gaps found - benchmarks and stale annotat... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
"""

#!/usr/bin/env python3
"""
Standalone Moral Philosophy Dialectic Engine
Uses SQLite for storage and Ollama for local LLM inference

This implementation is completely independent of ThemisDB and provides
a lightweight solution for generating philosophical debates on ethical questions.
"""

import argparse
import json
import random
import sqlite3
import time
import uuid
from dataclasses import dataclass, field, asdict
from datetime import datetime
from enum import Enum
from typing import List, Optional, Dict, Any

import requests


# ============================================================================
# Data Models
# ============================================================================

class PhilosophySchool(Enum):
    """Different schools of philosophy."""
    KANT = "kant"
    UTILITARIANISM = "utilitarianism"
    VIRTUE_ETHICS = "virtue_ethics"
    SOCRATIC = "socratic"
    STOICISM = "stoicism"
    USER = "user"  # User messages


class ArgumentDimension(Enum):
    """Different dimensions of philosophical analysis."""
    MORAL = "moral"
    ETHICAL = "ethical"
    SOCIAL = "social"
    POLITICAL = "political"


class MessageType(Enum):
    """Type of message in the debate."""
    STATEMENT = "statement"      # Initial position statement
    COUNTER = "counter"          # Counter-argument
    USER = "user"                # User contribution


@dataclass
class ChatMessage:
    """A single message in the philosophical dialogue."""
    id: str
    philosophy_school: PhilosophySchool
    message_type: MessageType
    dimension: ArgumentDimension
    content: str
    responds_to: Optional[str] = None
    timestamp: datetime = field(default_factory=datetime.now)
    
    def to_dict(self) -> Dict[str, Any]:
        """Convert to dictionary for serialization."""
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
        """Create from dictionary."""
        return cls(
            id=data['id'],
            philosophy_school=PhilosophySchool(data['philosophy_school']),
            message_type=MessageType(data['message_type']),
            dimension=ArgumentDimension(data['dimension']),
            content=data['content'],
            responds_to=data.get('responds_to'),
            timestamp=datetime.fromisoformat(data['timestamp'])
        )


@dataclass
class DebateSession:
    """A complete debate session."""
    id: str
    topic: str
    ethical_question: str
    messages: List[ChatMessage] = field(default_factory=list)
    created_at: datetime = field(default_factory=datetime.now)
    completed_at: Optional[datetime] = None
    
    def to_dict(self) -> Dict[str, Any]:
        """Convert to dictionary for serialization."""
        return {
            'id': self.id,
            'topic': self.topic,
            'ethical_question': self.ethical_question,
            'messages': [msg.to_dict() for msg in self.messages],
            'created_at': self.created_at.isoformat(),
            'completed_at': self.completed_at.isoformat() if self.completed_at else None
        }
    
    @classmethod
    def from_dict(cls, data: Dict[str, Any]) -> 'DebateSession':
        """Create from dictionary."""
        return cls(
            id=data['id'],
            topic=data['topic'],
            ethical_question=data['ethical_question'],
            messages=[ChatMessage.from_dict(msg) for msg in data.get('messages', [])],
            created_at=datetime.fromisoformat(data['created_at']),
            completed_at=datetime.fromisoformat(data['completed_at']) if data.get('completed_at') else None
        )


# ============================================================================
# Philosophy Profiles
# ============================================================================

PHILOSOPHY_PROFILES = {
    PhilosophySchool.KANT: {
        "name": "Immanuel Kant",
        "description": "Kantian ethics (Kategorischer Imperativ)",
        "system_prompt": """You are Immanuel Kant, the renowned German philosopher. You analyze ethical questions 
through the lens of the categorical imperative: "Act only according to that maxim whereby you can at the same 
time will that it should become a universal law." You emphasize:
- Duty over consequences
- Respect for human dignity and autonomy
- Universal moral principles
- Rationality in ethics
Speak in first person as Kant. Keep responses focused and under 150 words.""",
        "core_principles": [
            "Categorical Imperative",
            "Human dignity as end in itself",
            "Autonomy and self-legislation",
            "Duty over inclination"
        ]
    },
    PhilosophySchool.UTILITARIANISM: {
        "name": "John Stuart Mill",
        "description": "Utilitarianism (Greatest Happiness Principle)",
        "system_prompt": """You are John Stuart Mill, advocate of utilitarianism. You evaluate ethical questions 
by the principle of utility: actions are right if they promote happiness, wrong if they produce the reverse. 
You emphasize:
- Greatest happiness for the greatest number
- Consequences matter more than intentions
- Quality of pleasure matters (higher vs lower pleasures)
- Impartial consideration of all interests
Speak in first person as Mill. Keep responses focused and under 150 words.""",
        "core_principles": [
            "Greatest happiness principle",
            "Consequentialism",
            "Impartial consideration",
            "Utility maximization"
        ]
    },
    PhilosophySchool.VIRTUE_ETHICS: {
        "name": "Aristotle",
        "description": "Virtue Ethics (Eudaimonia and the Golden Mean)",
        "system_prompt": """You are Aristotle, founder of virtue ethics. You analyze ethical questions through 
character development and human flourishing (eudaimonia). You emphasize:
- Virtues as character excellences
- The golden mean between extremes
- Practical wisdom (phronesis)
- Human flourishing as the ultimate goal
Speak in first person as Aristotle. Keep responses focused and under 150 words.""",
        "core_principles": [
            "Virtue and character",
            "Golden mean",
            "Practical wisdom (phronesis)",
            "Human flourishing (eudaimonia)"
        ]
    },
    PhilosophySchool.SOCRATIC: {
        "name": "Socrates",
        "description": "Socratic Method (Dialectical Questioning)",
        "system_prompt": """You are Socrates, the classical Greek philosopher. You analyze ethical questions 
through dialectical questioning, seeking to uncover truth through dialogue. You emphasize:
- Questioning assumptions
- Examining definitions
- Logical consistency
- Intellectual humility ("I know that I know nothing")
Speak in first person as Socrates. Keep responses focused and under 150 words.""",
        "core_principles": [
            "Dialectical method",
            "Question everything",
            "Know thyself",
            "Virtue is knowledge"
        ]
    },
    PhilosophySchool.STOICISM: {
        "name": "Epictetus",
        "description": "Stoicism (Virtue and Acceptance)",
        "system_prompt": """You are Epictetus, the Stoic philosopher. You analyze ethical questions through 
the lens of Stoic ethics: virtue as the only true good, acceptance of what we cannot control. You emphasize:
- Distinguishing what is in our control from what is not
- Virtue as sufficient for happiness
- Living in accordance with nature and reason
- Accepting fate with equanimity
Speak in first person as Epictetus. Keep responses focused and under 150 words.""",
        "core_principles": [
            "Dichotomy of control",
            "Virtue as the only good",
            "Living according to nature",
            "Acceptance and equanimity"
        ]
    },
    PhilosophySchool.USER: {
        "name": "User",
        "description": "User contribution",
        "system_prompt": "",  # No system prompt for user
        "core_principles": []
    }
}


# ============================================================================
# SQLite Database Store
# ============================================================================

class SQLiteDebateStore:
    """Manages persistent storage of debates in SQLite."""
    
    def __init__(self, db_path: str = "moral_debates.db"):
        """Initialize the database store.
        
        Args:
            db_path: Path to the SQLite database file
        """
        self.db_path = db_path
        self.conn = None
        self._init_database()
    
    def _init_database(self):
        """Initialize database schema."""
        self.conn = sqlite3.connect(self.db_path)
        cursor = self.conn.cursor()
        
        # Create debates table
        cursor.execute("""
            CREATE TABLE IF NOT EXISTS debates (
                id TEXT PRIMARY KEY,
                topic TEXT NOT NULL,
                ethical_question TEXT NOT NULL,
                created_at TEXT NOT NULL,
                completed_at TEXT,
                data TEXT NOT NULL
            )
        """)
        
        # Create messages table
        cursor.execute("""
            CREATE TABLE IF NOT EXISTS messages (
                id TEXT PRIMARY KEY,
                debate_id TEXT NOT NULL,
                philosophy_school TEXT NOT NULL,
                message_type TEXT NOT NULL,
                dimension TEXT NOT NULL,
                content TEXT NOT NULL,
                responds_to TEXT,
                timestamp TEXT NOT NULL,
                FOREIGN KEY (debate_id) REFERENCES debates(id)
            )
        """)
        
        # Create indices
        cursor.execute("""
            CREATE INDEX IF NOT EXISTS idx_messages_debate_id 
            ON messages(debate_id)
        """)
        
        cursor.execute("""
            CREATE INDEX IF NOT EXISTS idx_messages_timestamp 
            ON messages(timestamp)
        """)
        
        self.conn.commit()
    
    def save_debate(self, session: DebateSession) -> bool:
        """Save a debate session to the database.
        
        Args:
            session: The debate session to save
            
        Returns:
            True if successful, False otherwise
        """
        try:
            cursor = self.conn.cursor()
            
            # Save debate metadata
            cursor.execute("""
                INSERT OR REPLACE INTO debates 
                (id, topic, ethical_question, created_at, completed_at, data)
                VALUES (?, ?, ?, ?, ?, ?)
            """, (
                session.id,
                session.topic,
                session.ethical_question,
                session.created_at.isoformat(),
                session.completed_at.isoformat() if session.completed_at else None,
                json.dumps(session.to_dict())
            ))
            
            # Save messages
            for message in session.messages:
                cursor.execute("""
                    INSERT OR REPLACE INTO messages
                    (id, debate_id, philosophy_school, message_type, dimension, 
                     content, responds_to, timestamp)
                    VALUES (?, ?, ?, ?, ?, ?, ?, ?)
                """, (
                    message.id,
                    session.id,
                    message.philosophy_school.value,
                    message.message_type.value,
                    message.dimension.value,
                    message.content,
                    message.responds_to,
                    message.timestamp.isoformat()
                ))
            
            self.conn.commit()
            return True
        except Exception as e:
            print(f"Error saving debate: {e}")
            return False
    
    def load_debate(self, debate_id: str) -> Optional[DebateSession]:
        """Load a debate session from the database.
        
        Args:
            debate_id: ID of the debate to load
            
        Returns:
            DebateSession if found, None otherwise
        """
        try:
            cursor = self.conn.cursor()
            cursor.execute("SELECT data FROM debates WHERE id = ?", (debate_id,))
            row = cursor.fetchone()
            
            if row:
                data = json.loads(row[0])
                return DebateSession.from_dict(data)
            return None
        except Exception as e:
            print(f"Error loading debate: {e}")
            return None
    
    def list_debates(self, limit: int = 10) -> List[Dict[str, Any]]:
        """List recent debates.
        
        Args:
            limit: Maximum number of debates to return
            
        Returns:
            List of debate summaries
        """
        try:
            cursor = self.conn.cursor()
            cursor.execute("""
                SELECT id, topic, ethical_question, created_at, completed_at
                FROM debates
                ORDER BY created_at DESC
                LIMIT ?
            """, (limit,))
            
            debates = []
            for row in cursor.fetchall():
                debates.append({
                    'id': row[0],
                    'topic': row[1],
                    'ethical_question': row[2],
                    'created_at': row[3],
                    'completed_at': row[4]
                })
            return debates
        except Exception as e:
            print(f"Error listing debates: {e}")
            return []
    
    def close(self):
        """Close database connection."""
        if self.conn:
            self.conn.close()


# ============================================================================
# Ollama LLM Backend
# ============================================================================

class OllamaBackend:
    """Interface to Ollama for local LLM inference."""
    
    def __init__(self, model: str = "llama3.2", base_url: str = "http://localhost:11434"):
        """Initialize Ollama backend.
        
        Args:
            model: Name of the Ollama model to use
            base_url: Base URL of the Ollama server
        """
        self.model = model
        self.base_url = base_url
    
    def check_availability(self) -> bool:
        """Check if Ollama is available.
        
        Returns:
            True if Ollama is running and accessible
        """
        try:
            response = requests.get(f"{self.base_url}/api/tags", timeout=5)
            return response.status_code == 200
        except (requests.RequestException, requests.Timeout):
            return False
    
    def generate(self, prompt: str, system_prompt: Optional[str] = None, 
                 temperature: float = 0.7, max_tokens: int = 800) -> str:
        """Generate text using Ollama.
        
        Args:
            prompt: The user prompt
            system_prompt: Optional system prompt for persona
            temperature: Sampling temperature (0-1)
            max_tokens: Maximum tokens to generate
            
        Returns:
            Generated text
        """
        try:
            payload = {
                "model": self.model,
                "prompt": prompt,
                "stream": False,
                "options": {
                    "temperature": temperature,
                    "num_predict": max_tokens
                }
            }
            
            if system_prompt:
                payload["system"] = system_prompt
            
            response = requests.post(
                f"{self.base_url}/api/generate",
                json=payload,
                timeout=120
            )
            response.raise_for_status()
            
            data = response.json()
            return data.get("response", "").strip()
        
        except Exception as e:
            print(f"Error generating with Ollama: {e}")
            return f"[Error: Could not generate response - {str(e)}]"


# ============================================================================
# Moral Dialectic Engine
# ============================================================================

class MoralDialecticEngine:
    """Engine for orchestrating philosophical debates."""
    
    def __init__(self, llm_backend: OllamaBackend, db_store: SQLiteDebateStore):
        """Initialize the dialectic engine.
        
        Args:
            llm_backend: LLM backend for generating arguments
            db_store: Database store for persistence
        """
        self.llm_backend = llm_backend
        self.db_store = db_store
    
    def start_debate(
        self,
        topic: str,
        ethical_question: str,
        philosophies: List[PhilosophySchool],
        dimensions: List[ArgumentDimension]
    ) -> DebateSession:
        """Start a new philosophical debate.
        
        Args:
            topic: The topic of debate
            ethical_question: The specific ethical question
            philosophies: List of philosophy schools to participate
            dimensions: List of dimensions to analyze
            
        Returns:
            Completed DebateSession
        """
        # Create new session
        session = DebateSession(
            id=f"debate_{uuid.uuid4().hex[:12]}",
            topic=topic,
            ethical_question=ethical_question
        )
        
        print(f"\n{'='*70}")
        print(f"MORAL PHILOSOPHY DEBATE")
        print(f"{'='*70}")
        print(f"Topic: {topic}")
        print(f"Question: {ethical_question}")
        print(f"{'='*70}\n")
        
        # Round 1: Initial statements
        print("ROUND 1: Initial Statements")
        print("-" * 70)
        
        for philosophy in philosophies:
            for dimension in dimensions:
                print(f"\n[{PHILOSOPHY_PROFILES[philosophy]['name']} - {dimension.value}]")
                message = self._generate_statement(philosophy, dimension, ethical_question, session)
                session.messages.append(message)
                print(f"{message.content}\n")
        
        # Save after round 1
        self.db_store.save_debate(session)
        
        # Round 2: Counter-arguments
        print("\n" + "="*70)
        print("ROUND 2: Counter-Arguments")
        print("-" * 70)
        
        statements = [msg for msg in session.messages if msg.message_type == MessageType.STATEMENT]
        
        for philosophy in philosophies:
            # Pick a random opposing statement to counter
            opposing_statements = [s for s in statements if s.philosophy_school != philosophy]
            if opposing_statements:
                target = random.choice(opposing_statements)
                print(f"\n[{PHILOSOPHY_PROFILES[philosophy]['name']} responds to "
                      f"{PHILOSOPHY_PROFILES[target.philosophy_school]['name']}]")
                counter = self._generate_counter(philosophy, target, ethical_question, session)
                session.messages.append(counter)
                print(f"{counter.content}\n")
        
        # Mark as completed
        session.completed_at = datetime.now()
        self.db_store.save_debate(session)
        
        print("="*70)
        print(f"Debate completed and saved with ID: {session.id}")
        print("="*70 + "\n")
        
        return session
    
    def _generate_statement(
        self,
        philosophy: PhilosophySchool,
        dimension: ArgumentDimension,
        ethical_question: str,
        session: DebateSession
    ) -> ChatMessage:
        """Generate an initial philosophical statement.
        
        Args:
            philosophy: The philosophy school
            dimension: The dimension to analyze
            ethical_question: The ethical question
            session: Current debate session
            
        Returns:
            ChatMessage with the statement
        """
        profile = PHILOSOPHY_PROFILES[philosophy]
        
        prompt = f"""Ethical Question: {ethical_question}

Analyze this from a {dimension.value} perspective. 
Provide a clear, concise position statement (maximum 150 words).
Focus on your core philosophical principles."""
        
        content = self.llm_backend.generate(
            prompt=prompt,
            system_prompt=profile["system_prompt"]
        )
        
        return ChatMessage(
            id=f"msg_{uuid.uuid4().hex[:16]}",
            philosophy_school=philosophy,
            message_type=MessageType.STATEMENT,
            dimension=dimension,
            content=content
        )
    
    def _generate_counter(
        self,
        philosophy: PhilosophySchool,
        target_message: ChatMessage,
        ethical_question: str,
        session: DebateSession
    ) -> ChatMessage:
        """Generate a counter-argument to another philosopher's statement.
        
        Args:
            philosophy: The philosophy school responding
            target_message: The message being responded to
            ethical_question: The ethical question
            session: Current debate session
            
        Returns:
            ChatMessage with the counter-argument
        """
        profile = PHILOSOPHY_PROFILES[philosophy]
        target_profile = PHILOSOPHY_PROFILES[target_message.philosophy_school]
        
        prompt = f"""Ethical Question: {ethical_question}

{target_profile['name']} argued:
"{target_message.content}"

Provide a respectful counter-argument from your philosophical perspective.
Point out potential weaknesses or alternative viewpoints (maximum 150 words)."""
        
        content = self.llm_backend.generate(
            prompt=prompt,
            system_prompt=profile["system_prompt"]
        )
        
        return ChatMessage(
            id=f"msg_{uuid.uuid4().hex[:16]}",
            philosophy_school=philosophy,
            message_type=MessageType.COUNTER,
            dimension=target_message.dimension,
            content=content,
            responds_to=target_message.id
        )
    
    def export_debate_markdown(self, session: DebateSession) -> str:
        """Export debate session as markdown.
        
        Args:
            session: The debate session to export
            
        Returns:
            Markdown formatted string
        """
        md = []
        md.append("# Moral Philosophy Debate\n")
        md.append(f"**Topic:** {session.topic}\n")
        md.append(f"**Question:** {session.ethical_question}\n")
        md.append(f"**Date:** {session.created_at.strftime('%Y-%m-%d %H:%M:%S')}\n")
        md.append(f"**Debate ID:** {session.id}\n")
        md.append("\n---\n")
        
        # Group messages by type
        statements = [m for m in session.messages if m.message_type == MessageType.STATEMENT]
        counters = [m for m in session.messages if m.message_type == MessageType.COUNTER]
        
        md.append("\n## Round 1: Initial Statements\n")
        for msg in statements:
            profile = PHILOSOPHY_PROFILES[msg.philosophy_school]
            md.append(f"\n### {profile['name']} ({msg.dimension.value})\n")
            md.append(f"{msg.content}\n")
        
        if counters:
            md.append("\n## Round 2: Counter-Arguments\n")
            for msg in counters:
                profile = PHILOSOPHY_PROFILES[msg.philosophy_school]
                md.append(f"\n### {profile['name']} (Counter-Argument)\n")
                md.append(f"{msg.content}\n")
        
        md.append("\n---\n")
        md.append(f"\n*Generated with Standalone Moral Dialectic Engine*\n")
        
        return "".join(md)


# ============================================================================
# Main Demo
# ============================================================================

def main():
    """Main demonstration function."""
    parser = argparse.ArgumentParser(
        description="Standalone Moral Philosophy Dialectic Engine"
    )
    parser.add_argument(
        "--topic",
        default="Künstliche Intelligenz in der Medizin",
        help="Topic for philosophical debate"
    )
    parser.add_argument(
        "--question",
        default="Sollte eine KI über Leben und Tod entscheiden dürfen?",
        help="Specific ethical question"
    )
    parser.add_argument(
        "--model",
        default="llama3.2",
        help="Ollama model to use"
    )
    parser.add_argument(
        "--db",
        default="moral_debates.db",
        help="SQLite database path"
    )
    parser.add_argument(
        "--export",
        help="Export debate to markdown file"
    )
    
    args = parser.parse_args()
    
    print("Standalone Moral Philosophy Dialectic Engine")
    print("=" * 70)
    
    # Initialize components
    print("\n1. Initializing SQLite database...")
    db_store = SQLiteDebateStore(args.db)
    print(f"   ✓ Database ready at: {args.db}")
    
    print("\n2. Connecting to Ollama...")
    llm_backend = OllamaBackend(model=args.model)
    
    if not llm_backend.check_availability():
        print("   ✗ Error: Ollama not available!")
        print("   Please ensure Ollama is running:")
        print("     - Install: https://ollama.ai")
        print("     - Run: ollama serve")
        print(f"     - Pull model: ollama pull {args.model}")
        db_store.close()
        return 1
    
    print(f"   ✓ Ollama connected (model: {args.model})")
    
    print("\n3. Initializing Moral Dialectic Engine...")
    engine = MoralDialecticEngine(llm_backend, db_store)
    print("   ✓ Engine ready")
    
    # Start debate
    print("\n4. Starting philosophical debate...\n")
    
    philosophies = [
        PhilosophySchool.KANT,
        PhilosophySchool.UTILITARIANISM,
        PhilosophySchool.VIRTUE_ETHICS,
        PhilosophySchool.SOCRATIC,
        PhilosophySchool.STOICISM
    ]
    
    dimensions = [
        ArgumentDimension.MORAL,
        ArgumentDimension.ETHICAL
    ]
    
    session = engine.start_debate(
        topic=args.topic,
        ethical_question=args.question,
        philosophies=philosophies,
        dimensions=dimensions
    )
    
    # Export if requested
    if args.export:
        markdown = engine.export_debate_markdown(session)
        with open(args.export, 'w', encoding='utf-8') as f:
            f.write(markdown)
        print(f"\n✓ Debate exported to: {args.export}")
    
    # List recent debates
    print("\n5. Recent debates in database:")
    debates = db_store.list_debates(limit=5)
    for i, debate in enumerate(debates, 1):
        print(f"   {i}. [{debate['id']}] {debate['topic']}")
        print(f"      Question: {debate['ethical_question']}")
        print(f"      Created: {debate['created_at']}")
    
    # Cleanup
    db_store.close()
    print("\n✓ All done!\n")
    
    return 0


if __name__ == "__main__":
    exit(main())
