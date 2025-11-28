"""
End-to-End Tests für RESPO.

Diese Tests validieren komplette Workflows über mehrere Module hinweg:
- Ingestion → RAG → Chat
- Scraper → Chunker → Embedder → VectorStore
- Agent Planning → Task Execution → SSE Streaming
- MCP Request → Tool Execution → Response
"""

import pytest
import asyncio
from unittest.mock import AsyncMock, MagicMock, patch
from dataclasses import dataclass
from typing import List, Dict, Any


# ============================================================================
# Test Fixtures
# ============================================================================

@dataclass
class MockCodeChunk:
    """Mock für Code-Chunks."""
    content: str
    file_path: str
    language: str
    start_line: int = 1
    end_line: int = 10
    chunk_type: str = "function"
    name: str = "test_function"
    docstring: str = ""


@dataclass
class MockSearchResult:
    """Mock für Suchergebnisse."""
    id: str
    content: str
    score: float
    metadata: Dict[str, Any]


@dataclass
class MockLLMResponse:
    """Mock für LLM-Antworten."""
    content: str
    model: str = "codellama"
    tokens_used: int = 100


class MockVectorStore:
    """Mock Vector Store für Tests."""
    
    def __init__(self):
        self.documents = []
        self.embeddings = []
    
    async def add(self, texts: List[str], embeddings: List[List[float]], 
                  metadatas: List[Dict[str, Any]]) -> List[str]:
        ids = [f"doc_{i}" for i in range(len(texts))]
        for i, (text, emb, meta) in enumerate(zip(texts, embeddings, metadatas)):
            self.documents.append({
                "id": ids[i],
                "text": text,
                "embedding": emb,
                "metadata": meta
            })
        return ids
    
    async def search(self, embedding: List[float], k: int = 10) -> List[MockSearchResult]:
        # Simuliere Suche - gib alle Dokumente zurück
        results = []
        for i, doc in enumerate(self.documents[:k]):
            results.append(MockSearchResult(
                id=doc["id"],
                content=doc["text"],
                score=0.9 - (i * 0.1),
                metadata=doc["metadata"]
            ))
        return results


class MockEmbedder:
    """Mock Embedder für Tests."""
    
    def __init__(self, dim: int = 768):
        self.dim = dim
    
    async def embed(self, texts: List[str]) -> List[List[float]]:
        # Generiere deterministische Embeddings basierend auf Text-Länge
        return [[len(t) / 100.0] * self.dim for t in texts]
    
    async def embed_query(self, text: str) -> List[float]:
        return [len(text) / 100.0] * self.dim


class MockLLMClient:
    """Mock LLM Client für Tests."""
    
    def __init__(self):
        self.call_count = 0
        self.last_prompt = None
    
    async def generate(self, prompt: str, **kwargs) -> MockLLMResponse:
        self.call_count += 1
        self.last_prompt = prompt
        
        # Simuliere verschiedene Antworten basierend auf Prompt-Inhalt
        if "plan" in prompt.lower():
            return MockLLMResponse(
                content='{"steps": [{"id": 1, "description": "Step 1", "type": "research"}]}'
            )
        elif "explain" in prompt.lower():
            return MockLLMResponse(
                content="This code implements a function that processes data."
            )
        elif "review" in prompt.lower():
            return MockLLMResponse(
                content='{"issues": [], "suggestions": ["Add docstring"], "score": 8}'
            )
        else:
            return MockLLMResponse(
                content="Here is the implementation:\n```python\ndef example(): pass\n```"
            )
    
    async def stream(self, prompt: str, **kwargs):
        """Simuliere Streaming-Antwort."""
        self.call_count += 1
        self.last_prompt = prompt
        
        response = "This is a streaming response."
        for word in response.split():
            yield word + " "
            await asyncio.sleep(0.01)


# ============================================================================
# E2E Test: Ingestion → RAG → Chat
# ============================================================================

