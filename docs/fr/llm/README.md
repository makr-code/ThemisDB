# Documentation Intégration LLM & IA

**État :** 20 décembre 2025  
**Version :** 1.3.0 (Intégration LLM Native avec llama.cpp - Optionnel)  
**Catégorie :** LLM & IA Distribuée

---

## 🚀 Aperçu

ThemisDB offre depuis la v1.3.0 un **moteur LLM embarqué optionnel** basé sur **llama.cpp** ainsi que des fondations pour le **raisonnement distribué**. Le module LLM comprend une architecture de plugins, le chargement paresseux de modèles, Multi-LoRA et l'intégration RAG.

> **Note importante** : L'intégration LLM est une **fonctionnalité optionnelle** :
> - Nécessite le flag de build : `-DTHEMIS_ENABLE_LLM=ON`
> - Nécessite une dépendance externe : llama.cpp (à cloner séparément)
> - Non activé par défaut

**Implémenté dans v1.3.0 :**
- 🧠 **llama.cpp embarqué** – Inférence LLM native sans APIs externes (optionnel)
- ⚡ **Accélération GPU** (CUDA/Metal/Vulkan) pour des débits élevés
- 🧩 **Architecture de plugins** avec `LlamaWrapper`
- 🗃️ **Chargement paresseux de modèles** (style Ollama)
- 🔀 **Gestion Multi-LoRA** (style vLLM)

**Liens rapides :**
- [🧠 **Guide Complet de Configuration LLM**](../guides/LLM_COMPLETE_SETUP_GUIDE.md) – **NOUVEAU** Guide complet pour la configuration et l'inférence
- [LLAMA_CPP_INTEGRATION.md](./LLAMA_CPP_INTEGRATION.md) – Intégration & Build
- [README_PLUGINS.md](./README_PLUGINS.md) – Démarrage rapide & Exemples
- [INTEGRATION_REVIEW_AND_SEQUENCE.md](./INTEGRATION_REVIEW_AND_SEQUENCE.md) – Architecture & Séquences

## Référence du Code Source

| Composant | Header | Source | Description |
|-----------|--------|--------|-------------|
| LLMInteractionStore | `llm_interaction_store.h` | `llm_interaction_store.cpp` | Stockage des interactions |
| PromptManager | `prompt_manager.h` | `prompt_manager.cpp` | Modèles de prompts |

**Total :** 2 Header, 2 fichiers Source, ~700 LOC

## Classes Implémentées

### LLMInteractionStore

```cpp
class LLMInteractionStore {
    // Format de clé : "llm_interaction:{interaction_id}"
    
    struct Interaction {
        std::string id;                        // UUID
        std::string prompt_template_id;        // Référence au modèle
        std::string prompt;                    // Prompt envoyé
        std::vector<std::string> reasoning_chain; // Étapes Chain-of-Thought
        std::string response;                  // Réponse LLM
        std::string model_version;             // ex., "gpt-4o-mini"
        int64_t timestamp_ms;
        int latency_ms;
        int token_count;
        nlohmann::json metadata;               // Feedback, user_id, etc.
    };
    
    struct ListOptions {
        size_t limit = 100;
        std::optional<std::string> start_after_id;  // Pagination
        std::optional<std::string> filter_model;
        std::optional<int64_t> since_timestamp_ms;
    };
    
    struct Stats {
        size_t total_interactions;
        int64_t total_tokens;
        double avg_latency_ms;
        size_t total_size_bytes;
    };
    
    // API
    Interaction createInteraction(Interaction);
    std::optional<Interaction> getInteraction(id);
    std::vector<Interaction> listInteractions(ListOptions);
    bool deleteInteraction(id);
    Stats getStats();
};
```

### PromptManager

```cpp
class PromptManager {
    struct PromptTemplate {
        std::string id;
        std::string name;
        std::string template_text;
        std::string version;
        nlohmann::json variables;     // {name, description, required}
        int64_t created_at;
        int64_t updated_at;
    };
    
    // API
    PromptTemplate createTemplate(PromptTemplate);
    std::optional<PromptTemplate> getTemplate(id);
    std::vector<PromptTemplate> listTemplates();
    PromptTemplate updateTemplate(id, updates);
    bool deleteTemplate(id);
    
    // Rendu
    std::string render(template_id, variables);
};
```

## Fonctionnalités

### Stockage Chain-of-Thought

```cpp
Interaction interaction;
interaction.reasoning_chain = {
    "Étape 1 : Analyser la requête utilisateur",
    "Étape 2 : Identifier les documents pertinents",
    "Étape 3 : Générer la réponse basée sur le contexte"
};
store.createInteraction(interaction);
```

