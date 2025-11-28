"""Tests for Task Management CRUD endpoints."""

import pytest
from unittest.mock import AsyncMock, MagicMock, patch
import asyncio
from datetime import datetime

from respo.api.sessions import TaskManager, Task, TaskStatus


class TestTask:
    """Tests for Task dataclass."""

    def test_task_creation(self):
        """Test creating a task."""
        task = Task(
            id="task-123",
            type="deep_research",
            status=TaskStatus.PENDING,
            metadata={"query": "test query"}
        )
        
        assert task.id == "task-123"
        assert task.type == "deep_research"
        assert task.status == TaskStatus.PENDING
        assert task.metadata["query"] == "test query"
        assert task.created_at is not None

    def test_task_status_transitions(self):
        """Test task status transitions."""
        task = Task(
            id="task-1",
            type="plan_execution",
            status=TaskStatus.PENDING
        )
        
        # Pending -> Running
        task.status = TaskStatus.RUNNING
        assert task.status == TaskStatus.RUNNING
        
        # Running -> Paused
        task.status = TaskStatus.PAUSED
        assert task.status == TaskStatus.PAUSED
        
        # Paused -> Running
        task.status = TaskStatus.RUNNING
        assert task.status == TaskStatus.RUNNING
        
        # Running -> Completed
        task.status = TaskStatus.COMPLETED
        assert task.status == TaskStatus.COMPLETED

    def test_task_to_dict(self):
        """Test converting task to dictionary."""
        task = Task(
            id="task-1",
            type="research",
            status=TaskStatus.RUNNING,
            progress=50,
            result={"partial": "data"}
        )
        
        data = task.to_dict()
        
        assert data["id"] == "task-1"
        assert data["type"] == "research"
        assert data["status"] == "running"
        assert data["progress"] == 50
        assert data["result"]["partial"] == "data"


class TestTaskStatus:
    """Tests for TaskStatus enum."""

    def test_status_values(self):
        """Test all status values exist."""
        assert TaskStatus.PENDING.value == "pending"
        assert TaskStatus.RUNNING.value == "running"
        assert TaskStatus.PAUSED.value == "paused"
        assert TaskStatus.COMPLETED.value == "completed"
        assert TaskStatus.FAILED.value == "failed"
        assert TaskStatus.CANCELLED.value == "cancelled"

    def test_status_from_string(self):
        """Test creating status from string."""
        assert TaskStatus("pending") == TaskStatus.PENDING
        assert TaskStatus("running") == TaskStatus.RUNNING