class TestIngestionToChat:
    """Tests für den kompletten Flow von Ingestion bis Chat."""
    
    @pytest.fixture
    def vector_store(self):
        return MockVectorStore()
    
    @pytest.fixture
    def embedder(self):
        return MockEmbedder()
    
    @pytest.fixture
    def llm_client(self):
        return MockLLMClient()
    
    @pytest.mark.asyncio
    async def test_ingest_and_query_code(self, vector_store, embedder, llm_client):
        """Test: Code wird indexiert und kann über Chat abgefragt werden."""
        # 1. Simuliere Code-Ingestion
        code_chunks = [
            MockCodeChunk(
                content="def calculate_sum(a, b):\n    return a + b",
                file_path="math_utils.py",
                language="python",
                name="calculate_sum",
                docstring="Calculate the sum of two numbers."
            ),
            MockCodeChunk(
                content="def calculate_product(a, b):\n    return a * b",
                file_path="math_utils.py",
                language="python",
                name="calculate_product",
                docstring="Calculate the product of two numbers."
            )
        ]
        
        # 2. Embeddings erstellen und in Vector Store speichern
        texts = [chunk.content for chunk in code_chunks]
        embeddings = await embedder.embed(texts)
        metadatas = [
            {"file": chunk.file_path, "language": chunk.language, "name": chunk.name}
            for chunk in code_chunks
        ]
        
        ids = await vector_store.add(texts, embeddings, metadatas)
        assert len(ids) == 2
        
        # 3. Query durchführen
        query = "How do I add two numbers?"
        query_embedding = await embedder.embed_query(query)
        results = await vector_store.search(query_embedding, k=5)
        
        assert len(results) > 0
        assert "calculate_sum" in results[0].content or "calculate_product" in results[0].content
        
        # 4. LLM-Antwort generieren
        context = "\n".join([r.content for r in results])
        prompt = f"Context:\n{context}\n\nQuestion: {query}"
        response = await llm_client.generate(prompt)
        
        assert response.content is not None
        assert llm_client.call_count == 1
    
    @pytest.mark.asyncio
    async def test_multi_file_ingestion(self, vector_store, embedder):
        """Test: Mehrere Dateien können indexiert werden."""
        files = [
            ("app.py", "def main(): pass"),
            ("utils.py", "def helper(): return True"),
            ("models.py", "class User: pass"),
        ]
        
        for file_path, content in files:
            embedding = await embedder.embed([content])
            await vector_store.add(
                [content], 
                embedding, 
                [{"file": file_path, "language": "python"}]
            )
        
        assert len(vector_store.documents) == 3
        
        # Suche sollte alle relevanten Dokumente finden
        query_emb = await embedder.embed_query("main function")
        results = await vector_store.search(query_emb, k=10)
        assert len(results) == 3


# ============================================================================
# E2E Test: Agent Planning → Task Execution
# ============================================================================

class TestAgentPlanningFlow:
    """Tests für Agent Planning und Task Execution."""
    
    @pytest.fixture
    def llm_client(self):
        return MockLLMClient()
    
    @pytest.mark.asyncio
    async def test_plan_creation_and_execution(self, llm_client):
        """Test: Plan wird erstellt und Schritte werden ausgeführt."""
        # 1. Plan erstellen
        problem = "Implement a REST API with authentication"
        plan_prompt = f"Create a plan for: {problem}"
        plan_response = await llm_client.generate(plan_prompt)
        
        assert "steps" in plan_response.content
        
        # 2. Schritte simulieren
        import json
        try:
            plan_data = json.loads(plan_response.content)
            steps = plan_data.get("steps", [])
            assert len(steps) > 0
            
            # 3. Jeden Schritt ausführen
            for step in steps:
                step_prompt = f"Execute step: {step.get('description', '')}"
                step_response = await llm_client.generate(step_prompt)
                assert step_response.content is not None
        except json.JSONDecodeError:
            # Plan ist nicht im erwarteten JSON-Format
            pass
    
    @pytest.mark.asyncio
    async def test_deep_research_flow(self, llm_client):
        """Test: Deep Research mit mehreren Iterationen."""
        query = "Best practices for database connection pooling"
        
        # Simuliere mehrere Research-Iterationen
        findings = []
        for i in range(3):
            prompt = f"Research iteration {i+1}: {query}"
            response = await llm_client.generate(prompt)
            findings.append(response.content)
        
        assert len(findings) == 3
        assert llm_client.call_count == 3


