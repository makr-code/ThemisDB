"""
RESPO Load Testing with Locust

Performance benchmarks for the RESPO API endpoints.

Usage:
    cd projects/respo/loadtest
    pip install locust
    locust -f locustfile.py --host=http://localhost:8080
"""

from locust import HttpUser, task, between, tag
import random


class ChatUser(HttpUser):
    """Simulates users making chat requests."""
    
    wait_time = between(1, 3)
    weight = 3
    
    sample_messages = [
        "Wie implementiere ich einen LRU Cache in Python?",
        "Erkläre mir async/await in JavaScript",
        "Was ist der Unterschied zwischen REST und GraphQL?",
        "Wie funktioniert Dependency Injection?",
        "Best practices für Error Handling in Go",
    ]
    
    @task(10)
    @tag("chat")
    def chat(self):
        message = random.choice(self.sample_messages)
        self.client.post("/chat", json={"message": message})
    
    @task(3)
    @tag("complete")
    def complete(self):
        self.client.post("/complete", json={"code": "def fib(n):\n    ", "language": "python"})


class SearchUser(HttpUser):
    """Simulates users making search requests."""
    
    wait_time = between(2, 5)
    weight = 2
    
    @task
    @tag("search")
    def search(self):
        queries = ["database connection", "authentication", "error handling"]
        self.client.post("/search", json={"query": random.choice(queries), "limit": 10})


class HealthCheckUser(HttpUser):
    """Simulates monitoring health checks."""
    
    wait_time = between(5, 10)
    weight = 1
    
    @task
    @tag("health")
    def health_check(self):
        self.client.get("/health")
