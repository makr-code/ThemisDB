"""
RESPO API Models

Pydantic models for API request/response schemas.
"""

from typing import Any, Optional

from pydantic import BaseModel, Field


# ============================================================================
# Chat Models
# ============================================================================


class ChatMessage(BaseModel):
    """A chat message."""

    role: str = Field(..., description="Role: 'user' or 'assistant'")
    content: str = Field(..., description="Message content")


class ChatRequest(BaseModel):
    """Chat request."""

    message: str = Field(..., description="User message")
    history: list[ChatMessage] = Field(default_factory=list, description="Chat history")
    language: Optional[str] = Field(None, description="Programming language filter")
    stream: bool = Field(False, description="Enable streaming response")

    class Config:
        json_schema_extra = {
            "example": {
                "message": "Wie implementiere ich einen LRU Cache in Python?",
                "language": "python",
                "stream": False,
            }
        }


class ChatResponse(BaseModel):
    """Chat response."""

    answer: str = Field(..., description="Generated answer")
    sources: list[dict[str, Any]] = Field(
        default_factory=list, description="Source documents used"
    )
    usage: dict[str, int] = Field(
        default_factory=dict, description="Token usage statistics"
    )


# ============================================================================
# Code Models
# ============================================================================


class CodeRequest(BaseModel):
    """Base request for code-related endpoints."""

    code: str = Field(..., description="Source code")
    language: Optional[str] = Field(None, description="Programming language")
    instruction: Optional[str] = Field(None, description="Specific instruction")


class CompleteRequest(CodeRequest):
    """Code completion request."""

    cursor_position: Optional[int] = Field(
        None, description="Cursor position for completion"
    )
    max_tokens: int = Field(256, description="Maximum tokens to generate")

    class Config:
        json_schema_extra = {
            "example": {
                "code": "def calculate_sum(numbers):\n    ",
                "language": "python",
                "max_tokens": 100,
            }
        }


class CompleteResponse(BaseModel):
    """Code completion response."""

    completion: str = Field(..., description="Generated completion")
    confidence: float = Field(..., description="Confidence score 0-1")


class ExplainRequest(CodeRequest):
    """Code explanation request."""

    detail_level: str = Field(
        "medium", description="Detail level: 'brief', 'medium', 'detailed'"
    )

    class Config:
        json_schema_extra = {
            "example": {
                "code": "@lru_cache(maxsize=128)\ndef fibonacci(n):\n    if n < 2:\n        return n\n    return fibonacci(n-1) + fibonacci(n-2)",
                "language": "python",
                "detail_level": "medium",
            }
        }


class ExplainResponse(BaseModel):
    """Code explanation response."""

    explanation: str = Field(..., description="Code explanation")
    concepts: list[str] = Field(
        default_factory=list, description="Key concepts identified"
    )


class ReviewRequest(CodeRequest):
    """Code review request."""

    focus_areas: list[str] = Field(
        default_factory=lambda: ["bugs", "security", "performance", "style"],
        description="Areas to focus review on",
    )

    class Config:
        json_schema_extra = {
            "example": {
                "code": "def process_user_input(data):\n    exec(data)\n    return True",
                "language": "python",
                "focus_areas": ["security", "bugs"],
            }
        }


class ReviewIssue(BaseModel):
    """A review issue."""

    severity: str = Field(..., description="Severity: 'critical', 'warning', 'info'")
    category: str = Field(..., description="Issue category")
    line: Optional[int] = Field(None, description="Line number")
    message: str = Field(..., description="Issue description")
    suggestion: Optional[str] = Field(None, description="Suggested fix")


class ReviewResponse(BaseModel):
    """Code review response."""

    summary: str = Field(..., description="Review summary")
    issues: list[ReviewIssue] = Field(default_factory=list, description="Found issues")
    score: int = Field(..., description="Code quality score 0-100")


# ============================================================================
# Search Models
# ============================================================================


class SearchRequest(BaseModel):
    """Code search request."""

    query: str = Field(..., description="Search query")
    language: Optional[str] = Field(None, description="Language filter")
    repo: Optional[str] = Field(None, description="Repository filter")
    limit: int = Field(10, description="Maximum results", ge=1, le=100)

    class Config:
        json_schema_extra = {
            "example": {
                "query": "database connection pooling",
                "language": "python",
                "limit": 10,
            }
        }


class SearchResult(BaseModel):
    """A search result."""

    id: str = Field(..., description="Document ID")
    path: str = Field(..., description="File path")
    content: str = Field(..., description="Code snippet")
    score: float = Field(..., description="Relevance score")
    language: Optional[str] = Field(None, description="Programming language")
    metadata: dict[str, Any] = Field(default_factory=dict, description="Metadata")


class SearchResponse(BaseModel):
    """Search response."""

    results: list[SearchResult] = Field(default_factory=list, description="Results")
    total: int = Field(..., description="Total matching documents")
    query_time_ms: float = Field(..., description="Query time in milliseconds")


# ============================================================================
# Ingestion Models
# ============================================================================


class IngestRequest(BaseModel):
    """Ingestion request."""

    source_type: str = Field(
        ..., description="Source type: 'github', 'directory', 'file'"
    )
    source: str = Field(
        ..., description="Source location (repo URL, path, or content)"
    )
    language: Optional[str] = Field(None, description="Language filter")
    branch: Optional[str] = Field(None, description="Branch for GitHub repos")

    class Config:
        json_schema_extra = {
            "example": {
                "source_type": "github",
                "source": "owner/repo",
                "language": "python",
            }
        }


class IngestResponse(BaseModel):
    """Ingestion response."""

    status: str = Field(..., description="Status: 'success', 'partial', 'failed'")
    files_processed: int = Field(..., description="Number of files processed")
    chunks_indexed: int = Field(..., description="Number of chunks indexed")
    errors: int = Field(0, description="Number of errors")
    message: Optional[str] = Field(None, description="Status message")


# ============================================================================
# Status Models
# ============================================================================


class HealthResponse(BaseModel):
    """Health check response."""

    status: str = Field(..., description="Service status")
    version: str = Field(..., description="API version")
    vector_store: str = Field(..., description="Vector store backend")
    document_count: int = Field(..., description="Total indexed documents")