### Suivi des Tokens & Latence

```cpp
interaction.token_count = 150;
interaction.latency_ms = 450;

auto stats = store.getStats();
// stats.avg_latency_ms = 380.5
// stats.total_tokens = 125000
```

### Versionnement des Prompts

```cpp
auto template = manager.createTemplate({
    .name = "qa_template",
    .template_text = "Question: {{question}}\nContexte: {{context}}\nRéponse:",
    .version = "1.0.0",
    .variables = {{"question", {.required = true}}, {"context", {.required = true}}}
});

auto prompt = manager.render(template.id, {
    {"question", "Qu'est-ce que ThemisDB ?"},
    {"context", "ThemisDB est une base de données multi-modèle..."}
});
```

## API HTTP

### POST /api/llm/interactions

```json
{
  "prompt": "Quelle est la capitale de la France ?",
  "response": "La capitale de la France est Paris.",
  "model_version": "gpt-4o-mini",
  "reasoning_chain": ["Analyser la requête", "Rechercher la connaissance", "Générer la réponse"],
  "token_count": 25,
  "latency_ms": 200
}
```

### GET /api/llm/interactions?limit=10&filter_model=gpt-4o-mini
### GET /api/llm/stats

### PATCH /llm/interaction/{id} - Mettre à Jour les Métadonnées (Enterprise)

```json
{
  "feedback": {
    "rating": 5,
    "feedback_text": "Excellente réponse",
    "user_id": "user123",
    "flagged_for_training": true,
    "training_category": "positive"
  }
}
```

### POST /query/enhanced - Requête Améliorée avec Contexte LLM (Enterprise)

```json
{
  "aql": "FOR doc IN products FILTER doc.category == 'electronics' RETURN doc",
  "llm_context": {
    "limit": 5,
    "model": "gpt-4o-mini"
  }
}
```

## Fonctionnalités Enterprise

### Système de Feedback

Le système de feedback est implémenté comme un add-on Enterprise et utilise le champ flexible `metadata`. Il ne nécessite **aucune couche séparée**.

**Cas d'usage :**
- Collecter les retours utilisateurs sur les réponses LLM
- Marquer les données d'entraînement pour le fine-tuning LoRA
- Suivi des métriques de qualité

**Voir :** [LLM Feedback Enterprise](./LLM_FEEDBACK_ENTERPRISE.md)

### Amélioration de Requête

Combine les requêtes de base de données avec le contexte LLM pour les applications assistées par IA.

**Avantages :**
- Réduction des coûts de 49%
- Amélioration de la latence de 38%
- Amélioration de la qualité de +25-40%
- Boucle de feedback en temps réel

