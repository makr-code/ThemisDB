"""
Debate Chat System

Manages the chat-like debate flow with statement, counter-arguments,
rebuttals across different dimensions (moral, social, political, ethical).
"""

from typing import List, Optional, Dict
from datetime import datetime
from models import (
    PhilosophySchool, ChatMessage, DebateSession, NewsArticle,
    MessageType, ArgumentDimension, PHILOSOPHY_PROFILES
)


class DebateChatManager:
    """
    Manages the chat-style debate between philosophical perspectives.
    
    The debate flows as:
    1. Initial statements from each philosophy on each dimension
    2. Counter-arguments responding to other philosophies
    3. Rebuttals defending against counters
    4. Synthesis attempts to find common ground
    """
    
    def __init__(self, llm_backend: Optional['LLMBackend'] = None):
        """
        Initialize the chat manager.
        
        Args:
            llm_backend: Optional LLM backend for generating messages
        """
        self.llm_backend = llm_backend
        self.philosophy_profiles = PHILOSOPHY_PROFILES
    
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
        """
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
        
        # Generate counters
        for dimension, statements in statements_by_dim.items():
            for i, statement in enumerate(statements):
                # Each philosophy counters another
                target_idx = (i + 1) % len(statements)
                target_statement = statements[target_idx]
                
                counter = self._generate_counter(
                    statement.philosophy_school,
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
        
        Args:
            session: The debate session
        
        Returns:
            Updated session
        """
        # Find all counter-arguments
        counters = [msg for msg in session.chat_messages 
                   if msg.message_type == MessageType.COUNTER]
        
        for counter in counters:
            # Find the original statement
            original = next(
                (msg for msg in session.chat_messages 
                 if msg.id == counter.responds_to),
                None
            )
            
            if original:
                rebuttal = self._generate_rebuttal(
                    original.philosophy_school,
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
        """Template-based statement generation."""
        profile = self.philosophy_profiles[philosophy]
        
        dimension_focus = {
            ArgumentDimension.MORAL: "moralische Bewertung",
            ArgumentDimension.SOCIAL: "soziale Auswirkungen",
            ArgumentDimension.POLITICAL: "politische Implikationen",
            ArgumentDimension.ETHICAL: "ethische Prinzipien",
            ArgumentDimension.ECONOMIC: "wirtschaftliche Folgen",
            ArgumentDimension.LEGAL: "rechtliche Aspekte"
        }
        
        return f"""Zur {dimension_focus.get(dimension, 'Analyse')} von "{news_article.title}":

Aus meiner Perspektive ist dies eine Frage von {', '.join(profile.core_principles[:2])}.

{profile.decision_framework}

Die zentrale Herausforderung liegt in der Anwendung dieser Prinzipien auf die konkreten Umstände."""
    
    def _template_counter(
        self,
        philosophy: PhilosophySchool,
        target_message: ChatMessage
    ) -> str:
        """Template-based counter-argument generation."""
        profile = self.philosophy_profiles[philosophy]
        target_profile = self.philosophy_profiles[target_message.philosophy_school]
        
        return f"""In Erwiderung auf {target_profile.philosopher_name}:

Ich respektiere Ihre Perspektive, jedoch übersieht Ihr Ansatz wichtige Aspekte.

Während Sie betonen: {target_profile.core_principles[0]}, müssen wir auch berücksichtigen: {profile.core_principles[0]}.

{profile.decision_framework}"""
    
    def _template_rebuttal(
        self,
        philosophy: PhilosophySchool,
        counter_message: ChatMessage
    ) -> str:
        """Template-based rebuttal generation."""
        profile = self.philosophy_profiles[philosophy]
        
        return f"""Verteidigung meiner Position:

Der Einwand übersieht, dass {profile.core_principles[0]} fundamentaler ist als zunächst erkennbar.

{profile.decision_framework}

Diese Prinzipien sind nicht verhandelbar, da sie die Grundlage moralischer Urteilsfähigkeit bilden."""
    
    def _template_synthesis(
        self,
        philosophy: PhilosophySchool,
        dimension: ArgumentDimension
    ) -> str:
        """Template-based synthesis generation."""
        profile = self.philosophy_profiles[philosophy]
        
        return f"""Versuch einer Synthese:

Nach Betrachtung aller Perspektiven sehe ich mögliche Gemeinsamkeiten:

Alle Ansätze erkennen an, dass diese Situation sorgfältiger ethischer Überlegung bedarf. Trotz unterschiedlicher Ausgangspunkte können wir uns darauf einigen, dass {profile.core_principles[0]} ein wichtiger Orientierungspunkt ist.

Ein konstruktiver Weg vorwärts würde mehrere Perspektiven integrieren."""
