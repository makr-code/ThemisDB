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


# ============================================================================
# Capabilities Endpoint (Self-Discovery)
# ============================================================================


@router.get("/capabilities")
async def get_capabilities():
    """
    Get server capabilities and available endpoints.
    
    Returns self-describing API with request/response patterns for each endpoint.
    Enables dynamic client integration (VS Code, MCP, etc.)
    """
    return {
        "server": {
            "name": "respo",
            "version": "1.0.0",
            "description": "RAG LLM Coding Assistant",
            "protocol_version": "2024-11-05",
        },
        "capabilities": {
            "chat": True,
            "streaming": True,
            "code_completion": True,
            "code_explanation": True,
            "code_review": True,
            "semantic_search": True,
            "ingestion": True,
            "agentic_planning": True,
            "deep_research": True,
            "mcp": True,
            "sse": True,
            "graph_search": _vector_store is not None and hasattr(_vector_store, "graph_search"),
        },
        "endpoints": {
            "chat": {
                "path": "/chat",
                "method": "POST",
                "description": "Interactive chat with code context",
                "request": {
                    "message": {"type": "string", "required": True, "description": "User message"},
                    "language": {"type": "string", "required": False, "default": "python"},
                    "history": {"type": "array", "required": False, "description": "Chat history"},
                },
                "response": {
                    "answer": {"type": "string", "description": "Generated response"},
                    "sources": {"type": "array", "description": "Retrieved code sources"},
                    "usage": {"type": "object", "description": "Token usage stats"},
                },
            },
            "chat_stream": {
                "path": "/chat/stream",
                "method": "POST",
                "description": "Streaming chat with SSE",
                "content_type": "text/event-stream",
                "request": {
                    "message": {"type": "string", "required": True},
                    "language": {"type": "string", "required": False},
                },
                "response": {
                    "format": "SSE",
                    "events": ["data", "[DONE]", "[ERROR]"],
                },
            },
            "complete": {
                "path": "/complete",
                "method": "POST",
                "description": "Code completion",
                "request": {
                    "code": {"type": "string", "required": True, "description": "Code to complete"},
                    "language": {"type": "string", "required": True},
                    "max_tokens": {"type": "integer", "required": False, "default": 512},
                },
                "response": {
                    "completion": {"type": "string"},
                    "confidence": {"type": "number"},
                },
            },
            "explain": {
                "path": "/explain",
                "method": "POST",
                "description": "Code explanation",
                "request": {
                    "code": {"type": "string", "required": True},
                    "language": {"type": "string", "required": False},
                    "detail_level": {"type": "string", "enum": ["brief", "detailed", "comprehensive"]},
                },
                "response": {
                    "explanation": {"type": "string"},
                    "concepts": {"type": "array"},
                },
            },
            "review": {
                "path": "/review",
                "method": "POST",
                "description": "Code review",
                "request": {
                    "code": {"type": "string", "required": True},
                    "focus": {"type": "array", "items": ["bugs", "security", "performance", "style"]},
                },
                "response": {
                    "summary": {"type": "string"},
                    "issues": {"type": "array"},
                    "score": {"type": "number"},
                },
            },
            "search": {
                "path": "/search",
                "method": "POST",
                "description": "Semantic code search",
                "request": {
                    "query": {"type": "string", "required": True},
                    "limit": {"type": "integer", "default": 10},
                    "language": {"type": "string", "required": False},
                },
                "response": {
                    "results": {"type": "array"},
                    "total": {"type": "integer"},
                    "query_time_ms": {"type": "number"},
                },
            },
            "ingest": {
                "path": "/ingest",
                "method": "POST",
                "description": "Ingest code into vector store",
                "request": {
                    "source_type": {"type": "string", "enum": ["github", "directory", "file"]},
                    "source": {"type": "string", "required": True, "description": "owner/repo or path"},
                    "branch": {"type": "string", "required": False},
                },
                "response": {
                    "status": {"type": "string", "enum": ["success", "partial", "error"]},
                    "files_processed": {"type": "integer"},
                    "chunks_indexed": {"type": "integer"},
                },
            },
            "plan": {
                "path": "/agents/plan",
                "method": "POST",
                "description": "Create execution plan for complex task",
                "request": {
                    "task": {"type": "string", "required": True, "description": "Complex task description"},
                    "context": {"type": "string", "required": False},
                },
                "response": {
                    "plan_id": {"type": "string"},
                    "goal": {"type": "string"},
                    "steps": {"type": "array"},
                },
            },
            "plan_stream": {
                "path": "/agents/plan/stream",
                "method": "POST",
                "description": "Stream plan execution via SSE",
                "content_type": "text/event-stream",
                "request": {
                    "task": {"type": "string", "required": True},
                },
                "response": {
                    "format": "SSE",
                    "events": ["plan_start", "step_start", "step_complete", "step_error", "plan_complete"],
                },
            },
            "research": {
                "path": "/agents/research",
                "method": "POST",
                "description": "Deep research on a topic",
                "request": {
                    "query": {"type": "string", "required": True},
                    "context": {"type": "string", "required": False},
                },
                "response": {
                    "summary": {"type": "string"},
                    "findings": {"type": "array"},
                    "code_examples": {"type": "array"},
                    "confidence": {"type": "number"},
                },
            },
            "research_stream": {
                "path": "/agents/research/stream",
                "method": "POST",
                "description": "Stream deep research via SSE",
                "content_type": "text/event-stream",
                "request": {
                    "query": {"type": "string", "required": True},
                },
                "response": {
                    "format": "SSE",
                    "events": ["research_start", "plan_created", "step_start", "step_complete", "finding", "code_example", "research_complete"],
                },
            },
            "mcp": {
                "path": "/mcp",
                "method": "POST",
                "description": "MCP (Model Context Protocol) endpoint for VS Code",
                "request": {
                    "jsonrpc": {"type": "string", "value": "2.0"},
                    "method": {"type": "string", "description": "MCP method (tools/list, tools/call, etc.)"},
                    "params": {"type": "object"},
                    "id": {"type": "any"},
                },
                "response": {
                    "jsonrpc": {"type": "string", "value": "2.0"},
                    "result": {"type": "object"},
                    "id": {"type": "any"},
                },
            },
        },
        "tools": [
            {"name": "respo_search", "description": "Semantic code search"},
            {"name": "respo_implement", "description": "Generate code implementation"},
            {"name": "respo_explain", "description": "Explain code"},
            {"name": "respo_review", "description": "Review code"},
            {"name": "respo_research", "description": "Deep research"},
            {"name": "respo_plan", "description": "Create execution plan"},
        ],
    }