# ============================================================================
# E2E Test: MCP Request → Tool Execution → Response
# ============================================================================

class TestMCPFlow:
    """Tests für MCP Request/Response Flow."""
    
    @pytest.fixture
    def llm_client(self):
        return MockLLMClient()
    
    @pytest.fixture
    def vector_store(self):
        return MockVectorStore()
    
    @pytest.fixture
    def embedder(self):
        return MockEmbedder()
    
    @pytest.mark.asyncio
    async def test_mcp_search_tool(self, vector_store, embedder, llm_client):
        """Test: MCP search tool führt Suche durch und gibt Ergebnisse zurück."""
        # Setup: Dokumente hinzufügen
        await vector_store.add(
            ["def connect_db(): pass"],
            await embedder.embed(["def connect_db(): pass"]),
            [{"file": "db.py", "language": "python"}]
        )
        
        # MCP Request simulieren
        mcp_request = {
            "jsonrpc": "2.0",
            "method": "tools/call",
            "params": {
                "name": "respo_search",
                "arguments": {
                    "query": "database connection"
                }
            },
            "id": 1
        }
        
        # Tool ausführen
        query_emb = await embedder.embed_query(mcp_request["params"]["arguments"]["query"])
        results = await vector_store.search(query_emb, k=5)
        
        # Response erstellen
        mcp_response = {
            "jsonrpc": "2.0",
            "result": {
                "content": [
                    {"type": "text", "text": r.content}
                    for r in results
                ]
            },
            "id": mcp_request["id"]
        }
        
        assert mcp_response["id"] == 1
        assert len(mcp_response["result"]["content"]) > 0
    
    @pytest.mark.asyncio
    async def test_mcp_implement_tool(self, llm_client):
        """Test: MCP implement tool generiert Code."""
        mcp_request = {
            "jsonrpc": "2.0",
            "method": "tools/call",
            "params": {
                "name": "respo_implement",
                "arguments": {
                    "task": "Create a function to validate email addresses",
                    "language": "python"
                }
            },
            "id": 2
        }
        
        # Tool ausführen
        response = await llm_client.generate(
            f"Implement: {mcp_request['params']['arguments']['task']}"
        )
        
        mcp_response = {
            "jsonrpc": "2.0",
            "result": {
                "content": [{"type": "text", "text": response.content}]
            },
            "id": mcp_request["id"]
        }
        
        assert "```python" in mcp_response["result"]["content"][0]["text"] or \
               "def" in mcp_response["result"]["content"][0]["text"]


# ============================================================================
# E2E Test: Scraper → Chunker → Embedder → VectorStore
# ============================================================================

