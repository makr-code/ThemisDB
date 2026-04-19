> **Hinweis:** Workflow-Spezifikation gegen aktuellen Sourcecode verifizieren.

# Workflow-Design - DMS/ERP System

## 📋 Übersicht

Dieses Dokument beschreibt das Workflow-System des DMS/ERP-Systems, das auf Graph-basierten State Machines und einer flexiblen Rule Engine basiert.

## 🔄 Workflow-Konzepte

### State Machine Grundlagen

Workflows sind **gerichtete Graphen** mit folgenden Elementen:

```python
# Workflow-Definition
{
    "id": "workflow_uuid",
    "name": "Rechnungsgenehmigung",
    "version": "1.2.0",
    "states": [
        {
            "id": "draft",
            "name": "Entwurf",
            "type": "start",
            "allowed_roles": ["employee"],
            "actions": ["submit", "save_draft", "delete"]
        },
        {
            "id": "review",
            "name": "Prüfung",
            "type": "intermediate",
            "allowed_roles": ["manager"],
            "actions": ["approve", "reject", "request_changes"],
            "timeout": 86400,  # 24 Stunden
            "escalation": "director"
        },
        {
            "id": "approved",
            "name": "Genehmigt",
            "type": "end",
            "allowed_roles": ["system"],
            "actions": ["archive"]
        },
        {
            "id": "rejected",
            "name": "Abgelehnt",
            "type": "end",
            "allowed_roles": ["system"],
            "actions": ["archive"]
        }
    ],
    "transitions": [
        {
            "from": "draft",
            "to": "review",
            "action": "submit",
            "conditions": ["has_all_required_fields", "valid_amount"],
            "pre_hooks": ["validate_document", "extract_metadata"],
            "post_hooks": ["notify_manager", "log_transition"]
        },
        {
            "from": "review",
            "to": "approved",
            "action": "approve",
            "conditions": ["manager_authorized"],
            "post_hooks": ["notify_submitter", "trigger_payment_process"]
        },
        {
            "from": "review",
            "to": "rejected",
            "action": "reject",
            "conditions": ["rejection_reason_provided"],
            "post_hooks": ["notify_submitter"]
        }
    ]
}
```

### Workflow-Typen

#### 1. Sequential Workflow

Lineare Abfolge von Schritten:

```
Entwurf → Prüfung → Genehmigung → Archivierung
```

**Verwendung:**
- Einfache Genehmigungsprozesse
- Dokumenten-Reviews
- Standard-Bestellungen

**Code-Beispiel:**
```python
def create_sequential_workflow(name: str, steps: List[str]) -> dict:
    """Erstellt einen sequentiellen Workflow."""
    states = []
    transitions = []
    
    for i, step in enumerate(steps):
        state = {
            "id": f"step_{i}",
            "name": step,
            "type": "start" if i == 0 else ("end" if i == len(steps)-1 else "intermediate")
        }
        states.append(state)
        
        if i < len(steps) - 1:
            transition = {
                "from": f"step_{i}",
                "to": f"step_{i+1}",
                "action": "proceed"
            }
            transitions.append(transition)
    
    return {"name": name, "states": states, "transitions": transitions}

# Verwendung
workflow = create_sequential_workflow(
    "Simple Approval",
    ["Draft", "Review", "Approval", "Archive"]
)
```

#### 2. Parallel Workflow

Mehrere Schritte gleichzeitig:

```
         ┌─→ Manager A ─┐
Entwurf ─┼─→ Manager B ─┼─→ Zusammenführung → Freigabe
         └─→ Manager C ─┘
```

**Verwendung:**
- Multi-Approver Prozesse
- Parallele Qualitätsprüfungen
- Team-Reviews