class TestTaskManager:
    """Tests for TaskManager."""

    @pytest.fixture
    def task_manager(self):
        """Create a task manager instance."""
        return TaskManager()

    def test_create_task(self, task_manager):
        """Test creating a new task."""
        task = task_manager.create_task(
            task_type="deep_research",
            metadata={"query": "LRU cache"}
        )
        
        assert task.id is not None
        assert task.type == "deep_research"
        assert task.status == TaskStatus.PENDING
        assert task.metadata["query"] == "LRU cache"

    def test_get_task(self, task_manager):
        """Test getting a task by ID."""
        created = task_manager.create_task("research")
        retrieved = task_manager.get_task(created.id)
        
        assert retrieved is not None
        assert retrieved.id == created.id

    def test_get_nonexistent_task(self, task_manager):
        """Test getting a task that doesn't exist."""
        task = task_manager.get_task("nonexistent-id")
        assert task is None

    def test_list_tasks(self, task_manager):
        """Test listing all tasks."""
        task_manager.create_task("research")
        task_manager.create_task("plan")
        task_manager.create_task("ingest")
        
        tasks = task_manager.list_tasks()
        
        assert len(tasks) == 3

    def test_list_tasks_by_status(self, task_manager):
        """Test listing tasks filtered by status."""
        t1 = task_manager.create_task("research")
        t2 = task_manager.create_task("plan")
        
        t1.status = TaskStatus.RUNNING
        
        running_tasks = task_manager.list_tasks(status=TaskStatus.RUNNING)
        pending_tasks = task_manager.list_tasks(status=TaskStatus.PENDING)
        
        assert len(running_tasks) == 1
        assert running_tasks[0].id == t1.id
        assert len(pending_tasks) == 1
        assert pending_tasks[0].id == t2.id

    def test_list_tasks_by_type(self, task_manager):
        """Test listing tasks filtered by type."""
        task_manager.create_task("research")
        task_manager.create_task("research")
        task_manager.create_task("plan")
        
        research_tasks = task_manager.list_tasks(task_type="research")
        
        assert len(research_tasks) == 2

    def test_update_task_status(self, task_manager):
        """Test updating task status."""
        task = task_manager.create_task("research")
        
        updated = task_manager.update_status(task.id, TaskStatus.RUNNING)
        
        assert updated is True
        assert task.status == TaskStatus.RUNNING

    def test_update_task_progress(self, task_manager):
        """Test updating task progress."""
        task = task_manager.create_task("research")
        task.status = TaskStatus.RUNNING
        
        task_manager.update_progress(task.id, 50)
        
        assert task.progress == 50

    def test_cancel_task(self, task_manager):
        """Test cancelling a task."""
        task = task_manager.create_task("research")
        task.status = TaskStatus.RUNNING
        
        cancelled = task_manager.cancel_task(task.id)
        
        assert cancelled is True
        assert task.status == TaskStatus.CANCELLED

    def test_cancel_completed_task(self, task_manager):
        """Test that completed tasks cannot be cancelled."""
        task = task_manager.create_task("research")
        task.status = TaskStatus.COMPLETED
        
        cancelled = task_manager.cancel_task(task.id)
        
        assert cancelled is False
        assert task.status == TaskStatus.COMPLETED

    def test_pause_task(self, task_manager):
        """Test pausing a task."""
        task = task_manager.create_task("research")
        task.status = TaskStatus.RUNNING
        
        paused = task_manager.pause_task(task.id)
        
        assert paused is True
        assert task.status == TaskStatus.PAUSED

    def test_resume_task(self, task_manager):
        """Test resuming a paused task."""
        task = task_manager.create_task("research")
        task.status = TaskStatus.PAUSED
        
        resumed = task_manager.resume_task(task.id)
        
        assert resumed is True
        assert task.status == TaskStatus.RUNNING

    def test_delete_task(self, task_manager):
        """Test deleting a task."""
        task = task_manager.create_task("research")
        task_id = task.id
        
        deleted = task_manager.delete_task(task_id)
        
        assert deleted is True
        assert task_manager.get_task(task_id) is None

    def test_delete_running_task(self, task_manager):
        """Test that running tasks cannot be deleted."""
        task = task_manager.create_task("research")
        task.status = TaskStatus.RUNNING
        
        deleted = task_manager.delete_task(task.id)
        
        assert deleted is False
        assert task_manager.get_task(task.id) is not None

    def test_set_task_result(self, task_manager):
        """Test setting task result."""
        task = task_manager.create_task("research")
        result = {"findings": ["item1", "item2"]}
        
        task_manager.set_result(task.id, result)
        
        assert task.result == result

    def test_set_task_error(self, task_manager):
        """Test setting task error."""
        task = task_manager.create_task("research")
        
        task_manager.set_error(task.id, "Something went wrong")
        
        assert task.status == TaskStatus.FAILED
        assert task.error == "Something went wrong"


class TestTaskManagerConcurrency:
    """Tests for TaskManager thread safety."""

    @pytest.fixture
    def task_manager(self):
        """Create a task manager instance."""
        return TaskManager()

    @pytest.mark.asyncio
    async def test_concurrent_task_creation(self, task_manager):
        """Test creating tasks concurrently."""
        async def create_task(i):
            return task_manager.create_task(f"task_{i}")
        
        tasks = await asyncio.gather(*[create_task(i) for i in range(100)])
        
        assert len(tasks) == 100
        assert len(set(t.id for t in tasks)) == 100  # All unique IDs

    @pytest.mark.asyncio
    async def test_concurrent_status_updates(self, task_manager):
        """Test updating status concurrently."""
        task = task_manager.create_task("research")
        task.status = TaskStatus.RUNNING
        
        async def update_progress(value):
            task_manager.update_progress(task.id, value)
        
        await asyncio.gather(*[update_progress(i) for i in range(100)])
        
        # Progress should be one of the values (last write wins)
        assert 0 <= task.progress < 100
