"""
Session and Task Management for MCP SSE with CRUD operations.

Provides endpoints for creating, reading, updating, and cancelling
long-running tasks and SSE streams.
"""

import asyncio
import uuid
from dataclasses import dataclass, field
from datetime import datetime
from enum import Enum
from typing import Any, Callable, Coroutine, Optional

from pydantic import BaseModel


class TaskStatus(str, Enum):
    """Task execution status."""
    PENDING = "pending"
    RUNNING = "running"
    COMPLETED = "completed"
    CANCELLED = "cancelled"
    FAILED = "failed"
    PAUSED = "paused"


class TaskType(str, Enum):
    """Types of long-running tasks."""
    PLAN_EXECUTION = "plan_execution"
    DEEP_RESEARCH = "deep_research"
    CODE_INGESTION = "code_ingestion"
    BENCHMARK = "benchmark"
    TRAINING = "training"
    CUSTOM = "custom"


@dataclass
class TaskProgress:
    """Progress information for a task."""
    current_step: int = 0
    total_steps: int = 0
    percentage: float = 0.0
    current_step_name: str = ""
    message: str = ""


@dataclass
class Task:
    """Represents a long-running task."""
    id: str
    type: TaskType
    status: TaskStatus
    created_at: datetime
    updated_at: datetime
    started_at: Optional[datetime] = None
    completed_at: Optional[datetime] = None
    progress: TaskProgress = field(default_factory=TaskProgress)
    result: Any = None
    error: Optional[str] = None
    metadata: dict = field(default_factory=dict)
    cancel_event: asyncio.Event = field(default_factory=asyncio.Event)
    pause_event: asyncio.Event = field(default_factory=asyncio.Event)
    
    def __post_init__(self):
        # pause_event starts set (not paused)
        self.pause_event.set()