**Synchronisations-Strategien:**
```python
class ParallelSync:
    """Strategien für parallele Workflow-Ausführung."""
    
    @staticmethod
    def all_must_approve(approvals: List[bool]) -> bool:
        """Alle Teilnehmer müssen zustimmen."""
        return all(approvals)
    
    @staticmethod
    def majority_must_approve(approvals: List[bool]) -> bool:
        """Mehrheit muss zustimmen."""
        return sum(approvals) > len(approvals) / 2
    
    @staticmethod
    def any_can_approve(approvals: List[bool]) -> bool:
        """Ein Teilnehmer reicht."""
        return any(approvals)
    
    @staticmethod
    def weighted_voting(
        approvals: Dict[str, bool],
        weights: Dict[str, float]
    ) -> bool:
        """Gewichtete Abstimmung."""
        total_weight = sum(weights.values())
        approval_weight = sum(
            weights[user] for user, approved in approvals.items()
            if approved
        )
        return approval_weight > total_weight / 2
```

#### 3. Conditional Workflow

Verzweigungen basierend auf Bedingungen:

```
                    ┌─→ Schnellverfahren (< 1000€)
Entwurf → Prüfung ─┤
                    └─→ Standardverfahren (≥ 1000€) → Direktor
```

**Beispiel:**
```python
class ConditionalWorkflow:
    """Bedingte Workflow-Logik."""
    
    @staticmethod
    def evaluate_condition(
        document: dict,
        condition: dict
    ) -> bool:
        """Evaluiert eine Workflow-Bedingung."""
        condition_type = condition["type"]
        
        if condition_type == "amount_threshold":
            amount = document.get("metadata", {}).get("amount", 0)
            threshold = condition["value"]
            operator = condition["operator"]  # <, >, <=, >=, ==
            
            operators = {
                "<": lambda a, b: a < b,
                ">": lambda a, b: a > b,
                "<=": lambda a, b: a <= b,
                ">=": lambda a, b: a >= b,
                "==": lambda a, b: a == b
            }
            return operators[operator](amount, threshold)
        
        elif condition_type == "document_type":
            doc_type = document.get("type")
            allowed_types = condition["values"]
            return doc_type in allowed_types
        
        elif condition_type == "user_role":
            user_role = document.get("owner_role")
            required_role = condition["value"]
            return user_role == required_role
        
        elif condition_type == "custom_rule":
            # Führt Python-Code aus (sicher sandboxed)
            rule_code = condition["code"]
            return eval(rule_code, {"document": document})
        
        return False
    
    @staticmethod
    def determine_next_state(
        current_state: str,
        document: dict,
        transitions: List[dict]
    ) -> str:
        """Bestimmt den nächsten Zustand basierend auf Bedingungen."""
        for transition in transitions:
            if transition["from"] != current_state:
                continue
            
            conditions = transition.get("conditions", [])
            if all(
                ConditionalWorkflow.evaluate_condition(document, cond)
                for cond in conditions
            ):
                return transition["to"]
        
        raise ValueError(f"No valid transition from {current_state}")
```

## 🎯 Approval-Prozesse

### Single Approver

Einfachster Fall - eine Person genehmigt:

```python
class SingleApprover:
    """Single Approver Workflow."""
    
    def __init__(self, themis_client):
        self.client = themis_client
    
    async def submit_for_approval(
        self,
        document_id: str,
        approver_id: str
    ) -> dict:
        """Dokument zur Genehmigung einreichen."""
        # Update workflow state
        await self.client.update(
            "documents",
            document_id,
            {
                "workflow_state": "pending_approval",
                "assigned_to": approver_id,
                "submitted_at": datetime.now().isoformat()
            }
        )
        
        # Create task for approver
        task = {
            "type": "approval_request",
            "document_id": document_id,
            "assigned_to": approver_id,
            "due_date": (datetime.now() + timedelta(days=3)).isoformat(),
            "priority": "normal"
        }
        await self.client.create("tasks", task)
        
        return {"status": "submitted", "approver": approver_id}
    
    async def approve(self, document_id: str, approver_id: str) -> dict:
        """Dokument genehmigen."""
        doc = await self.client.get("documents", document_id)
        
        if doc["assigned_to"] != approver_id:
            raise PermissionError("Not authorized to approve")
        
        await self.client.update(
            "documents",
            document_id,
            {
                "workflow_state": "approved",
                "approved_by": approver_id,
                "approved_at": datetime.now().isoformat()
            }
        )
        
        return {"status": "approved"}
```

