"""
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            debate_chat.py                                     ║
  Version:         0.0.46                                             ║
  Last Modified:   2026-04-15 18:01:27                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     632                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 2a1fb04231  2026-03-03  Merge branch 'develop' into copilot/audit-src-module-docu... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
"""

"""
Debate Chat System

Manages the chat-like debate flow with statement, counter-arguments,
rebuttals across different dimensions (moral, social, political, ethical).
Philosophers speak in first person and respond to each other randomly.
"""

from typing import List, Optional, Dict
from datetime import datetime
import random
from models import (
    PhilosophySchool, ChatMessage, DebateSession, NewsArticle,
    MessageType, ArgumentDimension, PHILOSOPHY_PROFILES
)

# Optional imports for AI Synthesizer and Knowledge Researcher
try:
    from ai_synthesizer import AISynthesizer
    AI_SYNTHESIZER_AVAILABLE = True
except ImportError:
    AI_SYNTHESIZER_AVAILABLE = False

try:
    from knowledge_researcher import KnowledgeResearcher
    KNOWLEDGE_RESEARCHER_AVAILABLE = True
except ImportError:
    KNOWLEDGE_RESEARCHER_AVAILABLE = False


class DebateChatManager:
    """
    Manages the chat-style debate between philosophical perspectives.
    
    The debate flows as:
    1. Initial statements from each philosophy on each dimension
    2. Counter-arguments responding to other philosophies
    3. Rebuttals defending against counters
    4. Synthesis attempts to find common ground
    """
    
    def __init__(
        self,
        llm_backend: Optional['LLMBackend'] = None,
        enable_ai_synthesis: bool = True,
        enable_knowledge_research: bool = True,
        themis_client: Optional['MoralDebateClient'] = None
    ):
        """
        Initialize the chat manager.
        
        Args:
            llm_backend: Optional LLM backend for generating messages
            enable_ai_synthesis: Enable AI synthesizer participant
            enable_knowledge_research: Enable knowledge research for LLM context
            themis_client: Optional ThemisDB client for multi-model storage
        """
        self.llm_backend = llm_backend
        self.philosophy_profiles = PHILOSOPHY_PROFILES
        self.themis_client = themis_client
        
        # Initialize AI Synthesizer (KI participant)
        self.ai_synthesizer = None
        if enable_ai_synthesis and AI_SYNTHESIZER_AVAILABLE:
            try:
                self.ai_synthesizer = AISynthesizer()
            except Exception as e:
                print(f"AI Synthesizer initialization failed: {e}")
        
        # Initialize Knowledge Researcher
        self.knowledge_researcher = None
        if enable_knowledge_research and KNOWLEDGE_RESEARCHER_AVAILABLE:
            try:
                self.knowledge_researcher = KnowledgeResearcher()
            except Exception as e:
                print(f"Knowledge Researcher initialization failed: {e}")
    
    def start_debate_chat(
        self,
        session: DebateSession,
        dimensions: List[ArgumentDimension],
        philosophies: List[PhilosophySchool]
    ) -> DebateSession:
        """
        Starts a new debate chat session.
        
        Args:
            session: The debate session
            dimensions: Dimensions to explore (moral, social, political, etc.)
            philosophies: Philosophical schools to include
        
        Returns:
            Updated session with initial statements
        """
        session.active_dimensions = dimensions
        session.current_round = 1
        
        # Store debate start in timeline
        if self.themis_client:
            try:
                self.themis_client.add_timeline_event(
                    debate_id=session.id,
                    event_type="debate_started",
                    timestamp=datetime.now(),
                    data={
                        "dimensions": [d.value for d in dimensions],
                        "philosophies": [p.value for p in philosophies],
                        "topic": session.debate_topic
                    }
                )
            except Exception as e:
                print(f"Failed to log timeline event: {e}")
        
        # Research knowledge context if available
        if self.knowledge_researcher and session.news_article:
            try:
                topic = session.debate_topic or session.news_article.title
                keywords = session.news_article.ethical_topics
                knowledge_context = self.knowledge_researcher.research_topic(
                    topic=topic,
                    keywords=keywords,
                    depth="moderate"
                )
                # Store in session metadata for LLM use
                session.metadata = session.metadata or {}
                session.metadata['knowledge_context'] = knowledge_context.to_llm_prompt_context()
            except Exception as e:
                print(f"Knowledge research failed: {e}")
        
        # Phase 1: Generate initial statements for each dimension
        for dimension in dimensions:
            for philosophy in philosophies:
                message = self._generate_statement(
                    philosophy,
                    dimension,
                    session.news_article,
                    session.ethical_question
                )
                session.chat_messages.append(message)
        
        # Add AI Synthesizer initial statement
        if self.ai_synthesizer:
            try:
                ai_message = self.ai_synthesizer.analyze_debate_round(session, 1)
                session.chat_messages.append(ai_message)
            except Exception as e:
                print(f"AI Synthesizer failed: {e}")
        
        return session
    
    def advance_debate_round(
        self,
        session: DebateSession
    ) -> DebateSession:
        """
        Advances the debate to the next round with counter-arguments.
        
        Args:
            session: The debate session
        
        Returns:
            Updated session with new messages
        
        Raises:
            ValueError: If time limit exceeded
        """
        # Check time limit before advancing
        if session.is_time_limit_exceeded():
            elapsed = session.get_elapsed_time_minutes()
            raise ValueError(
                f"Debatte-Zeitlimit überschritten: {elapsed:.1f} Minuten von "
                f"{session.max_duration_minutes} Minuten. Debatte wird automatisch beendet."
            )
        
        session.current_round += 1
        
        if session.current_round == 2:
            # Phase 2: Generate counter-arguments
            session = self._generate_counter_arguments(session)
        elif session.current_round == 3:
            # Phase 3: Generate rebuttals
            session = self._generate_rebuttals(session)
        elif session.current_round == 4:
            # Phase 4: Synthesis attempts
            session = self._generate_synthesis(session)
        
        # Add AI Synthesizer analysis after each round
        if self.ai_synthesizer and session.current_round <= 4:
            try:
                ai_message = self.ai_synthesizer.analyze_debate_round(
                    session,
                    session.current_round
                )
                session.chat_messages.append(ai_message)
            except Exception as e:
                print(f"AI Synthesizer failed in round {session.current_round}: {e}")
        
        # Check time limit after round completion
        if session.is_time_limit_exceeded():
            session.completed_at = datetime.now()
            print(f"Debatte nach Runde {session.current_round} wegen Zeitlimit beendet")
        
        return session
    
    def _generate_statement(
        self,
        philosophy: PhilosophySchool,
        dimension: ArgumentDimension,
        news_article: NewsArticle,
        ethical_question: str
    ) -> ChatMessage:
        """
        Generates an initial statement from a philosophical perspective.
        
        Args:
            philosophy: The philosophical school
            dimension: The dimension to address
            news_article: The news article
            ethical_question: The ethical question
        
        Returns:
            ChatMessage with the statement
        """
        message = ChatMessage()
        message.philosophy_school = philosophy
        message.message_type = MessageType.STATEMENT
        message.dimension = dimension
        
        if self.llm_backend:
            content = self.llm_backend.generate_statement(
                philosophy,
                dimension,
                news_article,
                ethical_question
            )
        else:
            content = self._template_statement(
                philosophy,
                dimension,
                news_article
            )
        
        message.content = content
        return message
    
    def _generate_counter_arguments(self, session: DebateSession) -> DebateSession:
        """
        Generates counter-arguments responding to initial statements.
        Randomly selects which philosopher responds to which.
        
        Args:
            session: The debate session
        
        Returns:
            Updated session
        """
        # Get all statements grouped by dimension
        statements_by_dim: Dict[ArgumentDimension, List[ChatMessage]] = {}
        for msg in session.chat_messages:
            if msg.message_type == MessageType.STATEMENT:
                if msg.dimension not in statements_by_dim:
                    statements_by_dim[msg.dimension] = []
                statements_by_dim[msg.dimension].append(msg)
        
        # Generate counters with random pairings
        for dimension, statements in statements_by_dim.items():
            # Create random pairings
            philosophers = [stmt.philosophy_school for stmt in statements]
            shuffled = philosophers.copy()
            random.shuffle(shuffled)
            
            # Ensure no one responds to themselves
            for i in range(len(philosophers)):
                if shuffled[i] == philosophers[i]:
                    # Swap with next if same
                    next_idx = (i + 1) % len(shuffled)
                    shuffled[i], shuffled[next_idx] = shuffled[next_idx], shuffled[i]
            
            # Generate counters based on random pairings
            for i, responding_phil in enumerate(shuffled):
                # Find the target statement
                target_phil = philosophers[i]
                target_statement = next(s for s in statements if s.philosophy_school == target_phil)
                
                counter = self._generate_counter(
                    responding_phil,
                    target_statement,
                    session
                )
                session.chat_messages.append(counter)
        
        return session
    
    def _generate_counter(
        self,
        philosophy: PhilosophySchool,
        target_message: ChatMessage,
        session: DebateSession
    ) -> ChatMessage:
        """
        Generates a counter-argument to a statement.
        
        Args:
            philosophy: The philosophy making the counter
            target_message: The message being countered
            session: The debate session
        
        Returns:
            ChatMessage with counter-argument
        """
        message = ChatMessage()
        message.philosophy_school = philosophy
        message.message_type = MessageType.COUNTER
        message.dimension = target_message.dimension
        message.responds_to = target_message.id
        
        if self.llm_backend:
            content = self.llm_backend.generate_counter(
                philosophy,
                target_message,
                session
            )
        else:
            content = self._template_counter(
                philosophy,
                target_message
            )
        
        message.content = content
        return message
    
    def _generate_rebuttals(self, session: DebateSession) -> DebateSession:
        """
        Generates rebuttals defending against counter-arguments.
        Randomly decides who rebuts (not always the original speaker).
        
        Args:
            session: The debate session
        
        Returns:
            Updated session
        """
        # Find all counter-arguments
        counters = [msg for msg in session.chat_messages 
                   if msg.message_type == MessageType.COUNTER]
        
        # Get all participating philosophers
        all_philosophies = list(set(msg.philosophy_school for msg in session.chat_messages))
        
        for counter in counters:
            # Find the original statement
            original = next(
                (msg for msg in session.chat_messages 
                 if msg.id == counter.responds_to),
                None
            )
            
            if original:
                # 70% chance original speaker rebuts, 30% chance random other philosopher jumps in
                if random.random() < 0.7:
                    rebutter = original.philosophy_school
                else:
                    # Random other philosopher supports the original position
                    other_phils = [p for p in all_philosophies 
                                  if p != counter.philosophy_school and p != original.philosophy_school]
                    if other_phils:
                        rebutter = random.choice(other_phils)
                    else:
                        rebutter = original.philosophy_school
                
                rebuttal = self._generate_rebuttal(
                    rebutter,
                    counter,
                    session
                )
                session.chat_messages.append(rebuttal)
        
        return session
    
    def _generate_rebuttal(
        self,
        philosophy: PhilosophySchool,
        counter_message: ChatMessage,
        session: DebateSession
    ) -> ChatMessage:
        """
        Generates a rebuttal to a counter-argument.
        
        Args:
            philosophy: The philosophy making the rebuttal
            counter_message: The counter being rebutted
            session: The debate session
        
        Returns:
            ChatMessage with rebuttal
        """
        message = ChatMessage()
        message.philosophy_school = philosophy
        message.message_type = MessageType.REBUTTAL
        message.dimension = counter_message.dimension
        message.responds_to = counter_message.id
        
        if self.llm_backend:
            content = self.llm_backend.generate_rebuttal(
                philosophy,
                counter_message,
                session
            )
        else:
            content = self._template_rebuttal(
                philosophy,
                counter_message
            )
        
        message.content = content
        return message
    
    def _generate_synthesis(self, session: DebateSession) -> DebateSession:
        """
        Generates synthesis messages attempting to find common ground.
        
        Args:
            session: The debate session
        
        Returns:
            Updated session
        """
        # Each philosophy attempts synthesis on each dimension
        for dimension in session.active_dimensions:
            for philosophy in set(msg.philosophy_school 
                                for msg in session.chat_messages[:3]):
                synthesis = self._generate_synthesis_message(
                    philosophy,
                    dimension,
                    session
                )
                session.chat_messages.append(synthesis)
        
        return session
    
    def _generate_synthesis_message(
        self,
        philosophy: PhilosophySchool,
        dimension: ArgumentDimension,
        session: DebateSession
    ) -> ChatMessage:
        """
        Generates a synthesis message.
        
        Args:
            philosophy: The philosophy making the synthesis
            dimension: The dimension
            session: The debate session
        
        Returns:
            ChatMessage with synthesis
        """
        message = ChatMessage()
        message.philosophy_school = philosophy
        message.message_type = MessageType.SYNTHESIS
        message.dimension = dimension
        
        if self.llm_backend:
            content = self.llm_backend.generate_synthesis(
                philosophy,
                dimension,
                session
            )
        else:
            content = self._template_synthesis(
                philosophy,
                dimension
            )
        
        message.content = content
        return message
    
    # Template-based generation methods (fallback when no LLM)
    
    def _template_statement(
        self,
        philosophy: PhilosophySchool,
        dimension: ArgumentDimension,
        news_article: NewsArticle
    ) -> str:
        """Template-based statement generation in first person."""
        profile = self.philosophy_profiles[philosophy]
        
        dimension_focus = {
            # Practical Philosophy
            ArgumentDimension.MORAL: "moralischen Bewertung",
            ArgumentDimension.SOCIAL: "sozialen Auswirkungen",
            ArgumentDimension.POLITICAL: "politischen Implikationen",
            ArgumentDimension.ETHICAL: "ethischen Prinzipien",
            ArgumentDimension.ECONOMIC: "wirtschaftlichen Folgen",
            ArgumentDimension.LEGAL: "rechtlichen Aspekten",
            # Theoretical Philosophy
            ArgumentDimension.EPISTEMOLOGICAL: "erkenntnistheoretischen Analyse",
            ArgumentDimension.METAPHYSICAL: "metaphysischen Betrachtung",
            ArgumentDimension.LOGICAL: "logischen Untersuchung",
            ArgumentDimension.ONTOLOGICAL: "ontologischen Fragestellung",
            ArgumentDimension.PHENOMENOLOGICAL: "phänomenologischen Perspektive",
            # Meta-Ethics
            ArgumentDimension.METAETHICAL: "metaethischen Analyse",
            ArgumentDimension.NORMATIVE: "normativen Betrachtung",
            ArgumentDimension.APPLIED: "angewandten Ethik",
            ArgumentDimension.DESCRIPTIVE: "deskriptiven Untersuchung"
        }
        
        return f"""Zur {dimension_focus.get(dimension, 'Analyse')} von "{news_article.title}":

Meiner Ansicht nach ist dies fundamentally eine Frage von {profile.core_principles[0].lower()}. Ich argumentiere, dass wir dies durch mein philosophisches Rahmenwerk betrachten müssen.

{profile.decision_framework}

Ich sehe die zentrale Herausforderung darin, diese Prinzipien auf die konkreten Umstände anzuwenden, ohne ihre Gültigkeit zu kompromittieren."""
    
    def _template_counter(
        self,
        philosophy: PhilosophySchool,
        target_message: ChatMessage
    ) -> str:
        """Template-based counter-argument generation in first person."""
        profile = self.philosophy_profiles[philosophy]
        target_profile = self.philosophy_profiles[target_message.philosophy_school]
        
        responses = [
            f"""Ich muss {target_profile.philosopher_name} hier widersprechen. Ihre Perspektive übersieht einen entscheidenden Punkt:

Während Sie betonen, dass {target_profile.core_principles[0].lower()}, argumentiere ich, dass wir nicht vergessen dürfen: {profile.core_principles[0]}

Aus meiner Sicht müssen wir {profile.decision_framework.lower()}""",
            
            f"""Mit Verlaub, {target_profile.philosopher_name}, ich sehe das anders. Ihr Ansatz hat Schwächen:

Sie konzentrieren sich auf {target_profile.core_principles[0].lower()}, aber ich gebe zu bedenken, dass {profile.core_principles[0].lower()} mindestens ebenso wichtig ist.

Mein Ansatz würde verlangen, dass wir {profile.decision_framework.lower()}""",
            
            f"""Ich schätze Ihren Beitrag, {target_profile.philosopher_name}, aber ich muss eine andere Position vertreten:

Ihr Fokus auf {target_profile.core_principles[0].lower()} greift meiner Meinung nach zu kurz. Ich halte es für zentral, dass {profile.core_principles[0].lower()}.

Nach meinem Verständnis sollten wir {profile.decision_framework.lower()}"""
        ]
        
        return random.choice(responses)
    
    def _template_rebuttal(
        self,
        philosophy: PhilosophySchool,
        counter_message: ChatMessage
    ) -> str:
        """Template-based rebuttal generation in first person."""
        profile = self.philosophy_profiles[philosophy]
        counter_profile = self.philosophy_profiles[counter_message.philosophy_school]
        
        rebuttals = [
            f"""Ich muss meine Position verteidigen, {counter_profile.philosopher_name}:

Ihr Einwand übersieht, dass {profile.core_principles[0].lower()} fundamentaler ist, als Sie annehmen. Ich vertrete die Auffassung, dass {profile.decision_framework.lower()}

Diese Prinzipien sind für mich nicht verhandelbar, da sie die Grundlage jeder rationalen moralischen Urteilsfähigkeit bilden.""",
            
            f"""Ich danke Ihnen für die Herausforderung, {counter_profile.philosopher_name}, aber ich bleibe bei meiner Analyse:

Sie unterschätzen die Bedeutung von {profile.core_principles[0].lower()}. Meiner Überzeugung nach müssen wir {profile.decision_framework.lower()}

Ich sehe keinen Grund, von dieser Position abzuweichen, da sie auf soliden philosophischen Grundlagen ruht.""",
            
            f"""Ich höre Ihren Einwand, {counter_profile.philosopher_name}, aber ich kann ihm nicht folgen:

Was Sie übersehen, ist dass {profile.core_principles[0].lower()} nicht einfach eine Option unter vielen ist. Nach meinem Verständnis ist es zwingend, dass wir {profile.decision_framework.lower()}

Ich halte an meiner ursprünglichen Position fest."""
        ]
        
        return random.choice(rebuttals)
    
    def _template_synthesis(
        self,
        philosophy: PhilosophySchool,
        dimension: ArgumentDimension
    ) -> str:
        """Template-based synthesis generation in first person."""
        profile = self.philosophy_profiles[philosophy]
        
        syntheses = [
            f"""Nach dieser intensiven Diskussion möchte ich einen Versuch der Synthese wagen:

Ich habe allen Kollegen aufmerksam zugehört und erkenne durchaus Gemeinsamkeiten. Wir alle stimmen überein, dass diese Situation sorgfältiger ethischer Überlegung bedarf.

Trotz unserer unterschiedlichen Ausgangspunkte sehe ich, dass wir uns darauf einigen können, dass {profile.core_principles[0].lower()} ein wichtiger - wenn auch vielleicht nicht der einzige - Orientierungspunkt ist.

Ich schlage vor, dass wir einen Weg finden, der mehrere unserer Perspektiven berücksichtigt.""",
            
            f"""Lassen Sie mich versuchen, die verschiedenen Stränge zusammenzuführen:

Ich beobachte, dass wir trotz unterschiedlicher Herangehensweisen gemeinsame Anliegen teilen. Jeder von uns betont verschiedene Aspekte, aber ich glaube, dass {profile.core_principles[0].lower()} als verbindendes Element dienen könnte.

Meiner Ansicht nach sollten wir einen integrativen Ansatz verfolgen, der die Stärken aller Positionen nutzt.""",
            
            f"""Nach Anhörung aller Argumente möchte ich eine Brücke schlagen:

Ich erkenne an, dass jede vorgetragene Position ihre Berechtigung hat. Obwohl ich nach wie vor überzeugt bin, dass mein Ansatz zentral ist, sehe ich auch Wert in den anderen Perspektiven.

Vielleicht können wir uns darauf einigen, dass {profile.core_principles[0].lower()}, kombiniert mit den Einsichten meiner Kollegen, zu einer umfassenderen Lösung führen würde."""
        ]
        
        return random.choice(syntheses)
