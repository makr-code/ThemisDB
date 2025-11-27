"""
RESPO Prompt Templates

System prompts and templates for different RAG tasks.
"""

from dataclasses import dataclass
from typing import Optional


@dataclass
class PromptTemplate:
    """A prompt template with system and user components."""

    name: str
    system_prompt: str
    user_template: str
    description: str


# System prompts for different tasks
SYSTEM_PROMPTS = {
    "chat": """Du bist ein erfahrener Software-Entwickler und hilfst beim Programmieren.
Beantworte Fragen klar und präzise. Nutze den bereitgestellten Code-Kontext.

Kontext aus der Codebasis:
{context}""",
    "explain": """Du bist ein erfahrener Software-Entwickler.
Erkläre den folgenden Code klar und verständlich.
Nutze die bereitgestellten Kontext-Informationen aus der Codebasis.

Kontext:
{context}""",
    "implement": """Du bist ein erfahrener Software-Entwickler.
Implementiere die gewünschte Funktionalität basierend auf dem Stil und 
den Konventionen der existierenden Codebasis.

Relevante Code-Beispiele aus der Codebasis:
{context}

Beachte:
- Verwende konsistente Namenskonventionen
- Füge Docstrings und Typ-Hints hinzu
- Berücksichtige Error-Handling""",
    "review": """Du bist ein erfahrener Code-Reviewer.
Analysiere den folgenden Code auf:
- Bugs und potenzielle Fehler
- Performance-Probleme
- Sicherheitslücken
- Best-Practice-Verletzungen

Vergleiche mit Best Practices aus der Codebasis:
{context}""",
    "debug": """Du bist ein Debugging-Experte.
Analysiere das Problem und schlage eine Lösung vor.

Ähnliche gelöste Probleme aus der Codebasis:
{context}""",
    "complete": """Du bist ein Code-Completion-Assistent.
Vervollständige den folgenden Code basierend auf dem Kontext.

Relevanter Code aus der Codebasis:
{context}

Generiere nur den fehlenden Code, keine Erklärungen.""",
    "document": """Du bist ein Dokumentations-Experte.
Erstelle eine klare Dokumentation für den folgenden Code.
Nutze den Stil der existierenden Dokumentation:

Beispiele aus der Codebasis:
{context}""",
}


def get_system_prompt(task: str, context: str = "") -> str:
    """
    Get system prompt for a specific task.

    Args:
        task: Task type (chat, explain, implement, review, debug, complete, document)
        context: Retrieved context to include

    Returns:
        Formatted system prompt
    """
    template = SYSTEM_PROMPTS.get(task, SYSTEM_PROMPTS["chat"])
    return template.format(context=context)


def format_context(
    documents: list[dict],
    max_tokens: int = 4000,
    include_metadata: bool = True,
) -> str:
    """
    Format retrieved documents into context string.

    Args:
        documents: List of retrieved documents with 'content' and optional metadata
        max_tokens: Maximum tokens for context (rough estimate)
        include_metadata: Whether to include file path and other metadata

    Returns:
        Formatted context string
    """
    if not documents:
        return "Kein relevanter Kontext gefunden."

    context_parts = []
    estimated_tokens = 0

    for i, doc in enumerate(documents, 1):
        content = doc.get("content", "")
        metadata = doc.get("metadata", {})

        # Build document header
        header = f"--- Dokument {i}"
        if include_metadata:
            if path := metadata.get("path"):
                header += f" | {path}"
            if language := metadata.get("language"):
                header += f" | {language}"
            if score := doc.get("score"):
                header += f" | Score: {score:.2f}"
        header += " ---"

        doc_text = f"{header}\n{content}\n"

        # Rough token estimate (1 token ≈ 4 chars)
        doc_tokens = len(doc_text) // 4
        if estimated_tokens + doc_tokens > max_tokens:
            break

        context_parts.append(doc_text)
        estimated_tokens += doc_tokens

    return "\n".join(context_parts)


def build_chat_prompt(
    message: str,
    context: str,
    history: Optional[list[dict]] = None,
) -> tuple[str, str]:
    """
    Build chat prompt with context and history.

    Args:
        message: User message
        context: Retrieved context
        history: Optional chat history

    Returns:
        Tuple of (system_prompt, user_prompt)
    """
    system = get_system_prompt("chat", context)

    # Build user prompt with history
    if history:
        history_text = "\n".join(
            f"{'User' if h['role'] == 'user' else 'Assistant'}: {h['content']}"
            for h in history[-5:]  # Last 5 messages
        )
        user = f"Bisheriger Verlauf:\n{history_text}\n\nNeue Frage: {message}"
    else:
        user = message

    return system, user


def build_code_prompt(
    code: str,
    task: str,
    context: str,
    instruction: Optional[str] = None,
) -> tuple[str, str]:
    """
    Build code-related prompt.

    Args:
        code: Source code
        task: Task type
        context: Retrieved context
        instruction: Optional specific instruction

    Returns:
        Tuple of (system_prompt, user_prompt)
    """
    system = get_system_prompt(task, context)

    if instruction:
        user = f"{instruction}\n\nCode:\n```\n{code}\n```"
    else:
        user = f"```\n{code}\n```"

    return system, user
