"""
RESPO API Routes

Implemented API endpoints for RAG operations.
"""

import time
from typing import Optional

import structlog
from fastapi import APIRouter, Depends, HTTPException
from fastapi.responses import StreamingResponse

from respo.api.models import (
    ChatRequest,
    ChatResponse,
    CompleteRequest,
    CompleteResponse,
    ExplainRequest,
    ExplainResponse,
    IngestRequest,
    IngestResponse,
    ReviewIssue,
    ReviewRequest,
    ReviewResponse,
    SearchRequest,
    SearchResponse,
    SearchResult,
)
from respo.config import Settings, get_settings

logger = structlog.get_logger(__name__)

router = APIRouter()


# Global instances (initialized in app lifespan)
_pipeline = None
_vector_store = None
_embedder = None
_llm_client = None
_ingestion_pipeline = None


def get_pipeline():
    """Get RAG pipeline instance."""
    if _pipeline is None:
        raise HTTPException(
            status_code=503, detail="RAG pipeline not initialized"
        )
    return _pipeline


def get_vector_store():
    """Get vector store instance."""
    if _vector_store is None:
        raise HTTPException(
            status_code=503, detail="Vector store not initialized"
        )
    return _vector_store


def get_ingestion():
    """Get ingestion pipeline instance."""
    if _ingestion_pipeline is None:
        raise HTTPException(
            status_code=503, detail="Ingestion pipeline not initialized"
        )
    return _ingestion_pipeline


# ============================================================================
# Chat Endpoint
# ============================================================================


@router.post("/chat", response_model=ChatResponse)
async def chat(request: ChatRequest) -> ChatResponse:
    """
    Interactive chat with code context.

    Uses RAG to retrieve relevant code snippets and generate contextual responses.
    """
    pipeline = get_pipeline()

    logger.info("Chat request", message=request.message[:100])

    try:
        # Build history for context
        history = [
            {"role": msg.role, "content": msg.content}
            for msg in request.history
        ]

        # Query RAG pipeline
        response = await pipeline.chat(
            message=request.message,
            history=history,
            language=request.language,
        )

        return ChatResponse(
            answer=response.answer,
            sources=response.sources,
            usage={
                "prompt_tokens": response.prompt_tokens,
                "completion_tokens": response.completion_tokens,
            },
        )

    except Exception as e:
        logger.error("Chat error", error=str(e))
        raise HTTPException(status_code=500, detail=str(e))


@router.post("/chat/stream")
async def chat_stream(request: ChatRequest):
    """
    Streaming chat with code context.

    Returns a streaming response with incremental text generation.
    """
    pipeline = get_pipeline()

    async def generate():
        try:
            async for chunk in pipeline.query_stream(
                question=request.message,
                task="chat",
                language=request.language,
            ):
                yield f"data: {chunk}\n\n"
            yield "data: [DONE]\n\n"
        except Exception as e:
            logger.error("Stream error", error=str(e))
            yield f"data: [ERROR] {str(e)}\n\n"

    return StreamingResponse(
        generate(),
        media_type="text/event-stream",
    )


# ============================================================================
# Code Endpoints
# ============================================================================


@router.post("/complete", response_model=CompleteResponse)
async def complete(request: CompleteRequest) -> CompleteResponse:
    """
    Code completion.

    Generates code to complete the given snippet.
    """
    pipeline = get_pipeline()

    logger.info("Complete request", language=request.language)

    try:
        response = await pipeline.query(
            question=f"Complete this code:\n```\n{request.code}\n```",
            task="complete",
            language=request.language,
        )

        return CompleteResponse(
            completion=response.answer,
            confidence=0.85,  # TODO: Calculate actual confidence
        )

    except Exception as e:
        logger.error("Complete error", error=str(e))
        raise HTTPException(status_code=500, detail=str(e))


@router.post("/explain", response_model=ExplainResponse)
async def explain(request: ExplainRequest) -> ExplainResponse:
    """
    Code explanation.

    Generates a detailed explanation of the provided code.
    """
    pipeline = get_pipeline()

    logger.info("Explain request", language=request.language)

    try:
        instruction = f"Explain this code in {request.detail_level} detail:"
        if request.instruction:
            instruction = request.instruction

        response = await pipeline.query(
            question=f"{instruction}\n```\n{request.code}\n```",
            task="explain",
            language=request.language,
        )

        # Extract key concepts (simplified)
        concepts = []
        keywords = ["function", "class", "decorator", "recursion", "cache",
                   "async", "generator", "iterator", "context manager"]
        for kw in keywords:
            if kw.lower() in response.answer.lower():
                concepts.append(kw)

        return ExplainResponse(
            explanation=response.answer,
            concepts=concepts[:5],
        )

    except Exception as e:
        logger.error("Explain error", error=str(e))
        raise HTTPException(status_code=500, detail=str(e))


