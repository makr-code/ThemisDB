# ThemisDB Docs Examples

> This folder contains documentation that accompanies specific code examples.
> The runnable example applications live in the top-level [`examples/`](../../examples/) directory.

---

## 📄 Files in This Directory

- **[load_model_from_themisdb_example.md](load_model_from_themisdb_example.md)** – How to load an ML model stored in ThemisDB at inference time

---

## 🚀 Getting Started with Examples

| Resource | Description |
|----------|-------------|
| [EXAMPLES_QUICKSTART.md](../EXAMPLES_QUICKSTART.md) | Guided tour: get productive in under an hour |
| [EXAMPLES_INDEX.md](../EXAMPLES_INDEX.md) | Full index of all 37+ example applications |

### Quickstart

```bash
# Start ThemisDB
docker run -d --name themisdb -p 8080:8080 themisdb/themisdb:latest

# Run the Hello World example
cd examples/01_hello_world
pip install -r requirements.txt
python main.py
```

---

## 📂 Example Categories

| Level | Examples | Topics |
|-------|----------|--------|
| ⭐ Beginner | 01–03, 11–13 | CRUD, full-text search, document model |
| ⭐⭐ Intermediate | 04–09 | Multi-model, graph, vector search, time-series |
| ⭐⭐⭐ Advanced | 10, 14–23 | LLM, e-commerce, real-time, AI, recommendations |

See the [Examples Index](../EXAMPLES_INDEX.md) for descriptions and quick-start commands for each example.