class TaskManager:
    """
    Manages long-running tasks with support for:
    - Creating and tracking tasks
    - Cancelling running tasks
    - Pausing and resuming tasks
    - Querying task status
    - Cleanup of completed tasks
    """
    
    def __init__(self, max_tasks: int = 1000, cleanup_after_hours: int = 24):
        self._tasks: dict[str, Task] = {}
        self._running_coroutines: dict[str, asyncio.Task] = {}
        self._max_tasks = max_tasks
        self._cleanup_after_hours = cleanup_after_hours
        self._lock = asyncio.Lock()
    
    async def create_task(
        self,
        task_type: TaskType,
        metadata: Optional[dict] = None
    ) -> Task:
        """Create a new task and return its ID."""
        async with self._lock:
            # Cleanup old tasks if limit reached
            if len(self._tasks) >= self._max_tasks:
                await self._cleanup_old_tasks()
            
            task_id = str(uuid.uuid4())
            now = datetime.utcnow()
            
            task = Task(
                id=task_id,
                type=task_type,
                status=TaskStatus.PENDING,
                created_at=now,
                updated_at=now,
                metadata=metadata or {}
            )
            
            self._tasks[task_id] = task
            return task
    
    async def start_task(
        self,
        task_id: str,
        coroutine: Coroutine
    ) -> None:
        """Start executing a task."""
        task = self._tasks.get(task_id)
        if not task:
            raise ValueError(f"Task {task_id} not found")
        
        task.status = TaskStatus.RUNNING
        task.started_at = datetime.utcnow()
        task.updated_at = datetime.utcnow()
        
        # Wrap coroutine to handle completion/errors
        async def _wrapped():
            try:
                result = await coroutine
                task.result = result
                task.status = TaskStatus.COMPLETED
            except asyncio.CancelledError:
                task.status = TaskStatus.CANCELLED
            except Exception as e:
                task.error = str(e)
                task.status = TaskStatus.FAILED
            finally:
                task.completed_at = datetime.utcnow()
                task.updated_at = datetime.utcnow()
        
        self._running_coroutines[task_id] = asyncio.create_task(_wrapped())
    
    async def get_task(self, task_id: str) -> Optional[Task]:
        """Get task by ID."""
        return self._tasks.get(task_id)
    
    async def list_tasks(
        self,
        task_type: Optional[TaskType] = None,
        status: Optional[TaskStatus] = None,
        limit: int = 100
    ) -> list[Task]:
        """List tasks with optional filtering."""
        tasks = list(self._tasks.values())
        
        if task_type:
            tasks = [t for t in tasks if t.type == task_type]
        if status:
            tasks = [t for t in tasks if t.status == status]
        
        # Sort by created_at descending
        tasks.sort(key=lambda t: t.created_at, reverse=True)
        return tasks[:limit]
    
    async def cancel_task(self, task_id: str) -> bool:
        """Cancel a running task."""
        task = self._tasks.get(task_id)
        if not task:
            return False
        
        if task.status not in [TaskStatus.PENDING, TaskStatus.RUNNING, TaskStatus.PAUSED]:
            return False
        
        # Signal cancellation
        task.cancel_event.set()
        
        # Cancel the asyncio task if running
        if task_id in self._running_coroutines:
            self._running_coroutines[task_id].cancel()
            try:
                await self._running_coroutines[task_id]
            except asyncio.CancelledError:
                pass
            del self._running_coroutines[task_id]
        
        task.status = TaskStatus.CANCELLED
        task.completed_at = datetime.utcnow()
        task.updated_at = datetime.utcnow()
        return True
    
    async def pause_task(self, task_id: str) -> bool:
        """Pause a running task."""
        task = self._tasks.get(task_id)
        if not task or task.status != TaskStatus.RUNNING:
            return False
        
        task.pause_event.clear()  # Clear event to pause
        task.status = TaskStatus.PAUSED
        task.updated_at = datetime.utcnow()
        return True
    
    async def resume_task(self, task_id: str) -> bool:
        """Resume a paused task."""
        task = self._tasks.get(task_id)
        if not task or task.status != TaskStatus.PAUSED:
            return False
        
        task.pause_event.set()  # Set event to resume
        task.status = TaskStatus.RUNNING
        task.updated_at = datetime.utcnow()
        return True
    
    async def update_progress(
        self,
        task_id: str,
        current_step: int,
        total_steps: int,
        step_name: str = "",
        message: str = ""
    ) -> None:
        """Update task progress."""
        task = self._tasks.get(task_id)
        if not task:
            return
        
        task.progress.current_step = current_step
        task.progress.total_steps = total_steps
        task.progress.percentage = (current_step / total_steps * 100) if total_steps > 0 else 0
        task.progress.current_step_name = step_name
        task.progress.message = message
        task.updated_at = datetime.utcnow()
    
    async def delete_task(self, task_id: str) -> bool:
        """Delete a task (only if completed/cancelled/failed)."""
        task = self._tasks.get(task_id)
        if not task:
            return False
        
        if task.status in [TaskStatus.PENDING, TaskStatus.RUNNING, TaskStatus.PAUSED]:
            # Must cancel first
            await self.cancel_task(task_id)
        
        del self._tasks[task_id]
        return True
    
    async def _cleanup_old_tasks(self) -> None:
        """Remove old completed/cancelled/failed tasks."""
        now = datetime.utcnow()
        cutoff_hours = self._cleanup_after_hours
        
        to_delete = []
        for task_id, task in self._tasks.items():
            if task.status in [TaskStatus.COMPLETED, TaskStatus.CANCELLED, TaskStatus.FAILED]:
                if task.completed_at:
                    hours_old = (now - task.completed_at).total_seconds() / 3600
                    if hours_old > cutoff_hours:
                        to_delete.append(task_id)
        
        for task_id in to_delete:
            del self._tasks[task_id]
    
    def is_cancelled(self, task_id: str) -> bool:
        """Check if task has been cancelled."""
        task = self._tasks.get(task_id)
        return task.cancel_event.is_set() if task else True
    
    async def wait_if_paused(self, task_id: str) -> bool:
        """Wait if task is paused. Returns False if cancelled."""
        task = self._tasks.get(task_id)
        if not task:
            return False
        
        # Wait for pause_event (set means not paused)
        while not task.pause_event.is_set():
            if task.cancel_event.is_set():
                return False
            await asyncio.sleep(0.1)
        
        return not task.cancel_event.is_set()


# Pydantic models for API
class TaskCreateRequest(BaseModel):
    """Request to create a new task."""
    type: TaskType
    metadata: dict = {}


class TaskResponse(BaseModel):
    """Task information response."""
    id: str
    type: TaskType
    status: TaskStatus
    created_at: datetime
    updated_at: datetime
    started_at: Optional[datetime] = None
    completed_at: Optional[datetime] = None
    progress: dict = {}
    error: Optional[str] = None
    metadata: dict = {}


class TaskListResponse(BaseModel):
    """List of tasks response."""
    tasks: list[TaskResponse]
    total: int


class TaskActionResponse(BaseModel):
    """Response for task actions (cancel, pause, resume, delete)."""
    success: bool
    task_id: str
    action: str
    message: str


# Global task manager instance
_task_manager: Optional[TaskManager] = None


def get_task_manager() -> TaskManager:
    """Get or create the global task manager."""
    global _task_manager
    if _task_manager is None:
        _task_manager = TaskManager()
    return _task_manager
