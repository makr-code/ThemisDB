"""Real HTTP-backed, serial YCSB-inspired operations; not an upstream binding."""

import asyncio
import hashlib
import json
import random
import string
import uuid

import httpx


class ThemisDBYCSBAdapter:
    """Own an isolated benchmark collection using a caller-owned HTTP client.

    Load and cleanup are explicit, outside measurement. Reads validate the last
    successful write. Writes replace whole records; read-modify-write is two
    non-atomic HTTP calls. Only serial use and A/B/C/F are supported. Key access
    is uniform, not the upstream YCSB Zipfian distribution. HTTP/thread-dispatch
    and validation costs are included in request latency. Always call cleanup,
    including after a partially failed load, then close the client separately.
    """

    def __init__(self, client: httpx.Client, workload: str, *, record_count: int = 1000,
                 field_count: int = 10, field_length: int = 100, seed: int = 42):
        if workload not in {"A", "B", "C", "F"}:
            raise ValueError("ThemisDB adapter supports only A/B/C/F; D/E require latest/scan semantics")
        for value in (record_count, field_count, field_length):
            if isinstance(value, bool) or not isinstance(value, int) or value <= 0:
                raise ValueError("Record count and field dimensions must be positive integers")
        self.client = client
        self.workload = workload
        self.record_count = record_count
        self.field_count = field_count
        self.field_length = field_length
        self.seed = seed
        self.collection = "ycsb_" + uuid.uuid4().hex
        self._generator = random.Random(seed)
        self._hashes = {}
        self._loaded = False

    def _value(self):
        return "".join(self._generator.choices(string.ascii_letters + string.digits, k=self.field_length))

    def _record(self):
        return {f"field{index}": self._value() for index in range(self.field_count)}

    @staticmethod
    def _digest(record):
        return hashlib.sha256(json.dumps(record, sort_keys=True, separators=(",", ":")).encode()).digest()

    def _put(self, key, record):
        self._hashes.setdefault(key, None)
        response = self.client.put(
            f"/entities/{self.collection}:{key}",
            json={"blob": json.dumps(record, separators=(",", ":"))},
        )
        response.raise_for_status()
        if response.status_code not in (200, 201) or response.json().get("success") is not True:
            raise RuntimeError(f"Database rejected benchmark write: {key}")
        self._hashes[key] = self._digest(record)

    def _get(self, key):
        response = self.client.get(f"/entities/{self.collection}:{key}")
        response.raise_for_status()
        payload = response.json()
        if payload.get("key") != f"{self.collection}:{key}" or not isinstance(payload.get("blob"), str):
            raise RuntimeError(f"Benchmark read envelope validation failed: {key}")
        record = json.loads(payload["blob"])
        if not isinstance(record, dict) or self._digest(record) != self._hashes[key]:
            raise RuntimeError(f"Benchmark read validation failed: {key}")
        return record

    async def load(self):
        """Create and read-validate the dataset; raise on any failed operation."""
        if self._hashes:
            raise RuntimeError("Adapter already has records; cleanup before loading again")
        await asyncio.to_thread(self._load)

    def _load(self):
        self._generator.seed(self.seed)
        for index in range(self.record_count):
            key = f"user{index:020d}"
            self._put(key, self._record())
            self._get(key)
        self._loaded = True

    async def __call__(self, operation_type: str):
        """Execute one serial operation; propagate transport or validation errors."""
        if not self._loaded:
            raise RuntimeError("Load and validate the benchmark dataset before measuring")
        allowed = {"C": {"read"}, "A": {"read", "write"},
                   "B": {"read", "write"}, "F": {"read", "read_modify_write"}}
        if operation_type not in allowed[self.workload]:
            raise ValueError(f"Unsupported operation for workload {self.workload}: {operation_type}")
        await asyncio.to_thread(self._operate, operation_type)

    def _operate(self, operation_type):
        key = f"user{self._generator.randrange(self.record_count):020d}"
        if operation_type == "read":
            self._get(key)
        elif operation_type == "write":
            self._put(key, self._record())
        else:
            record = self._get(key)
            record[f"field{self._generator.randrange(self.field_count)}"] = self._value()
            self._put(key, record)

    async def cleanup(self):
        """Delete this adapter's attempted writes, retaining keys on delete errors."""
        self._loaded = False
        await asyncio.to_thread(self._cleanup)

    def _cleanup(self):
        for key in list(self._hashes):
            response = self.client.delete(f"/entities/{self.collection}:{key}")
            if response.status_code != 404:
                response.raise_for_status()
            del self._hashes[key]