# ============================================================================
# Agentic Endpoints (Deep Research, Planning)
# ============================================================================

# Global agent instances
_planner = None
_research_agent = None


def get_planner():
    """Get agentic planner instance."""
    if _planner is None:
        raise HTTPException(status_code=503, detail="Planner not initialized")
    return _planner


def get_research_agent():
    """Get deep research agent instance."""
    if _research_agent is None:
        raise HTTPException(status_code=503, detail="Research agent not initialized")
    return _research_agent


@router.post("/agents/plan")
async def create_plan(request: dict):
    """
    Create an execution plan for a complex task.
    
    Decomposes the task into smaller, actionable steps.
    """
    task = request.get("task")
    if not task:
        raise HTTPException(status_code=400, detail="task is required")

    planner = get_planner()
    context = {"user_context": request.get("context")} if request.get("context") else None

    try:
        plan = await planner.create_plan(task, context)
        return plan.to_dict()
    except Exception as e:
        logger.error("Plan error", error=str(e))
        raise HTTPException(status_code=500, detail=str(e))


@router.post("/agents/plan/stream")
async def stream_plan_execution(request: dict):
    """
    Stream plan execution via SSE.
    
    Creates a plan and streams execution progress in real-time.
    """
    import json

    task = request.get("task")
    if not task:
        raise HTTPException(status_code=400, detail="task is required")

    planner = get_planner()
    from respo.agents.executor import StepExecutor
    executor = StepExecutor(_llm_client, _vector_store, _pipeline)

    async def generate():
        try:
            plan = await planner.create_plan(task)
            async for event in planner.stream_plan_execution(plan, executor):
                yield f"event: {event['event']}\ndata: {json.dumps(event['data'])}\n\n"
        except Exception as e:
            yield f"event: error\ndata: {json.dumps({'error': str(e)})}\n\n"

    return StreamingResponse(generate(), media_type="text/event-stream")


