> **Hinweis:** Inhalt mit aktuellem Modulcode und -stand abgleichen.

# GUI Screenshot (Text Representation)

```
╔══════════════════════════════════════════════════════════════════════════════╗
║  Standalone Moral Dialectic - Philosophy Chat                          [_][□][X]║
╠══════════════════════════════════════════════════════════════════════════════╣
║                                                                              ║
║  ┌─ Debate Topic ──────────────────────────────────────────────────────┐   ║
║  │ URL/Topic:      [Künstliche Intelligenz in der Medizin           ]  │   ║
║  │ Ethical Question: [Sollte eine KI über Leben und Tod entscheid...] │   ║
║  │                                                 [Start Debate]       │   ║
║  └──────────────────────────────────────────────────────────────────────┘   ║
║                                                                              ║
║  ⭐ Toolbar: [New Debate] [Load Previous] [Export to Markdown]         ⭐   ║
║                                                                              ║
║  ┌─ Select Philosophers ────────────────────────────────────────────────┐   ║
║  │ [✓] Immanuel Kant  [✓] John Stuart Mill  [✓] Aristotle              │   ║
║  │ [✓] Socrates       [✓] Epictetus                                     │   ║
║  └──────────────────────────────────────────────────────────────────────┘   ║
║                                                                              ║
║  ┌─ Debate Chat ────────────────────────────────────────────────────────┐   ║
║  │                                                                       │↑  ║
║  │ ══════════════════════════════════════════════════════════════════   │█  ║
║  │ [12:35:42] Immanuel Kant (moral perspective):                        │█  ║
║  │ From my perspective, we must apply the categorical imperative...     │█  ║
║  │                                                                       │█  ║
║  │ ──────────────────────────────────────────────────────────────────   │█  ║
║  │ [12:36:15] John Stuart Mill (moral perspective):                     │█  ║
║  │ The utilitarian calculus requires us to weigh consequences...        │█  ║
║  │                                                                       │█  ║
║  │ ──────────────────────────────────────────────────────────────────   │█  ║
║  │ [12:37:03] User (moral consideration):                               │█  ║
║  │ But what about the practical implications for hospitals?             │█  ║
║  │                                                                       │█  ║
║  │ ──────────────────────────────────────────────────────────────────   │█  ║
║  │ [12:37:45] Aristotle (counter-argument):                             │█  ║
║  │ Your practical concern aligns with phronesis - practical wisdom...   │█  ║
║  │                                                                       │↓  ║
║  └──────────────────────────────────────────────────────────────────────┘   ║
║                                                                              ║
║  ┌─ Your Moral Consideration ───────────────────────────────────────────┐   ║
║  │ [Type your moral consideration here...                           ]   │   ║
║  │ [                                                                 ]   │   ║
║  │ [Press Ctrl+Enter to send                                        ]   │   ║
║  │                                                     [Send Message]    │   ║
║  └──────────────────────────────────────────────────────────────────────┘   ║
║                                                                              ║
║  Status: Ready - 12 messages                                                 ║
╚══════════════════════════════════════════════════════════════════════════════╝
```

## Color Scheme (in actual GUI):

- **Immanuel Kant**: Blue text (#4A90E2)
- **John Stuart Mill**: Green text (#50C878)
- **Aristotle**: Purple text (#9B59B6)
- **Socrates**: Orange text (#E67E22)
- **Epictetus**: Gray text (#95A5A6)
- **User**: Red text (#E74C3C)

## Key Features Visible:

1. **Top Section**: Topic and question input fields with Start Debate button
2. **Toolbar**: New Debate, Load Previous, Export to Markdown buttons ⭐ NEW
3. **Philosophy Selection**: Checkboxes for selecting participating philosophers
4. **Chat Display**: Scrollable area with color-coded messages, timestamps, and separators
5. **User Input**: Multi-line text field with Send Message button
6. **Status Bar**: Shows current status and message count

## User Flow:

1. User enters topic/question → Clicks Start Debate
2. Philosophers engage in Round 1 (statements) and Round 2 (counter-arguments)
3. Chat display fills with color-coded philosophical arguments
4. User types their own moral consideration
5. Presses Ctrl+Enter or clicks Send Message
6. All selected philosophers respond to the user's point
7. **NEW**: User can export debate with "Export to Markdown" button
8. **NEW**: User can load previous debates with "Load Previous" button
9. **NEW**: User can start fresh with "New Debate" button
10. Conversation continues as long as user wants

## Technical Features:

- **Threading**: All LLM generation happens in background threads
- **Non-blocking UI**: GUI remains responsive during debate generation
- **Real-time Updates**: Messages appear as they're generated
- **Persistent Storage**: All debates saved to SQLite automatically
- **Status Updates**: Status bar shows what's happening
- **Error Handling**: Graceful handling of Ollama unavailability