**Voir :** [Analyse des Avantages de l'Intégration LLM](../enterprise/LLM_INTEGRATION_BENEFITS_ANALYSIS.md)

## Documentation Connexe

### Intégration LLM & Raisonnement Distribué

- [**Guide de Déploiement Docker/VM**](./DOCKER_VM_DEPLOYMENT_GUIDE.md) ⭐ **NOUVEAU :** Déploiement en conteneurs et VMs
  - **Passthrough GPU :** Docker NVIDIA Toolkit, KVM/QEMU, VMware vSphere
  - **Mode de Repli CPU :** Tests complets SANS GPU (5-10x plus lent, mais fonctionnel)
  - **Mode Mixte :** Cluster hybride avec shards GPU + CPU
  - **Tests Multi-Shard :** Complètement possible dans Docker sans GPU
  - **Kubernetes :** Plugin de périphérique GPU, StatefulSets, Sondes de santé
  - **Performance :** GPU (100%), CPU (15-20%), Mixte (90%/15%)

- [**Stratégie de Surveillance & Tests**](./MONITORING_TESTING_STRATEGY.md) ⭐ **NOUVEAU :** Observabilité & QA
  - **6 Tableaux de bord Grafana :** Vue d'ensemble du cluster, Communication Inter-Cérébrale, Performance LLM, Recherche vectorielle, Raisonnement distribué, Coût/ROI
  - **Surveillance Inter-Cérébrale :** Visualisation de communication Shard-à-Shard inspirée du cerveau
  - **40+ Métriques Prometheus :** Counter, Gauge, Histogram pour tous les composants
  - **Pyramide de Tests :** 70% Unitaires, 25% Intégration, 5% E2E
  - **Intégration CI/CD :** GitHub Actions avec tests GPU/CPU
  - **Benchmarks :** Débit d'inférence, Raisonnement distribué, Transfert LoRA, Recherche vectorielle
  - **Attendu :** 99,9% de disponibilité, <2s de latence p95, >70% d'utilisation VRAM

- [**Licence VRAM Enterprise**](./ENTERPRISE_VRAM_LICENSING.md) ⭐ **NOUVEAU :** Modèle de licence basé sur VRAM
  - **Édition Communautaire (Gratuit) :** ≤24 Go VRAM - Couvre 80% de tous les cas d'usage
  - **Édition Enterprise :** >24 Go VRAM - Llama-70B+, Multi-GPU, Cluster HA
  - **Implémentation Technique :** VRAMLicenseManager, Enforcement Runtime
  - **Tarification :** 5 000 € - 50 000 €/an selon le niveau VRAM
  - **ROI :** Économies de 90-99% vs. Hyperscaler
  - **Essai Gratuit :** Tester Enterprise pendant 30 jours

## 🚀 Démarrage Rapide

### Configuration de Base

```bash
# Activer LLM dans le build
cmake -DTHEMIS_ENABLE_LLM=ON ..

# Charger un modèle
curl -X POST http://localhost:8765/llm/models/load \
  -d '{"model": "llama-7b-q4", "path": "/models/llama-7b-q4.gguf"}'

# Exécuter l'inférence
curl -X POST http://localhost:8765/llm/infer \
  -d '{"prompt": "Qu'\''est-ce que ThemisDB ?", "max_tokens": 100}'
```

### Utilisation avec Python

```python
from themisdb import ThemisClient

client = ThemisClient('localhost:8765')

# Créer un modèle de prompt
template = client.llm.create_template(
    name="qa_template",
    template="Question: {{question}}\nRéponse:",
    variables={"question": {"required": True}}
)

# Utiliser le modèle
response = client.llm.infer(
    template_id=template.id,
    variables={"question": "Quelle est la capitale de la France ?"}
)

print(response.text)  # "La capitale de la France est Paris."
```

## 💡 Bonnes Pratiques

### Sélection de Modèle

1. **Pour la Production :**
   - Modèles 7B-13B pour la plupart des cas d'usage
   - Quantification Q4 pour équilibre vitesse/qualité
   - GPU pour <2s de latence

2. **Pour le Développement :**
   - Modèles 3B pour les tests rapides
   - Mode CPU acceptable pour les tests
   - Modèles de petite taille pour l'itération

### Gestion de la Mémoire

```yaml
# Configuration LLM
llm:
  max_models_in_memory: 2
  model_cache_size_gb: 16
  gpu_memory_fraction: 0.8
```

### Surveillance

```python
# Obtenir les statistiques
stats = client.llm.get_stats()
print(f"Interactions totales : {stats.total_interactions}")
print(f"Latence moyenne : {stats.avg_latency_ms}ms")
print(f"Tokens totaux : {stats.total_tokens}")
```

## 🔧 Dépannage

### Modèle Non Chargé

```bash
# Lister les modèles
curl http://localhost:8765/llm/models

# Charger le modèle
curl -X POST http://localhost:8765/llm/models/load \
  -d '{"model": "llama-7b", "path": "/models/llama-7b.gguf"}'
```

### Mémoire GPU Insuffisante

**Solution :**
- Utiliser des modèles plus petits (7B au lieu de 13B)
- Activer la quantification (Q4 au lieu de FP16)
- Réduire la taille du contexte

### Latence Élevée

**Optimisations :**
- Activer l'accélération GPU
- Utiliser le batching pour plusieurs requêtes
- Réduire max_tokens
- Utiliser le cache sémantique

---

## 📚 Voir Aussi

### Documentation LLM

- **[Guide Complet de Configuration](../guides/LLM_COMPLETE_SETUP_GUIDE.md)** - Configuration de bout en bout
- **[Intégration llama.cpp](LLAMA_CPP_INTEGRATION.md)** - Détails d'intégration
- **[Architecture de Plugins](README_PLUGINS.md)** - Système de plugins
- **[Raisonnement Distribué](DISTRIBUTED_REASONING_ARCHITECTURE.md)** - Architecture multi-shard

### Guides

- **[Guide Utilisateur](../guides/USER_GUIDE.md)** - Utilisation générale
- **[Référence API](../apis/HTTP_API_REFERENCE.md)** - Endpoints HTTP
- **[AQL](../aql/README.md)** - Langage de requête

---

**Version :** 1.3.0  
**Statut :** Production (Optionnel)  
**Dernière Mise à Jour :** 23 Décembre 2025
