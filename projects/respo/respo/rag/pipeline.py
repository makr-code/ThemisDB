"""
RESPO RAG Pipeline

Complete RAG pipeline combining retrieval, reranking, and generation.
"""

from dataclasses import dataclass
from typing import Any, AsyncIterator, Optional

import structlog

from respo.embedding import CodeEmbedder
from respo.llm import GenerationConfig, VLLMClient
from respo.rag.prompts import build_chat_prompt, format_context, get_system_prompt
from respo.rag.reranker import CrossEncoderReranker, NoOpReranker
from respo.rag.retriever import HybridRetriever
from respo.vectorstore.base import VectorStoreBase

logger = structlog.get_logger(__name__)


@dataclass
class RAGResponse:
    """Response from RAG pipeline."""

    answer: str
    sources: list[dict[str, Any]]
    prompt_tokens: int
    completion_tokens: int


class RAGPipeline:
    """
    Complete RAG pipeline for code assistance.

    Combines:
    - Retrieval (vector + optional keyword)
    - Reranking (cross-encoder)
    - Generation (vLLM)
    """

    def __init__(
        self,
        vector_store: VectorStoreBase,
        embedder: CodeEmbedder,
        llm_client: VLLMClient,
        reranker: Optional[CrossEncoderReranker] = None,
        retrieval_k: int = 50,
        rerank_k: int = 10,
        max_context_tokens: int = 4000,
    ) -> None:
        """
        Initialize RAG pipeline.

        Args:
            vector_store: Vector store backend
            embedder: Code embedder
            llm_client: vLLM client
            reranker: Optional cross-encoder reranker
            retrieval_k: Number of documents to retrieve
            rerank_k: Number of documents after reranking
            max_context_tokens: Maximum tokens for context
        """
        self.retriever = HybridRetriever(vector_store, embedder)
        self.reranker = reranker or NoOpReranker()
        self.llm_client = llm_client
        self.retrieval_k = retrieval_k
        self.rerank_k = rerank_k
        self.max_context_tokens = max_context_tokens

    async def query(
        self,
        question: str,
        task: str = "chat",
        language: Optional[str] = None,
        lora_adapter: Optional[str] = None,
        generation_config: Optional[GenerationConfig] = None,
    ) -> RAGResponse:
        """
        Execute RAG query.

        Args:
            question: User question
            task: Task type (chat, explain, review, etc.)
            language: Optional language filter
            lora_adapter: Optional LoRA adapter
            generation_config: Generation configuration

        Returns:
            RAG response with answer and sources
        """
        logger.info("RAG query", question=question[:100], task=task)

        # 1. Retrieve
        retrieval_results = await self.retriever.retrieve(
            query=question,
            k=self.retrieval_k,
            language=language,
        )

        # 2. Rerank
        docs_for_rerank = [
            {
                "id": r.id,
                "content": r.content,
                "score": r.score,
                "metadata": r.metadata,
            }
            for r in retrieval_results
        ]
        reranked = self.reranker.rerank(question, docs_for_rerank, top_k=self.rerank_k)

        # 3. Build context
        context_docs = [
            {
                "content": r.content,
                "metadata": r.metadata,
                "score": r.score,
            }
            for r in reranked
        ]
        context = format_context(context_docs, max_tokens=self.max_context_tokens)

        # 4. Build prompt
        system_prompt = get_system_prompt(task, context)

        # 5. Generate
        result = await self.llm_client.generate(
            prompt=question,
            system_prompt=system_prompt,
            config=generation_config,
            lora_adapter=lora_adapter,
        )

        # Build sources
        sources = [
            {
                "id": r.id,
                "path": r.metadata.get("path", "unknown"),
                "score": r.score,
                "language": r.metadata.get("language"),
            }
            for r in reranked
        ]

        return RAGResponse(
            answer=result.text,
            sources=sources,
            prompt_tokens=result.prompt_tokens,
            completion_tokens=result.completion_tokens,
        )

    async def query_stream(
        self,
        question: str,
        task: str = "chat",
        language: Optional[str] = None,
        lora_adapter: Optional[str] = None,
        generation_config: Optional[GenerationConfig] = None,
    ) -> AsyncIterator[str]:
        """
        Execute RAG query with streaming response.

        Args:
            question: User question
            task: Task type
            language: Optional language filter
            lora_adapter: Optional LoRA adapter
            generation_config: Generation configuration

        Yields:
            Response text chunks
        """
        logger.info("RAG streaming query", question=question[:100], task=task)

        # 1. Retrieve
        retrieval_results = await self.retriever.retrieve(
            query=question,
            k=self.retrieval_k,
            language=language,
        )

        # 2. Rerank
        docs_for_rerank = [
            {
                "id": r.id,
                "content": r.content,
                "score": r.score,
                "metadata": r.metadata,
            }
            for r in retrieval_results
        ]
        reranked = self.reranker.rerank(question, docs_for_rerank, top_k=self.rerank_k)

        # 3. Build context
        context_docs = [
            {
                "content": r.content,
                "metadata": r.metadata,
                "score": r.score,
            }
            for r in reranked
        ]
        context = format_context(context_docs, max_tokens=self.max_context_tokens)

        # 4. Build prompt
        system_prompt = get_system_prompt(task, context)

        # 5. Generate with streaming
        async for chunk in self.llm_client.generate_stream(
            prompt=question,
            system_prompt=system_prompt,
            config=generation_config,
            lora_adapter=lora_adapter,
        ):
            yield chunk

    async def chat(
        self,
        message: str,
        history: Optional[list[dict]] = None,
        language: Optional[str] = None,
    ) -> RAGResponse:
        """
        Chat with RAG context.

        Args:
            message: User message
            history: Optional chat history
            language: Optional language filter

        Returns:
            RAG response
        """
        # Build search query from message and history
        search_query = message
        if history:
            # Include last user message for better retrieval
            last_user = next(
                (h["content"] for h in reversed(history) if h["role"] == "user"),
                None,
            )
            if last_user:
                search_query = f"{last_user} {message}"

        return await self.query(
            question=search_query,
            task="chat",
            language=language,
        )