@router.post("/agents/research")
async def deep_research(request: dict):
    """
    Perform deep research on a complex topic.
    
    Uses iterative search, analysis, and synthesis.
    """
    query = request.get("query")
    if not query:
        raise HTTPException(status_code=400, detail="query is required")

    agent = get_research_agent()
    context = request.get("context")

    try:
        result = await agent.research(query, context)
        return result.to_dict()
    except Exception as e:
        logger.error("Research error", error=str(e))
        raise HTTPException(status_code=500, detail=str(e))


@router.post("/agents/research/stream")
async def stream_research(request: dict):
    """
    Stream deep research via SSE.
    
    Provides real-time progress on research steps.
    """
    import json

    query = request.get("query")
    if not query:
        raise HTTPException(status_code=400, detail="query is required")

    agent = get_research_agent()
    context = request.get("context")

    async def generate():
        try:
            async for event in agent.stream_research(query, context):
                yield f"event: {event['event']}\ndata: {json.dumps(event['data'])}\n\n"
        except Exception as e:
            yield f"event: error\ndata: {json.dumps({'error': str(e)})}\n\n"

    return StreamingResponse(generate(), media_type="text/event-stream")


# ============================================================================
# MCP Endpoint
# ============================================================================

_mcp_handler = None


def get_mcp_handler():
    """Get MCP handler instance."""
    if _mcp_handler is None:
        raise HTTPException(status_code=503, detail="MCP handler not initialized")
    return _mcp_handler


@router.post("/mcp")
async def mcp_endpoint(request: dict):
    """
    MCP (Model Context Protocol) endpoint.
    
    Handles JSON-RPC requests for VS Code Copilot and other MCP clients.
    """
    handler = get_mcp_handler()

    try:
        response = await handler.handle_mcp_request(request)
        return response
    except Exception as e:
        logger.error("MCP error", error=str(e))
        return {
            "jsonrpc": "2.0",
            "id": request.get("id"),
            "error": {"code": -32603, "message": str(e)},
        }


# ============================================================================
# Task Management (CRUD) Endpoints for SSE/MCP
# ============================================================================

from respo.api.sessions import (
    TaskManager,
    TaskType,
    TaskStatus,
    TaskCreateRequest,
    TaskResponse,
    TaskListResponse,
    TaskActionResponse,
    get_task_manager,
)


def _task_to_response(task) -> TaskResponse:
    """Convert Task to TaskResponse."""
    return TaskResponse(
        id=task.id,
        type=task.type,
        status=task.status,
        created_at=task.created_at,
        updated_at=task.updated_at,
        started_at=task.started_at,
        completed_at=task.completed_at,
        progress={
            "current_step": task.progress.current_step,
            "total_steps": task.progress.total_steps,
            "percentage": task.progress.percentage,
            "current_step_name": task.progress.current_step_name,
            "message": task.progress.message,
        },
        error=task.error,
        metadata=task.metadata,
    )


@router.get("/tasks", response_model=TaskListResponse)
async def list_tasks(
    task_type: Optional[str] = None,
    status: Optional[str] = None,
    limit: int = 100,
) -> TaskListResponse:
    """
    List all tasks with optional filtering.
    
    Query parameters:
    - task_type: Filter by task type (plan_execution, deep_research, etc.)
    - status: Filter by status (pending, running, completed, cancelled, failed, paused)
    - limit: Maximum number of tasks to return (default 100)
    """
    manager = get_task_manager()
    
    type_filter = TaskType(task_type) if task_type else None
    status_filter = TaskStatus(status) if status else None
    
    tasks = await manager.list_tasks(
        task_type=type_filter,
        status=status_filter,
        limit=limit,
    )
    
    return TaskListResponse(
        tasks=[_task_to_response(t) for t in tasks],
        total=len(tasks),
    )