### Multi-Level Approval

Hierarchische Genehmigung mit mehreren Stufen:

```python
class MultiLevelApproval:
    """Multi-Level Approval Workflow."""
    
    def __init__(self, themis_client):
        self.client = themis_client
    
    async def get_approval_hierarchy(
        self,
        document: dict
    ) -> List[str]:
        """Bestimmt die Genehmigungshierarchie basierend auf Dokumentwert."""
        amount = document.get("metadata", {}).get("amount", 0)
        
        if amount < 1000:
            return ["manager"]
        elif amount < 10000:
            return ["manager", "director"]
        elif amount < 100000:
            return ["manager", "director", "cfo"]
        else:
            return ["manager", "director", "cfo", "ceo"]
    
    async def advance_to_next_level(
        self,
        document_id: str,
        current_level: int
    ) -> dict:
        """Fortschritt zur nächsten Genehmigungsstufe."""
        doc = await self.client.get("documents", document_id)
        hierarchy = await self.get_approval_hierarchy(doc)
        
        if current_level >= len(hierarchy):
            # Letzte Stufe erreicht - genehmigt
            await self.client.update(
                "documents",
                document_id,
                {
                    "workflow_state": "fully_approved",
                    "completed_at": datetime.now().isoformat()
                }
            )
            return {"status": "completed"}
        
        # Nächste Stufe
        next_role = hierarchy[current_level]
        next_approver = await self._find_user_with_role(next_role)
        
        await self.client.update(
            "documents",
            document_id,
            {
                "workflow_state": f"pending_{next_role}",
                "current_level": current_level + 1,
                "assigned_to": next_approver
            }
        )
        
        return {
            "status": "advanced",
            "level": current_level + 1,
            "approver": next_approver
        }
```

### Consensus Approval

Alle müssen zustimmen:

```python
class ConsensusApproval:
    """Consensus-basierte Genehmigung."""
    
    def __init__(self, themis_client):
        self.client = themis_client
    
    async def request_consensus(
        self,
        document_id: str,
        approvers: List[str]
    ) -> dict:
        """Startet Consensus-Genehmigungsprozess."""
        # Erstelle Approval-Requests für alle
        approval_requests = []
        for approver in approvers:
            request = {
                "document_id": document_id,
                "approver_id": approver,
                "status": "pending",
                "created_at": datetime.now().isoformat()
            }
            result = await self.client.create("approval_requests", request)
            approval_requests.append(result["id"])
        
        # Update Dokument
        await self.client.update(
            "documents",
            document_id,
            {
                "workflow_state": "consensus_pending",
                "approval_requests": approval_requests,
                "required_approvals": len(approvers),
                "received_approvals": 0
            }
        )
        
        return {"status": "consensus_started", "approvers": len(approvers)}
    
    async def record_approval(
        self,
        document_id: str,
        approver_id: str,
        approved: bool
    ) -> dict:
        """Zeichnet eine einzelne Genehmigung auf."""
        doc = await self.client.get("documents", document_id)
        
        # Update approval request
        requests = await self.client.query(
            "approval_requests",
            {
                "document_id": document_id,
                "approver_id": approver_id
            }
        )
        
        if not requests:
            raise ValueError("Approval request not found")
        
        request = requests[0]
        await self.client.update(
            "approval_requests",
            request["id"],
            {
                "status": "approved" if approved else "rejected",
                "decided_at": datetime.now().isoformat()
            }
        )
        
        # Bei Ablehnung ist Consensus gescheitert
        if not approved:
            await self.client.update(
                "documents",
                document_id,
                {
                    "workflow_state": "consensus_failed",
                    "rejected_by": approver_id
                }
            )
            return {"status": "consensus_failed"}
        
        # Zähle Genehmigungen
        approved_count = doc["received_approvals"] + 1
        await self.client.update(
            "documents",
            document_id,
            {"received_approvals": approved_count}
        )
        
        # Check ob Consensus erreicht
        if approved_count >= doc["required_approvals"]:
            await self.client.update(
                "documents",
                document_id,
                {
                    "workflow_state": "consensus_approved",
                    "completed_at": datetime.now().isoformat()
                }
            )
            return {"status": "consensus_achieved"}
        
        return {
            "status": "approval_recorded",
            "progress": f"{approved_count}/{doc['required_approvals']}"
        }
```

