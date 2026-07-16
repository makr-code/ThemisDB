# ThemisDB Examples Quickstart Guide

**Welcome to ThemisDB!** This guide will get you from zero to productive in less than an hour.

**Quick Links:** [Examples Index](EXAMPLES_INDEX.md) | [Full Documentation](../docs/) | [API Reference](api/API_REFERENCE.md)

---

## 🎯 Your Learning Journey

This quickstart is organized by time commitment:

- **[First 10 Minutes](#-your-first-10-minutes)** - Get ThemisDB running and create your first data
- **[First 30 Minutes](#-your-first-30-minutes)** - Learn CRUD operations and queries
- **[First Hour](#-your-first-hour)** - Explore vector search and real-time features
- **[First Day](#-your-first-day)** - Build production-ready applications

After completing this guide, explore the [Learning Paths](#-learning-paths-by-role) tailored to your role.

---

## ⚡ Your First 10 Minutes

### Step 1: Start ThemisDB Server (2 minutes)

The fastest way to get started is with Docker:

```bash
# Pull and run ThemisDB
docker run -d \
  --name themisdb \
  -p 8080:8080 \
  -p 18765:18765 \
  themisdb/themisdb:latest

# Verify it's running
curl http://localhost:8080/health
```

**Expected Output:**
```json
{
  "status": "healthy",
  "version": "1.9.0-beta"
}
```

**No Docker?** See the [Installation Guide](../README.md#installation) for alternative methods.

---

### Step 2: Run Hello World (5 minutes)

Let's run your first ThemisDB application:

```bash
# Clone the repository (if not already done)
git clone https://github.com/makr-code/ThemisDB.git
cd ThemisDB

# Navigate to Hello World example
cd examples/01_hello_world

# Install dependencies
pip install -r requirements.txt

# Run the application
python main.py
```

**What you'll see:**
- A simple GUI window with user management
- Forms to create, read, update, and delete users
- Real-time interaction with ThemisDB

**Try this:**
1. Enter name "Alice" and email "alice@example.com"
2. Click "Create User" - you've just stored your first data!
3. The user ID will be displayed
4. Click "Get User" to retrieve the data
5. Modify the name and click "Update User"
6. Click "Delete User" to remove it

**🎉 Congratulations!** You just performed your first CRUD operations with ThemisDB.

---

### Step 3: Understanding the Code (3 minutes)

Open `examples/01_hello_world/themis_client.py` and examine the core operations:

**Create:**
```python
def create_user(self, user_id, name, email):
    user_data = {
        "id": user_id,
        "name": name,
        "email": email
    }
    response = requests.post(
        f"{self.base_url}/api/v1/data",
        json=user_data
    )
    return response.json()
```

**Read:**
```python
def get_user(self, user_id):
    response = requests.get(
        f"{self.base_url}/api/v1/data/{user_id}"
    )
    return response.json()
```

**Update & Delete:** Similar patterns using PUT and DELETE methods.

**Key Takeaway:** ThemisDB uses a simple REST API for all operations.

---

## 🚀 Your First 30 Minutes

Now that you understand the basics, let's explore more features.

### Step 4: Learn Queries with Todo App (10 minutes)

The Todo App demonstrates filtering and searching:

```bash
cd ../02_todo_app
pip install -r requirements.txt
python main.py
```

**Try this:**
1. Create several tasks with different statuses (Open, In Progress, Done)
2. Use the filter dropdown to filter by status
3. Search for tasks by name
4. Mark tasks as complete
5. Delete completed tasks

**Key Concepts:**
- **Filtering:** Query data based on field values
- **Searching:** Full-text search across fields
- **Status Management:** Track item states

**Look at the code:** `themis_client.py` shows query construction:

```python
def get_todos(self, status_filter=None):
    url = f"{self.base_url}/api/v1/query"
    query = {
        "collection": "todos",
        "filter": {"status": status_filter} if status_filter else {}
    }
    response = requests.post(url, json=query)
    return response.json()
```

---

### Step 5: Master Full-Text Search (10 minutes)

The Contact Manager demonstrates advanced search:

```bash
cd ../03_contact_manager
pip install -r requirements.txt
python main.py
```

**Try this:**
1. Add multiple contacts with different details
2. Use the search box to find contacts by name, email, or phone
3. Categorize contacts (Friends, Family, Work)
4. Filter by category
5. Export contacts to JSON or CSV

**Key Concepts:**
- **Full-Text Search:** Search across multiple fields simultaneously
- **Categorization:** Organize data with tags/categories
- **Export/Import:** Data portability

---

### Step 6: Practice Exercise (10 minutes)

**Challenge:** Build a simple notes application

Using what you've learned, create a notes app that can:
1. Create notes with title and content
2. List all notes
3. Search notes by keyword
4. Delete notes

**Hints:**
- Use the Hello World example as a template
- Copy `themis_client.py` and modify the methods
- Add search using query filters from Todo App

**Solution:** Check out the Blog/Wiki example (`11_blog_wiki/`) for inspiration.

---

## 💡 Your First Hour

Ready for more advanced features? Let's explore vector search and time-series data.

### Step 7: Vector Search & Semantic Similarity (20 minutes)

Vector search enables semantic similarity - finding documents based on meaning, not just keywords.

```bash
cd ../07_vector_search_documents
pip install -r requirements.txt
python main.py
```

**What makes this special:**
- Traditional search: Finds "cat" only when you search for "cat"
- Vector search: Finds "cat", "kitten", "feline", "pet" when you search for "cat"

**Try this:**
1. Add several documents about different topics
2. Click "Generate Embeddings" to create vector representations
3. Search for a concept (e.g., "artificial intelligence")
4. See semantically similar documents, even without exact keyword matches

**Key Concepts:**
- **Embeddings:** Numerical representations of text meaning
- **Semantic Search:** Finding similar meanings, not just matching words
- **RAG (Retrieval Augmented Generation):** Foundation for AI applications

**Deep Dive:** Read [`VECTOR_SEARCH.md`](../examples/07_vector_search_documents/VECTOR_SEARCH.md) for implementation details.

---

### Step 8: Time-Series Data & Real-Time Monitoring (20 minutes)

Time-series data is crucial for monitoring, IoT, and analytics.

```bash
cd ../05_time_series_monitor
pip install -r requirements.txt
python main.py
```

**Try this:**
1. Start the sensor simulator
2. Watch real-time data flowing in
3. Observe live chart updates
4. Set alert thresholds
5. View historical data

**Key Concepts:**
- **Time-Series Storage:** Efficient storage of timestamped data
- **Real-Time Visualization:** Live chart updates
- **Alerts:** Threshold-based notifications
- **Historical Analysis:** Query data by time ranges

---

### Step 9: Graph Relationships (20 minutes)

Graph databases excel at representing and querying relationships.

```bash
cd ../06_graph_social_network
pip install -r requirements.txt
python main.py
```

**Try this:**
1. Create several users
2. Add friendships between users
3. Visualize the social graph
4. Find mutual friends
5. Detect communities

**Key Concepts:**
- **Nodes & Edges:** Users and relationships
- **Graph Queries:** Traverse relationships efficiently
- **Community Detection:** Find clusters in networks
- **Path Finding:** Shortest path between users

**Real-World Use Cases:**
- Social networks
- Supply chain tracking
- Fraud detection
- Recommendation systems

---

## 🏆 Your First Day

Now you're ready to build production-ready applications!

### Advanced Example: E-Commerce Catalog (60 minutes)

This multi-model example combines everything you've learned:

```bash
cd ../14_ecommerce_catalog
pip install -r requirements.txt
python main.py
```

**Features:**
- Product catalog (relational data)
- Product relationships (graph)
- Image search (vectors)
- Recommendations (ML)
- Reviews and ratings

**What you'll learn:**
- Multi-model data architecture
- Combining different data types
- Building production features
- Performance optimization

---

### Building Your Own Application

**Choose Your Adventure:**

#### For Web Applications
Build a blog, CMS, or dashboard:
- Start with [11 - Blog/Wiki](../examples/11_blog_wiki/)
- Add user management from [17 - CRM](../examples/17_crm/)
- Add real-time features from [18 - Chat](../examples/18_realtime_chat/)

#### For Data Analytics
Build monitoring or analytics:
- Start with [05 - Time Series](../examples/05_time_series_monitor/)
- Add IoT features from [09 - IoT Sensors](../examples/09_iot_sensor_network/)
- Scale with [Sharding Demo](../examples/sharding_demo.cpp)

#### For AI Applications
Build intelligent systems:
- Start with [07 - Vector Search](../examples/07_vector_search_documents/)
- Add LLM integration from [10 - Drone Analysis](../examples/10_drone_image_analysis/)
- Build recommendations with [19 - Rec Engine](../examples/19_recommendation_engine/)

---

## 🎓 Learning Paths by Role

### Web Developer Path

**Goal:** Build modern web applications with ThemisDB

**Week 1: Basics**
- Day 1: [01 - Hello World](../examples/01_hello_world/) + [02 - Todo App](../examples/02_todo_app/)
- Day 2: [03 - Contact Manager](../examples/03_contact_manager/)
- Day 3: [11 - Blog/Wiki](../examples/11_blog_wiki/)
- Day 4: [12 - Expense Tracker](../examples/12_expense_tracker/)
- Day 5: Practice project

**Week 2: Advanced**
- Day 1-2: [17 - CRM](../examples/17_crm/)
- Day 3-4: [18 - Real-Time Chat](../examples/18_realtime_chat/)
- Day 5: Personal project

**You'll Know:**
- REST API integration
- Real-time updates
- User management
- Business logic

---

### Data Engineer Path

**Goal:** Handle data pipelines and analytics

**Week 1: Foundations**
- Day 1: [01 - Hello World](../examples/01_hello_world/)
- Day 2: [05 - Time Series Monitor](../examples/05_time_series_monitor/)
- Day 3: [Data Retention Examples](../examples/adaptive_retention_example.cpp)
- Day 4: [Archive Pipeline](../examples/archive_pipeline.py)
- Day 5: Practice project

**Week 2: Scale & Performance**
- Day 1-2: [09 - IoT Sensor Network](../examples/09_iot_sensor_network/)
- Day 3: [Multi-SSD Config](../examples/example_multi_ssd_configuration.cpp)
- Day 4: [Sharding Demo](../examples/sharding_demo.cpp)
- Day 5: Performance tuning

**You'll Know:**
- Time-series handling
- Data retention strategies
- Scalability patterns
- Performance optimization

---

### ML Engineer Path

**Goal:** Build AI-powered applications

**Week 1: Embeddings & Search**
- Day 1: [01 - Hello World](../examples/01_hello_world/)
- Day 2-3: [07 - Vector Search](../examples/07_vector_search_documents/)
- Day 4: [Vector Encryption](../examples/example_vector_encryption.cpp)
- Day 5: Practice project

**Week 2: LLM Integration**
- Day 1-2: [10 - Drone Image Analysis](../examples/10_drone_image_analysis/)
- Day 3: [LLM Examples](../examples/embedded_llm_examples.cpp)
- Day 4: [19 - Recommendation Engine](../examples/19_recommendation_engine/)
- Day 5: AI project

**Week 3: Production**
- Build a complete RAG application
- Integrate with LLMs
- Deploy and monitor

**You'll Know:**
- Vector embeddings
- Semantic search
- LLM integration
- RAG architecture
- Model deployment

---

### Systems Architect Path

**Goal:** Design scalable, production systems

**Week 1: Architecture Patterns**
- Day 1: [01 - Hello World](../examples/01_hello_world/)
- Day 2: [04 - Inventory System](../examples/04_inventory_system/) (Multi-model)
- Day 3: [08 - DMS/ERP](../examples/08_dms_erp_system/) (Enterprise)
- Day 4: Study RBAC and workflows
- Day 5: Architecture design

**Week 2: Scalability**
- Day 1: [Sharding Demo](../examples/sharding_demo.cpp)
- Day 2: [Multi-SSD Config](../examples/example_multi_ssd_configuration.cpp)
- Day 3: [Hot Spare](../examples/hot_spare_example.cpp) + [Hot Reload](../examples/hot_reload_example.cpp)
- Day 4: [Optimization Tests](../examples/test_optimization_standalone.cpp)
- Day 5: Performance analysis

**Week 3: Production Readiness**
- Security: [Vector Encryption](../examples/example_vector_encryption.cpp), [AI Auditing](../examples/example_ai_auditing.cpp)
- Monitoring: [LLM Metrics](../examples/example_llm_metrics.cpp)
- Deployment: [Railway Example](../examples/railway/)

**You'll Know:**
- Multi-model architecture
- Scalability patterns
- High availability
- Security best practices
- Monitoring & observability

---

### IoT Developer Path

**Goal:** Build IoT and real-time systems

**Week 1: Real-Time Basics**
- Day 1: [01 - Hello World](../examples/01_hello_world/)
- Day 2: [05 - Time Series Monitor](../examples/05_time_series_monitor/)
- Day 3: [09 - IoT Sensor Network](../examples/09_iot_sensor_network/)
- Day 4: Practice project
- Day 5: Review

**Week 2: Advanced IoT**
- Day 1-2: [20 - Smart Home](../examples/20_smart_home/)
- Day 3: [Voice Assistant](../examples/voice_assistant_example.py)
- Day 4: Integration project
- Day 5: Testing & deployment

**You'll Know:**
- IoT data ingestion
- Real-time processing
- CEP (Complex Event Processing)
- Device management
- Automation rules

---

## 🛠️ Development Setup

### Prerequisites

**Required:**
- Python 3.8+ or C++ compiler (GCC 9+, Clang 10+, MSVC 2019+)
- Docker (for ThemisDB server)
- Git

**Optional:**
- .NET 8.0 SDK (for example 22)
- Node.js (for some tooling)

### IDE Setup

**VS Code (Recommended):**
```bash
# Install recommended extensions
code --install-extension ms-python.python
code --install-extension ms-vscode.cpptools
code --install-extension ms-dotnettools.csharp
```

**PyCharm / CLion:** Import the project and configure Python/C++ interpreters.

---

## 📚 Example Projects

### Beginner Project Ideas

**1. Personal Library Manager**
- Track books you've read
- Rate and review books
- Search by title, author, or genre
- Generate reading lists

**Technologies:** Hello World + Contact Manager patterns

---

**2. Simple Habit Tracker**
- Track daily habits
- Visualize progress
- Set goals and reminders
- Generate statistics

**Technologies:** Todo App + Time Series patterns

---

### Intermediate Project Ideas

**3. Recipe Recommendation System**
- Store recipes
- Rate and review
- Get recommendations based on preferences
- Search by ingredients

**Technologies:** Vector Search + Recommendation Engine

---

**4. Project Time Tracker**
- Track time on projects
- Generate reports
- Visualize time distribution
- Set budgets

**Technologies:** Time Series + Analytics

---

### Advanced Project Ideas

**5. Smart Document Search**
- Upload documents (PDF, Word, etc.)
- Extract text and generate embeddings
- Semantic search across all documents
- AI-powered summaries

**Technologies:** Vector Search + LLM Integration

---

**6. IoT Home Automation**
- Connect to sensors and devices
- Create automation rules
- Monitor energy usage
- Voice control integration

**Technologies:** IoT Sensors + Smart Home + Voice Assistant

---

## 🐛 Troubleshooting

### ThemisDB Server Issues

**Problem:** Connection refused
```
Error: Connection refused at localhost:8080
```

**Solution:**
```bash
# Check if container is running
docker ps | grep themisdb

# Check logs
docker logs themisdb

# Restart container
docker restart themisdb
```

---

**Problem:** Port already in use
```
Error: Port 8080 is already in use
```

**Solution:**
```bash
# Find process using port
lsof -i :8080

# Kill process or use different port
docker run -d --name themisdb -p 8081:8080 themisdb/themisdb:latest
```

---

### Python Issues

**Problem:** Tkinter not found
```
ModuleNotFoundError: No module named 'tkinter'
```

**Solution:**
```bash
# Ubuntu/Debian
sudo apt-get install python3-tk

# macOS (via Homebrew)
brew install python-tk

# Windows: Reinstall Python with Tcl/Tk option
```

---

**Problem:** Missing dependencies
```
ModuleNotFoundError: No module named 'requests'
```

**Solution:**
```bash
# Install all requirements
pip install -r requirements.txt

# Or install individually
pip install requests themisdb-client
```

---

### C++ Compilation Issues

**Problem:** Compiler not found
```
Error: g++: command not found
```

**Solution:**
```bash
# Ubuntu/Debian
sudo apt-get install build-essential

# macOS
xcode-select --install

# Windows
# Install Visual Studio 2019+ or MinGW
```

---

### Performance Issues

**Problem:** Slow queries

**Solutions:**
1. **Add indexes:**
   ```python
   # Create index on frequently queried fields
   client.create_index("users", "email")
   ```

2. **Use batch operations:**
   ```python
   # Instead of individual inserts
   client.batch_insert(users_list)
   ```

3. **Limit result sets:**
   ```python
   # Add limit to queries
   query = {"limit": 100}
   ```

4. **Check server resources:**
   ```bash
   docker stats themisdb
   ```

---

### Data Issues

**Problem:** Corrupted data

**Solution:**
```bash
# Backup first
docker exec themisdb /backup.sh

# Then try repair
docker exec themisdb /repair.sh
```

---

## 📖 Next Steps

### Dive Deeper

**Documentation:**
- [Examples Index](EXAMPLES_INDEX.md) - All examples
- [API Reference](api/API_REFERENCE.md) - Complete API docs
- [Architecture](../docs/architecture/) - System design

**Features:**
- [AQL Query Language](../docs/aql/) - Advanced queries
- [Vector Search Guide](../examples/07_vector_search_documents/VECTOR_SEARCH.md)
- [LLM Setup Guide](../docs/de/guides/LLM_COMPLETE_SETUP_GUIDE.md)

---

### Join the Community

**Get Help:**
- [GitHub Issues](https://github.com/makr-code/ThemisDB/issues)
- [Discussions](https://github.com/makr-code/ThemisDB/discussions)
- [Documentation](https://makr-code.github.io/ThemisDB/)

**Contribute:**
- Fix bugs
- Add examples
- Improve docs
- Share projects

See [CONTRIBUTING.md](../CONTRIBUTING.md)

---

### Keep Learning

**Practice:**
- Build personal projects
- Contribute to open source
- Join coding challenges

**Stay Updated:**
- Watch the repo for updates
- Read the changelog
- Follow releases

---

## 🎉 Congratulations!

You've completed the ThemisDB Quickstart Guide! You now know:

✅ How to run ThemisDB  
✅ Basic CRUD operations  
✅ Querying and filtering  
✅ Vector search & embeddings  
✅ Time-series data  
✅ Graph relationships  
✅ Building real applications  

**What's Next?**
1. Pick a [Learning Path](#-learning-paths-by-role)
2. Build a personal project
3. Explore the [Examples Index](EXAMPLES_INDEX.md)
4. Read the [API docs](api/API_REFERENCE.md)

**Happy Building! 🚀**

---

**Last Updated:** 2026-05  
**Version:** 1.9.0-beta  
**Feedback:** [Open an issue](https://github.com/makr-code/ThemisDB/issues/new) with your suggestions
