# ThemisDB Tutorials

Welcome to the ThemisDB tutorials! This directory contains comprehensive, hands-on guides to help you master ThemisDB.

## 📚 Tutorial Index

### Getting Started
- **[Getting Started Tutorial](GETTING_STARTED_TUTORIAL.md)** - Your first steps with ThemisDB
  - Installation methods (Docker, binary, source)
  - First connection and database creation
  - Basic CRUD operations
  - Simple queries and indexes
  - *Time: 30-45 minutes*

### Core Operations
- **[CRUD Operations Tutorial](CRUD_TUTORIAL.md)** - Master create, read, update, and delete operations
  - Entity management
  - Bulk operations
  - Conditional updates
  - Upserts and merges
  - *Time: 30 minutes*

- **[Batch Operations Guide](BATCH_OPERATIONS.md)** - Efficient bulk data handling
  - Batch inserts and updates
  - Transaction batching
  - Performance optimization
  - Error handling strategies
  - *Time: 25 minutes*

### Advanced Topics
- **[Schema Design Tutorial](SCHEMA_DESIGN.md)** - Design optimal database schemas
  - Multi-model schema design
  - Normalization vs. denormalization
  - Index strategies
  - Real-world patterns
  - *Time: 45 minutes*

- **[Best Practices Guide](BEST_PRACTICES.md)** - Production-ready patterns
  - Query optimization
  - Security best practices
  - Performance tuning
  - Error handling
  - *Time: 40 minutes*

### Interactive Learning
- **[Interactive Examples](INTERACTIVE_EXAMPLES.md)** - Try it yourself!
  - Code snippets with explanations
  - Common patterns
  - Real-world scenarios
  - Links to runnable examples
  - *Time: varies*

- **[Video Tutorials](VIDEO_TUTORIALS.md)** - Visual learning resources
  - Video series index
  - Feature deep-dives
  - Use case walkthroughs
  - *Time: varies*

## 🎯 Learning Paths

### For Beginners (0-1 month experience)
1. [Getting Started Tutorial](GETTING_STARTED_TUTORIAL.md)
2. [CRUD Operations Tutorial](CRUD_TUTORIAL.md)
3. [Interactive Examples](INTERACTIVE_EXAMPLES.md)
4. Practice with [Hello World Example](../../examples/01_hello_world/)

### For Application Developers (1-3 months experience)
1. [Batch Operations Guide](BATCH_OPERATIONS.md)
2. [Schema Design Tutorial](SCHEMA_DESIGN.md)
3. [Best Practices Guide](BEST_PRACTICES.md)
4. Build a [Todo App](../../examples/02_todo_app/)

### For Database Architects (3+ months experience)
1. [Schema Design Tutorial](SCHEMA_DESIGN.md)
2. [Best Practices Guide](BEST_PRACTICES.md)
3. Review [Performance Guide](../performance/PERFORMANCE_GUIDE.md)
4. Study [Architecture Documentation](../architecture/)

## 📖 Documentation Structure

```
docs/
├── tutorials/              ← You are here
│   ├── GETTING_STARTED_TUTORIAL.md
│   ├── CRUD_TUTORIAL.md
│   ├── BATCH_OPERATIONS.md
│   ├── SCHEMA_DESIGN.md
│   ├── BEST_PRACTICES.md
│   ├── INTERACTIVE_EXAMPLES.md
│   └── VIDEO_TUTORIALS.md
├── api/                    # API Reference
├── architecture/           # System Architecture
├── deployment/            # Deployment Guides
├── performance/           # Performance Tuning
└── examples/              # Example Applications
```

## 🚀 Quick Start

If you're completely new to ThemisDB:

1. **Install ThemisDB** (choose one):
   ```bash
   # Docker (recommended)
   docker run -d -p 8080:8080 themisdb/themisdb:latest
   
   # Or from source
   git clone https://github.com/makr-code/ThemisDB.git
   cd ThemisDB && ./scripts/setup.sh && ./scripts/build.sh
   ```

2. **Follow the [Getting Started Tutorial](GETTING_STARTED_TUTORIAL.md)**

3. **Try the [Interactive Examples](INTERACTIVE_EXAMPLES.md)**

4. **Build something!** Pick an example from `/examples` directory

## 💡 Tips for Learning

- **Hands-On First**: Start with the Getting Started tutorial and type every command
- **Build Projects**: The best way to learn is by building real applications
- **Use Examples**: Explore the 20+ examples in `/examples` directory
- **Ask Questions**: Join our community on GitHub Discussions
- **Read Error Messages**: ThemisDB has helpful error messages - read them!

## 🆘 Need Help?

- **Getting Stuck?** Check the [FAQ](../FAQ.md)
- **Found a Bug?** Open an issue on [GitHub](https://github.com/makr-code/ThemisDB/issues)
- **Have Questions?** Start a [Discussion](https://github.com/makr-code/ThemisDB/discussions)
- **Want More Examples?** See [Examples Index](../EXAMPLES_INDEX.md)

## 📝 Contributing

Found an error or want to improve a tutorial? Contributions are welcome!

1. Fork the repository
2. Edit the tutorial
3. Submit a pull request

See [CONTRIBUTING.md](../../CONTRIBUTING.md) for guidelines.

## 🔗 Related Resources

- [Quick Start Guide](../../QUICKSTART.md) - Get running in 5 minutes
- [Examples Quickstart](../EXAMPLES_QUICKSTART.md) - Example-driven learning
- [API Reference](../api/) - Complete API documentation
- [FAQ](../FAQ.md) - Frequently asked questions
- [Full Documentation Hub](../DOCUMENTATION_HUB.md) - All documentation

---

**Ready to start?** Begin with the [Getting Started Tutorial](GETTING_STARTED_TUTORIAL.md) →