## ⏰ Timeouts & Eskalation

### Timeout-Handling

```python
class WorkflowTimeout:
    """Timeout-Management für Workflows."""
    
    def __init__(self, themis_client):
        self.client = themis_client
    
    async def check_timeouts(self):
        """Prüft alle laufenden Workflows auf Timeouts."""
        now = datetime.now()
        
        # Finde Dokumente mit ausstehenden Genehmigungen
        pending_docs = await self.client.query(
            "documents",
            {
                "workflow_state": {"$in": [
                    "pending_approval",
                    "pending_manager",
                    "pending_director"
                ]}
            }
        )
        
        for doc in pending_docs:
            submitted_at = datetime.fromisoformat(doc["submitted_at"])
            timeout = doc.get("timeout_seconds", 86400)  # 24h default
            
            if (now - submitted_at).total_seconds() > timeout:
                await self._handle_timeout(doc)
    
    async def _handle_timeout(self, document: dict):
        """Behandelt Timeout für ein Dokument."""
        escalation_policy = document.get("escalation_policy", "notify")
        
        if escalation_policy == "notify":
            # Erinnerung an aktuellen Genehmiger
            await self._send_reminder(
                document["assigned_to"],
                document["id"]
            )
        
        elif escalation_policy == "escalate":
            # Eskaliere an nächsthöhere Ebene
            await self._escalate_to_supervisor(document)
        
        elif escalation_policy == "auto_approve":
            # Automatische Genehmigung bei Timeout
            await self._auto_approve(document)
        
        elif escalation_policy == "auto_reject":
            # Automatische Ablehnung
            await self._auto_reject(document)
```

### Eskalations-Mechanismen

```python
class EscalationManager:
    """Verwaltet Workflow-Eskalationen."""
    
    def __init__(self, themis_client):
        self.client = themis_client
    
    async def escalate(
        self,
        document_id: str,
        reason: str
    ) -> dict:
        """Eskaliert ein Dokument."""
        doc = await self.client.get("documents", document_id)
        current_assignee = doc["assigned_to"]
        
        # Finde Vorgesetzten
        supervisor = await self._get_supervisor(current_assignee)
        
        if not supervisor:
            # Keine weitere Eskalation möglich
            return {"status": "escalation_failed", "reason": "no_supervisor"}
        
        # Update Dokument
        await self.client.update(
            "documents",
            document_id,
            {
                "workflow_state": "escalated",
                "previous_assignee": current_assignee,
                "assigned_to": supervisor,
                "escalated_at": datetime.now().isoformat(),
                "escalation_reason": reason
            }
        )
        
        # Log Eskalation
        await self._log_escalation(document_id, current_assignee, supervisor, reason)
        
        # Benachrichtigungen
        await self._notify_escalation(current_assignee, supervisor, document_id)
        
        return {
            "status": "escalated",
            "from": current_assignee,
            "to": supervisor
        }
    
    async def _get_supervisor(self, user_id: str) -> Optional[str]:
        """Ermittelt Vorgesetzten eines Users."""
        user = await self.client.get("users", user_id)
        return user.get("supervisor_id")
```

## 🔧 Workflow Engine Implementation

### Core Engine

