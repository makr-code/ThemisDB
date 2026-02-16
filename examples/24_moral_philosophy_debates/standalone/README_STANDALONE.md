# Standalone Moral Philosophy Dialectic Engine

A self-contained implementation of moral philosophical debates using SQLite for storage and Ollama for local LLM inference. This system is completely independent of ThemisDB and provides a lightweight solution for generating philosophical discussions on ethical questions.

## Features

✅ **SQLite Storage** - Local database instead of ThemisDB  
✅ **Ollama Integration** - Local LLM inference without cloud dependencies  
✅ **5 Philosophy Schools** - Kant, Utilitarianism, Virtue Ethics, Socratic, Stoicism  
✅ **4 Argument Dimensions** - Moral, Ethical, Social, Political  
✅ **Two-Round Debates** - Initial statements + counter-arguments  
✅ **Markdown Export** - Save debates in readable format  
✅ **Persistent Storage** - All debates saved in SQLite database  
✅ **Fully Standalone** - No external services required

## Philosophy Schools

### 1. Immanuel Kant (Kantian Ethics)
- **Core Principle**: Categorical Imperative
- **Focus**: Duty, universal moral laws, human dignity
- **Approach**: Actions judged by whether their maxims can be universalized

### 2. John Stuart Mill (Utilitarianism)
- **Core Principle**: Greatest Happiness Principle
- **Focus**: Consequences, utility maximization
- **Approach**: Actions judged by their contribution to overall happiness

### 3. Aristotle (Virtue Ethics)
- **Core Principle**: Eudaimonia (human flourishing)
- **Focus**: Character, virtues, the golden mean
- **Approach**: Actions judged by whether they reflect virtuous character

### 4. Socrates (Socratic Method)
- **Core Principle**: Dialectical questioning
- **Focus**: Logical consistency, examining assumptions
- **Approach**: Truth through dialogue and questioning

### 5. Epictetus (Stoicism)
- **Core Principle**: Dichotomy of control
- **Focus**: Virtue, acceptance, living according to nature
- **Approach**: Focus on what we can control, accept what we cannot

## Installation

### 1. Install Ollama

