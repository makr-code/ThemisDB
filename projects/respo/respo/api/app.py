"""
RESPO API Application

FastAPI application for the RAG-Enhanced Software Programming Optimizer.
"""

import logging
from contextlib import asynccontextmanager
from typing import AsyncGenerator, Optional

import structlog
from fastapi import FastAPI
from fastapi.middleware.cors import CORSMiddleware

from respo.config import get_settings

# Configure logging
settings = get_settings()

if settings.log.format == "json":
    structlog.configure(
        processors=[
            structlog.stdlib.filter_by_level,
            structlog.stdlib.add_logger_name,
            structlog.stdlib.add_log_level,
            structlog.processors.TimeStamper(fmt="iso"),
            structlog.processors.JSONRenderer(),
        ],
        wrapper_class=structlog.stdlib.BoundLogger,
        context_class=dict,
        logger_factory=structlog.stdlib.LoggerFactory(),
    )
else:
    structlog.configure(
        processors=[
            structlog.stdlib.filter_by_level,
            structlog.dev.ConsoleRenderer(),
        ],
        wrapper_class=structlog.stdlib.BoundLogger,
        context_class=dict,
        logger_factory=structlog.stdlib.LoggerFactory(),
    )

logging.basicConfig(
    level=getattr(logging, settings.log.level.upper(), logging.INFO),
    format="%(message)s",
)

logger = structlog.get_logger(__name__)


# Global instances
_vector_store = None
_embedder = None
_llm_client = None
_pipeline = None
_ingestion_pipeline = None


async def initialize_components() -> None:
    """Initialize all components."""
    global _vector_store, _embedder, _llm_client, _pipeline, _ingestion_pipeline

    from respo.api.routes import endpoints

    # Initialize vector store
    logger.info("Initializing vector store", backend=settings.vector_store.backend)
    from respo.vectorstore.base import VectorStoreFactory
    try:
        _vector_store = VectorStoreFactory.create(
            settings.vector_store.backend,
            persist_directory=settings.vector_store.chroma_persist_dir,
        )
        endpoints._vector_store = _vector_store
    except Exception as e:
        logger.warning("Vector store init failed, using mock", error=str(e))

    # Initialize embedder
    logger.info("Initializing embedder", model=settings.embedding.model)
    try:
        from respo.embedding import CodeEmbedder
        _embedder = CodeEmbedder(
            model_name=settings.embedding.model,
            device=settings.embedding.device,
        )
    except Exception as e:
        logger.warning("Embedder init failed", error=str(e))

    # Initialize LLM client
    logger.info("Initializing LLM client", url=settings.vllm.url)
    try:
        from respo.llm import VLLMClient
        _llm_client = VLLMClient(
            base_url=f"{settings.vllm.url}/v1",
            model=settings.vllm.model,
        )
    except Exception as e:
        logger.warning("LLM client init failed", error=str(e))

    # Initialize RAG pipeline
    if _vector_store and _embedder and _llm_client:
        logger.info("Initializing RAG pipeline")
        try:
            from respo.rag import RAGPipeline
            _pipeline = RAGPipeline(
                vector_store=_vector_store,
                embedder=_embedder,
                llm_client=_llm_client,
            )
            endpoints._pipeline = _pipeline
        except Exception as e:
            logger.warning("RAG pipeline init failed", error=str(e))

    # Initialize ingestion pipeline
    if _vector_store and _embedder:
        logger.info("Initializing ingestion pipeline")
        try:
            from respo.ingestion.pipeline import IngestionPipeline
            _ingestion_pipeline = IngestionPipeline(
                vector_store=_vector_store,
                embedder=_embedder,
            )
            endpoints._ingestion_pipeline = _ingestion_pipeline
        except Exception as e:
            logger.warning("Ingestion pipeline init failed", error=str(e))


async def cleanup_components() -> None:
    """Cleanup all components."""
    global _vector_store, _llm_client

    if _vector_store:
        await _vector_store.close()

    if _llm_client:
        await _llm_client.close()


@asynccontextmanager
async def lifespan(app: FastAPI) -> AsyncGenerator[None, None]:
    """Application lifespan handler."""
    # Startup
    logger.info(
        "Starting RESPO API",
        vector_store=settings.vector_store.backend,
        vllm_url=settings.vllm.url,
        embedding_model=settings.embedding.model,
    )

    await initialize_components()

    yield

    # Shutdown
    logger.info("Shutting down RESPO API")
    await cleanup_components()


app = FastAPI(
    title="RESPO API",
    description="RAG-Enhanced Software Programming Optimizer - On-premise LLM coding assistant",
    version="0.1.0",
    docs_url="/docs",
    redoc_url="/redoc",
    openapi_url="/openapi.json",
    lifespan=lifespan,
)

# CORS
app.add_middleware(
    CORSMiddleware,
    allow_origins=settings.cors.origin_list,
    allow_credentials=True,
    allow_methods=["*"],
    allow_headers=["*"],
)

# Include API routes
from respo.api.routes.endpoints import router as api_router
app.include_router(api_router, tags=["api"])


# Health check
@app.get("/health")
async def health_check() -> dict:
    """Health check endpoint."""
    doc_count = 0
    if _vector_store:
        try:
            doc_count = await _vector_store.count()
        except Exception:
            pass

    return {
        "status": "healthy",
        "version": "0.1.0",
        "service": "respo",
        "vector_store": settings.vector_store.backend,
        "document_count": doc_count,
        "components": {
            "vector_store": _vector_store is not None,
            "embedder": _embedder is not None,
            "llm_client": _llm_client is not None,
            "rag_pipeline": _pipeline is not None,
            "ingestion": _ingestion_pipeline is not None,
        },
    }


# API info
@app.get("/")
async def root() -> dict:
    """Root endpoint with API information."""
    return {
        "name": "RESPO API",
        "description": "RAG-Enhanced Software Programming Optimizer",
        "version": "0.1.0",
        "docs": "/docs",
        "health": "/health",
        "endpoints": {
            "chat": "POST /chat",
            "chat_stream": "POST /chat/stream",
            "complete": "POST /complete",
            "explain": "POST /explain",
            "review": "POST /review",
            "search": "POST /search",
            "ingest": "POST /ingest",
        },
    }


def run_server() -> None:
    """Run the API server."""
    import uvicorn

    uvicorn.run(
        "respo.api.app:app",
        host=settings.api.host,
        port=settings.api.port,
        workers=settings.api.workers,
        reload=False,
    )


if __name__ == "__main__":
    run_server()