```python
class WorkflowEngine:
    """Haupt-Workflow-Engine."""
    
    def __init__(self, themis_client):
        self.client = themis_client
        self.state_handlers = {}
        self.transition_hooks = {}
    
    def register_state_handler(
        self,
        state: str,
        handler: Callable
    ):
        """Registriert Handler für einen State."""
        self.state_handlers[state] = handler
    
    def register_transition_hook(
        self,
        from_state: str,
        to_state: str,
        hook: Callable,
        when: str = "post"  # pre, post
    ):
        """Registriert Hook für eine Transition."""
        key = f"{from_state}->{to_state}"
        if key not in self.transition_hooks:
            self.transition_hooks[key] = {"pre": [], "post": []}
        self.transition_hooks[key][when].append(hook)
    
    async def execute_action(
        self,
        document_id: str,
        action: str,
        user_id: str,
        **kwargs
    ) -> dict:
        """Führt eine Workflow-Action aus."""
        # Lade Dokument und Workflow-Definition
        doc = await self.client.get("documents", document_id)
        workflow = await self._load_workflow(doc["workflow_id"])
        
        current_state = doc["workflow_state"]
        
        # Finde passende Transition
        transition = self._find_transition(
            workflow,
            current_state,
            action
        )
        
        if not transition:
            raise ValueError(f"Invalid action '{action}' for state '{current_state}'")
        
        # Prüfe Berechtigungen
        if not await self._check_permissions(user_id, transition):
            raise PermissionError("User not authorized for this action")
        
        # Pre-Hooks ausführen
        await self._execute_hooks(
            transition,
            "pre",
            document_id,
            **kwargs
        )
        
        # State-Transition durchführen
        new_state = transition["to"]
        await self.client.update(
            "documents",
            document_id,
            {
                "workflow_state": new_state,
                "last_transition": {
                    "from": current_state,
                    "to": new_state,
                    "action": action,
                    "user": user_id,
                    "timestamp": datetime.now().isoformat()
                }
            }
        )
        
        # Post-Hooks ausführen
        await self._execute_hooks(
            transition,
            "post",
            document_id,
            **kwargs
        )
        
        # State-Handler ausführen
        if new_state in self.state_handlers:
            await self.state_handlers[new_state](document_id, **kwargs)
        
        return {
            "status": "success",
            "previous_state": current_state,
            "new_state": new_state
        }
    
    async def _execute_hooks(
        self,
        transition: dict,
        when: str,
        document_id: str,
        **kwargs
    ):
        """Führt Transition-Hooks aus."""
        hooks = transition.get(f"{when}_hooks", [])
        for hook_name in hooks:
            hook = self._get_hook_function(hook_name)
            await hook(document_id, **kwargs)
```

### Best Practices

1. **Workflow-Versionierung**
   - Workflows versionieren (v1.0, v1.1, etc.)
   - Alte Instanzen mit alter Version weiterlaufen lassen
   - Neue Instanzen nutzen neue Version

2. **Idempotenz**
   - Actions sollten idempotent sein
   - Mehrfache Ausführung hat gleichen Effekt

3. **Transaktionen**
   - State-Transitions in Transaktionen wrappen
   - Rollback bei Fehlern

4. **Monitoring**
   - Workflow-Metriken sammeln (Dauer, Success-Rate)
   - Bottlenecks identifizieren

5. **Testing**
   - Unit-Tests für State-Transitions
   - Integration-Tests für komplette Workflows
   - Chaos-Engineering für Fehlerszenarien

## 📊 Workflow-Visualisierung

```python
def visualize_workflow(workflow: dict) -> str:
    """Generiert Mermaid-Diagramm für Workflow."""
    lines = ["graph TD"]
    
    # States
    for state in workflow["states"]:
        shape = "([{}])" if state["type"] == "start" else \
                "{{{}}" if state["type"] == "end" else \
                "[{}]"
        lines.append(f"    {state['id']}{shape.format(state['name'])}")
    
    # Transitions
    for trans in workflow["transitions"]:
        label = trans["action"]
        lines.append(f"    {trans['from']} -->|{label}| {trans['to']}")
    
    return "\n".join(lines)
```

## 🎓 Zusammenfassung

Workflows in ThemisDB kombinieren:
- **Graph-basierte State Machines** für flexible Prozesse
- **Role-Based Permissions** für Sicherheit
- **Hooks & Handlers** für Erweiterbarkeit
- **Timeouts & Eskalation** für Zuverlässigkeit
- **Multi-Pattern Support** für verschiedene Use Cases

**Nächste Schritte:**
- [API_REFERENCE.md](API_REFERENCE.md) für REST-API Details
- [ADMIN_GUIDE.md](ADMIN_GUIDE.md) für Konfiguration
- [SECURITY.md](SECURITY.md) für Sicherheitsaspekte