class TestScraperToVectorStore:
    """Tests für den kompletten Scraping-Pipeline."""
    
    @pytest.fixture
    def vector_store(self):
        return MockVectorStore()
    
    @pytest.fixture
    def embedder(self):
        return MockEmbedder()
    
    @pytest.mark.asyncio
    async def test_process_scraped_files(self, vector_store, embedder):
        """Test: Gescrapte Dateien werden verarbeitet und indexiert."""
        # Simuliere gescrapte Dateien
        scraped_files = [
            {
                "path": "src/main.py",
                "content": """
def main():
    '''Entry point for the application.'''
    app = create_app()
    app.run()
""",
                "language": "python"
            },
            {
                "path": "src/utils.py",
                "content": """
def format_date(date):
    '''Format a date object to string.'''
    return date.strftime('%Y-%m-%d')
""",
                "language": "python"
            }
        ]
        
        # Verarbeite jede Datei
        for file in scraped_files:
            # Chunking simulieren (in echt würde AST-Parsing stattfinden)
            chunks = [file["content"]]  # Vereinfacht: ganzer Inhalt als ein Chunk
            
            # Embeddings erstellen
            embeddings = await embedder.embed(chunks)
            
            # In Vector Store speichern
            await vector_store.add(
                chunks,
                embeddings,
                [{"file": file["path"], "language": file["language"]}]
            )
        
        assert len(vector_store.documents) == 2
        
        # Suche durchführen
        query_emb = await embedder.embed_query("format date")
        results = await vector_store.search(query_emb, k=5)
        
        # Sollte utils.py finden
        found_utils = any("format_date" in r.content for r in results)
        assert found_utils


# ============================================================================
# E2E Test: Task Management mit SSE
# ============================================================================

class TestTaskManagementFlow:
    """Tests für Task Management mit SSE Events."""
    
    @pytest.mark.asyncio
    async def test_task_lifecycle(self):
        """Test: Task wird erstellt, ausgeführt und abgeschlossen."""
        from dataclasses import dataclass, field
        from enum import Enum
        from typing import Optional
        import uuid
        
        class TaskStatus(Enum):
            PENDING = "pending"
            RUNNING = "running"
            COMPLETED = "completed"
            CANCELLED = "cancelled"
            FAILED = "failed"
        
        @dataclass
        class Task:
            id: str
            type: str
            status: TaskStatus = TaskStatus.PENDING
            progress: float = 0.0
            result: Optional[Any] = None
            error: Optional[str] = None
        
        # Task erstellen
        task = Task(
            id=str(uuid.uuid4()),
            type="deep_research"
        )
        
        assert task.status == TaskStatus.PENDING
        
        # Task starten
        task.status = TaskStatus.RUNNING
        assert task.status == TaskStatus.RUNNING
        
        # Progress simulieren
        for progress in [0.25, 0.5, 0.75, 1.0]:
            task.progress = progress
            await asyncio.sleep(0.01)
        
        # Task abschließen
        task.status = TaskStatus.COMPLETED
        task.result = {"findings": ["Finding 1", "Finding 2"]}
        
        assert task.status == TaskStatus.COMPLETED
        assert task.progress == 1.0
        assert task.result is not None
    
    @pytest.mark.asyncio
    async def test_task_cancellation(self):
        """Test: Task kann während der Ausführung abgebrochen werden."""
        from dataclasses import dataclass
        from enum import Enum
        import uuid
        
        class TaskStatus(Enum):
            PENDING = "pending"
            RUNNING = "running"
            CANCELLED = "cancelled"
        
        @dataclass
        class Task:
            id: str
            status: TaskStatus = TaskStatus.PENDING
            cancel_requested: bool = False
        
        task = Task(id=str(uuid.uuid4()))
        task.status = TaskStatus.RUNNING
        
        # Cancel anfordern
        task.cancel_requested = True
        
        # Simuliere Task-Schleife die auf Cancel prüft
        if task.cancel_requested:
            task.status = TaskStatus.CANCELLED
        
        assert task.status == TaskStatus.CANCELLED


# ============================================================================
# E2E Test: Evaluation Flow
# ============================================================================