@router.post("/tasks", response_model=TaskResponse, status_code=201)
async def create_task(request: TaskCreateRequest) -> TaskResponse:
    """
    Create a new task.
    
    The task will be created in PENDING status. Use the task ID
    to start execution via the appropriate endpoint (e.g., /agents/plan/stream).
    """
    manager = get_task_manager()
    task = await manager.create_task(
        task_type=request.type,
        metadata=request.metadata,
    )
    
    logger.info("Task created", task_id=task.id, type=task.type)
    return _task_to_response(task)


@router.get("/tasks/{task_id}", response_model=TaskResponse)
async def get_task(task_id: str) -> TaskResponse:
    """
    Get task by ID.
    
    Returns the current status and progress of a task.
    """
    manager = get_task_manager()
    task = await manager.get_task(task_id)
    
    if not task:
        raise HTTPException(status_code=404, detail=f"Task {task_id} not found")
    
    return _task_to_response(task)


@router.post("/tasks/{task_id}/cancel", response_model=TaskActionResponse)
async def cancel_task(task_id: str) -> TaskActionResponse:
    """
    Cancel a running or pending task.
    
    Signals the task to stop gracefully. SSE streams will receive
    a cancellation event.
    """
    manager = get_task_manager()
    task = await manager.get_task(task_id)
    
    if not task:
        raise HTTPException(status_code=404, detail=f"Task {task_id} not found")
    
    success = await manager.cancel_task(task_id)
    
    logger.info("Task cancelled", task_id=task_id, success=success)
    
    return TaskActionResponse(
        success=success,
        task_id=task_id,
        action="cancel",
        message="Task cancelled" if success else "Task could not be cancelled",
    )


@router.post("/tasks/{task_id}/pause", response_model=TaskActionResponse)
async def pause_task(task_id: str) -> TaskActionResponse:
    """
    Pause a running task.
    
    The task will pause at the next checkpoint. SSE streams will
    receive a pause event.
    """
    manager = get_task_manager()
    task = await manager.get_task(task_id)
    
    if not task:
        raise HTTPException(status_code=404, detail=f"Task {task_id} not found")
    
    success = await manager.pause_task(task_id)
    
    logger.info("Task paused", task_id=task_id, success=success)
    
    return TaskActionResponse(
        success=success,
        task_id=task_id,
        action="pause",
        message="Task paused" if success else "Task could not be paused",
    )


@router.post("/tasks/{task_id}/resume", response_model=TaskActionResponse)
async def resume_task(task_id: str) -> TaskActionResponse:
    """
    Resume a paused task.
    
    The task will continue from where it was paused.
    """
    manager = get_task_manager()
    task = await manager.get_task(task_id)
    
    if not task:
        raise HTTPException(status_code=404, detail=f"Task {task_id} not found")
    
    success = await manager.resume_task(task_id)
    
    logger.info("Task resumed", task_id=task_id, success=success)
    
    return TaskActionResponse(
        success=success,
        task_id=task_id,
        action="resume",
        message="Task resumed" if success else "Task could not be resumed",
    )


@router.delete("/tasks/{task_id}", response_model=TaskActionResponse)
async def delete_task(task_id: str) -> TaskActionResponse:
    """
    Delete a task.
    
    Running tasks will be cancelled before deletion.
    """
    manager = get_task_manager()
    task = await manager.get_task(task_id)
    
    if not task:
        raise HTTPException(status_code=404, detail=f"Task {task_id} not found")
    
    success = await manager.delete_task(task_id)
    
    logger.info("Task deleted", task_id=task_id, success=success)
    
    return TaskActionResponse(
        success=success,
        task_id=task_id,
        action="delete",
        message="Task deleted" if success else "Task could not be deleted",
    )