Download and install Ollama from [https://ollama.ai](https://ollama.ai)

Start the Ollama service:
```bash
ollama serve
```

### 2. Download LLM Model

Pull the required model (llama3.2 recommended):
```bash
ollama pull llama3.2
```

Other compatible models:
- `llama3.2` (recommended, 3B parameters)
- `llama2` (7B parameters)
- `mistral` (7B parameters)
- `phi` (3B parameters)

### 3. Install Python Dependencies

```bash
cd examples/24_moral_philosophy_debates/standalone
pip install -r requirements_standalone.txt
```

## Usage

### Basic Usage

Run with default settings (AI in Medicine debate):
```bash
python standalone_moral_dialectic.py
```

### Custom Topics

Specify your own topic and ethical question:
```bash
python standalone_moral_dialectic.py \
  --topic "Climate Change" \
  --question "Is geo-engineering ethically justifiable?"
```

### Export to Markdown

Save the debate to a markdown file:
```bash
python standalone_moral_dialectic.py \
  --topic "Autonomous Vehicles" \
  --question "Who is responsible when a self-driving car causes harm?" \
  --export debate_output.md
```

### Use Different Model

Specify a different Ollama model:
```bash
python standalone_moral_dialectic.py --model mistral
```

### Custom Database

Use a different database file:
```bash
python standalone_moral_dialectic.py --db my_debates.db
```

## Command-Line Arguments

| Argument | Default | Description |
|----------|---------|-------------|
| `--topic` | "Künstliche Intelligenz in der Medizin" | Topic for debate |
| `--question` | "Sollte eine KI über Leben und Tod entscheiden dürfen?" | Ethical question |
| `--model` | "llama3.2" | Ollama model name |
| `--db` | "moral_debates.db" | SQLite database path |
| `--export` | None | Export debate to markdown file |

## How It Works

### Architecture

```
┌─────────────────────────────────────────────────┐
│  Standalone Moral Dialectic Engine              │
├─────────────────────────────────────────────────┤
│                                                 │
│  ┌──────────────┐      ┌──────────────┐       │
│  │   Ollama     │◄─────┤  LLM Backend │       │
│  │   Server     │      │              │       │
│  └──────────────┘      └──────────────┘       │
│                               │                │
│                               ▼                │
│  ┌──────────────────────────────────┐         │
│  │  MoralDialecticEngine            │         │
│  │  - Generate statements           │         │
│  │  - Generate counter-arguments    │         │
│  │  - Orchestrate debate rounds     │         │
│  └──────────────────────────────────┘         │
│                               │                │
│                               ▼                │
│  ┌──────────────────────────────────┐         │
│  │  SQLiteDebateStore               │         │
│  │  - debates table                 │         │
│  │  - messages table                │         │
│  └──────────────────────────────────┘         │
│                                                 │
└─────────────────────────────────────────────────┘
```

### Debate Flow

1. **Initialization**
   - Create SQLite database (if not exists)
   - Connect to Ollama server
   - Initialize debate engine

2. **Round 1: Initial Statements**
   - Each philosophy generates statements for each dimension
   - Example: Kant provides moral and ethical perspectives
   - All statements saved to database

3. **Round 2: Counter-Arguments**
   - Each philosophy responds to an opposing viewpoint
   - Randomly selects target statement to counter
   - Generates respectful critique from own perspective

4. **Storage & Export**
   - All messages saved to SQLite with timestamps
   - Debate session marked as completed
   - Optional markdown export for sharing

### Database Schema

**debates table:**
```sql
CREATE TABLE debates (
    id TEXT PRIMARY KEY,
    topic TEXT NOT NULL,
    ethical_question TEXT NOT NULL,
    created_at TEXT NOT NULL,
    completed_at TEXT,
    data TEXT NOT NULL  -- Full JSON serialization
)
```

**messages table:**
```sql
CREATE TABLE messages (
    id TEXT PRIMARY KEY,
    debate_id TEXT NOT NULL,
    philosophy_school TEXT NOT NULL,
    message_type TEXT NOT NULL,
    dimension TEXT NOT NULL,
    content TEXT NOT NULL,
    responds_to TEXT,  -- ID of message being responded to
    timestamp TEXT NOT NULL,
    FOREIGN KEY (debate_id) REFERENCES debates(id)
)
```

## Example Output

```markdown
# Moral Philosophy Debate

**Topic:** Künstliche Intelligenz in der Medizin
**Question:** Sollte eine KI über Leben und Tod entscheiden dürfen?

## Round 1: Initial Statements

### Immanuel Kant (moral)
From the standpoint of the categorical imperative, we must ask: can we 
universalize a maxim that allows machines to decide life and death? Such 
delegation violates human dignity, treating patients as means rather than 
ends in themselves. Medical decisions require moral autonomy and the 
recognition of each person's inherent worth—qualities that artificial 
systems fundamentally lack...

### John Stuart Mill (moral)
The utilitarian calculus must weigh potential benefits against harms. If 
AI systems can make faster, more accurate diagnoses and treatment decisions, 
they may maximize overall welfare. However, we must carefully consider 
the quality of outcomes, not just quantity...

## Round 2: Counter-Arguments

### Immanuel Kant (Counter-Argument)
While the utilitarian raises important points about outcomes, this approach 
fundamentally misunderstands the nature of moral duty. We cannot sacrifice 
individual dignity for aggregate utility...
```

## Configuration

The `config.yaml` file contains default settings:

```yaml
database:
  path: "moral_debates.db"

ollama:
  base_url: "http://localhost:11434"
  model: "llama3.2"
  temperature: 0.7
  max_tokens: 800

debate:
  default_philosophies:
    - kant
    - utilitarianism
    - virtue_ethics
    - socratic
    - stoicism
  default_dimensions:
    - moral
    - ethical
    - social
    - political
  rounds: 2
```

## Comparison with Original ThemisDB Version

| Feature | Original | Standalone |
|---------|----------|------------|
| Database | ThemisDB (Multi-Model) | SQLite |
| LLM Backend | SimpleLLMBackend (placeholder) | OllamaBackend (fully implemented) |
| Dependencies | ThemisDB server required | Only Ollama required |
| Setup Complexity | High | Low |
| Portability | Limited | High |
| External Services | Required | None |

## Troubleshooting

### Ollama Not Available

If you see: `Error: Ollama not available!`

1. Check if Ollama is running:
   ```bash
   curl http://localhost:11434/api/tags
   ```

2. Start Ollama if not running:
   ```bash
   ollama serve
   ```

3. Verify the model is installed:
   ```bash
   ollama list
   ```

### Slow Generation

If responses are slow:

1. Use a smaller model:
   ```bash
   python standalone_moral_dialectic.py --model phi
   ```

2. Reduce dimensions (edit code to use fewer dimensions)

3. Check system resources (Ollama needs sufficient RAM)

### Database Errors

If you encounter database errors:

1. Check file permissions on the database file
2. Try deleting the database file to start fresh
3. Ensure SQLite3 is installed (usually built into Python)

## Requirements

- **Python**: 3.8 or higher
- **SQLite**: 3.x (built-in with Python)
- **Ollama**: 0.1.0 or higher
- **System RAM**: Minimum 8GB (16GB recommended for larger models)
- **Platform**: Linux, macOS, Windows

## Future Enhancements

Potential improvements for future versions:

- [ ] YAML config parsing for runtime configuration
- [ ] HTML export in addition to markdown
- [ ] Round 3: Synthesis and consensus-finding
- [ ] Interactive CLI mode for user questions
- [ ] Vector embeddings for semantic search
- [ ] GUI interface (tkinter-based)
- [ ] Support for additional philosophy schools
- [ ] Multi-language support
- [ ] Export to PDF format

## License

This software is part of the ThemisDB project. See the main repository for license information.

## Contributing

Contributions are welcome! Please:

1. Test thoroughly with various ethical scenarios
2. Ensure Ollama compatibility
3. Follow existing code style
4. Update documentation as needed

## Support

For issues and questions:

- Open an issue in the main ThemisDB repository
- Check Ollama documentation: https://ollama.ai/docs
- Review the SQLite documentation: https://www.sqlite.org/docs.html

---

*Standalone Moral Philosophy Dialectic Engine - Part of the ThemisDB Moral Philosophy Debates Suite*