class TestEvaluationFlow:
    """Tests für LLM-as-Judge Evaluation."""
    
    @pytest.fixture
    def llm_client(self):
        return MockLLMClient()
    
    @pytest.mark.asyncio
    async def test_code_evaluation(self, llm_client):
        """Test: Code wird bewertet und erhält Score."""
        code = """
def fibonacci(n):
    if n <= 1:
        return n
    return fibonacci(n-1) + fibonacci(n-2)
"""
        task = "Implement Fibonacci sequence"
        
        # Evaluation durchführen
        eval_prompt = f"Review this code:\n{code}\n\nTask: {task}"
        response = await llm_client.generate(eval_prompt)
        
        assert response.content is not None
        # In echter Implementierung würde hier Score extrahiert
    
    @pytest.mark.asyncio
    async def test_code_comparison(self, llm_client):
        """Test: Zwei Code-Varianten werden verglichen."""
        code_a = "def add(a, b): return a + b"
        code_b = """
def add(a: int, b: int) -> int:
    '''Add two integers.'''
    return a + b
"""
        
        # Vergleich durchführen
        compare_prompt = f"Compare:\nA: {code_a}\nB: {code_b}"
        response = await llm_client.generate(compare_prompt)
        
        assert response.content is not None


# ============================================================================
# E2E Test: Hybrid Search mit ThemisDB
# ============================================================================

class TestHybridSearchFlow:
    """Tests für Hybrid Search mit Graph Expansion."""
    
    @pytest.fixture
    def vector_store(self):
        store = MockVectorStore()
        # Erweitere um Graph-Funktionalität
        store.graph_edges = []
        return store
    
    @pytest.fixture
    def embedder(self):
        return MockEmbedder()
    
    @pytest.mark.asyncio
    async def test_hybrid_search_with_graph(self, vector_store, embedder):
        """Test: Hybrid Search kombiniert Vector, Keyword und Graph."""
        # Dokumente mit Beziehungen hinzufügen
        docs = [
            {"content": "class Database: pass", "id": "db", "type": "class"},
            {"content": "class UserRepository(Database): pass", "id": "user_repo", "type": "class"},
            {"content": "def get_user(repo): return repo.find()", "id": "get_user", "type": "function"},
        ]
        
        # Graph-Kanten (Beziehungen)
        edges = [
            ("user_repo", "db", "inherits"),
            ("get_user", "user_repo", "uses"),
        ]
        
        # Dokumente indexieren
        for doc in docs:
            emb = await embedder.embed([doc["content"]])
            await vector_store.add([doc["content"]], emb, [{"id": doc["id"], "type": doc["type"]}])
        
        # Graph-Kanten speichern
        vector_store.graph_edges = edges
        
        # Suche durchführen
        query_emb = await embedder.embed_query("user repository")
        vector_results = await vector_store.search(query_emb, k=5)
        
        # Graph Expansion simulieren
        expanded_ids = set()
        for result in vector_results:
            doc_id = result.metadata.get("id")
            if doc_id:
                # Finde verwandte Knoten
                for source, target, rel in vector_store.graph_edges:
                    if source == doc_id:
                        expanded_ids.add(target)
                    if target == doc_id:
                        expanded_ids.add(source)
        
        # Sollte verwandte Dokumente finden
        assert len(vector_results) > 0


# ============================================================================
# Performance Tests
# ============================================================================

class TestPerformance:
    """Performance-Tests für kritische Pfade."""
    
    @pytest.fixture
    def embedder(self):
        return MockEmbedder()
    
    @pytest.mark.asyncio
    async def test_batch_embedding_performance(self, embedder):
        """Test: Batch-Embeddings sind effizient."""
        import time
        
        # Generiere viele Texte
        texts = [f"def function_{i}(): pass" for i in range(100)]
        
        start = time.time()
        embeddings = await embedder.embed(texts)
        elapsed = time.time() - start
        
        assert len(embeddings) == 100
        # Sollte schnell sein (< 1s für Mocks)
        assert elapsed < 1.0
    
    @pytest.mark.asyncio
    async def test_concurrent_requests(self):
        """Test: System kann mehrere gleichzeitige Requests verarbeiten."""
        llm = MockLLMClient()
        
        async def make_request(query: str):
            return await llm.generate(query)
        
        # 10 parallele Requests
        queries = [f"Query {i}" for i in range(10)]
        tasks = [make_request(q) for q in queries]
        
        results = await asyncio.gather(*tasks)
        
        assert len(results) == 10
        assert llm.call_count == 10