@router.post("/review", response_model=ReviewResponse)
async def review(request: ReviewRequest) -> ReviewResponse:
    """
    Code review.

    Analyzes code for bugs, security issues, and best practices.
    """
    pipeline = get_pipeline()

    logger.info("Review request", focus=request.focus_areas)

    try:
        focus_str = ", ".join(request.focus_areas)
        response = await pipeline.query(
            question=f"Review this code focusing on {focus_str}:\n```\n{request.code}\n```",
            task="review",
            language=request.language,
        )

        # Parse issues from response (simplified)
        issues = []

        # Check for common security issues
        if "security" in request.focus_areas:
            if "exec(" in request.code or "eval(" in request.code:
                issues.append(ReviewIssue(
                    severity="critical",
                    category="security",
                    message="Use of exec/eval detected - potential code injection",
                    suggestion="Avoid exec/eval with user input",
                ))

            if "password" in request.code.lower() and "=" in request.code:
                issues.append(ReviewIssue(
                    severity="warning",
                    category="security",
                    message="Hardcoded password detected",
                    suggestion="Use environment variables or secrets manager",
                ))

        # Calculate score
        critical_count = sum(1 for i in issues if i.severity == "critical")
        warning_count = sum(1 for i in issues if i.severity == "warning")
        score = max(0, 100 - (critical_count * 30) - (warning_count * 10))

        return ReviewResponse(
            summary=response.answer,
            issues=issues,
            score=score,
        )

    except Exception as e:
        logger.error("Review error", error=str(e))
        raise HTTPException(status_code=500, detail=str(e))


# ============================================================================
# Search Endpoint
# ============================================================================


@router.post("/search", response_model=SearchResponse)
async def search(request: SearchRequest) -> SearchResponse:
    """
    Semantic code search.

    Searches indexed code using semantic similarity.
    """
    vector_store = get_vector_store()
    settings = get_settings()

    logger.info("Search request", query=request.query[:100])

    start_time = time.time()

    try:
        # Get embedder
        from respo.embedding import CodeEmbedder
        embedder = CodeEmbedder(model_name=settings.embedding.model)

        # Generate query embedding
        query_embedding = embedder.embed_single(request.query)

        # Build filter
        search_filter = {}
        if request.language:
            search_filter["language"] = request.language
        if request.repo:
            search_filter["repo"] = request.repo

        # Search
        results = await vector_store.search(
            query_embedding=query_embedding,
            k=request.limit,
            filter=search_filter if search_filter else None,
        )

        query_time = (time.time() - start_time) * 1000

        return SearchResponse(
            results=[
                SearchResult(
                    id=r.id,
                    path=r.metadata.get("path", "unknown"),
                    content=r.content,
                    score=r.score,
                    language=r.metadata.get("language"),
                    metadata=r.metadata,
                )
                for r in results
            ],
            total=len(results),
            query_time_ms=query_time,
        )

    except Exception as e:
        logger.error("Search error", error=str(e))
        raise HTTPException(status_code=500, detail=str(e))


# ============================================================================
# Ingestion Endpoint
# ============================================================================


@router.post("/ingest", response_model=IngestResponse)
async def ingest(request: IngestRequest) -> IngestResponse:
    """
    Ingest code into the vector store.

    Supports GitHub repositories, directories, and individual files.
    """
    ingestion = get_ingestion()

    logger.info("Ingest request", source_type=request.source_type, source=request.source)

    try:
        if request.source_type == "github":
            # Parse owner/repo
            parts = request.source.split("/")
            if len(parts) != 2:
                raise HTTPException(
                    status_code=400,
                    detail="Invalid GitHub source format. Use 'owner/repo'",
                )

            owner, repo = parts
            stats = await ingestion.ingest_github_repo(
                owner=owner,
                repo=repo,
                branch=request.branch,
            )

        elif request.source_type == "directory":
            from pathlib import Path
            directory = Path(request.source)
            if not directory.exists():
                raise HTTPException(
                    status_code=400,
                    detail=f"Directory not found: {request.source}",
                )

            stats = await ingestion.ingest_directory(directory)

        else:
            raise HTTPException(
                status_code=400,
                detail=f"Unsupported source type: {request.source_type}",
            )

        status = "success" if stats.errors == 0 else "partial"

        return IngestResponse(
            status=status,
            files_processed=stats.files_processed,
            chunks_indexed=stats.chunks_indexed,
            errors=stats.errors,
            message=f"Ingested {stats.files_processed} files, {stats.chunks_indexed} chunks",
        )

    except HTTPException:
        raise
    except Exception as e:
        logger.error("Ingest error", error=str(e))
        raise HTTPException(status_code=500, detail=str(e))
