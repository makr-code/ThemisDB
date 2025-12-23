---
category: "📋 Guides"
version: "v1.3.0"
status: "✅"
date: "23.12.2025"
---

# 📋 Guide de Démarrage Rapide

Démarrer avec ThemisDB en 5 minutes

## 📋 Table des Matières

- [✨ Fonctionnalités & Points Forts](#-fonctionnalités--points-forts)
- [🚀 Démarrage Rapide](#-démarrage-rapide)
- [💡 Premiers Pas](#-premiers-pas)
- [🔧 Configuration](#-configuration)
- [🛠️ Dépannage](#-dépannage)
- [📚 Voir Aussi](#-voir-aussi)

---

## ✨ Fonctionnalités & Points Forts

ThemisDB offre :
- 🐳 **Support Docker** - Installation la plus rapide
- 📦 **Multi-Plateforme** - Windows, Linux, macOS
- ⚡ **Zéro Configuration** - Démarrage en secondes
- 🔗 **API REST** - Intégration simple
- 📊 **Multi-Modèle** - Relationnel, Document, Graphe, Vecteur

---

## 🚀 Démarrage Rapide (5 Minutes)

### Prérequis

- **Docker** (recommandé) OU
- **Linux/macOS/Windows** avec outils de build

### Option 1 : Docker (Recommandé)

**Le moyen le plus rapide de démarrer :**

```bash
# Télécharger la dernière image
docker pull themisdb/themisdb:latest

# Exécuter ThemisDB
docker run -d \
  --name themisdb \
  -p 8765:8765 \
  -p 8080:8080 \
  -v themis_data:/data \
  themisdb/themisdb:latest

# Vérifier que ça fonctionne
curl http://localhost:8765/health
```

**Réponse attendue :**
```json
{"status":"ok","uptime":5}
```

### Option 2 : Paquets Pré-compilés

**Debian/Ubuntu :**
```bash
wget https://github.com/makr-code/ThemisDB/releases/latest/download/themisdb_1.2.0-1_amd64.deb
sudo apt install ./themisdb_1.2.0-1_amd64.deb
sudo systemctl start themisdb
```

**macOS (Homebrew) :**
```bash
brew install themisdb
brew services start themisdb
```

**Windows (Chocolatey) :**
```powershell
choco install themisdb
```

---

## 💡 Premiers Pas

### 1. Vérifier la Santé du Serveur

```bash
curl http://localhost:8765/health
```

### 2. Créer Votre Première Entité

```bash
curl -X PUT http://localhost:8765/entities/users:alice \
  -H "Content-Type: application/json" \
  -d '{"blob":"{\"name\":\"Alice\",\"age\":30,\"city\":\"Berlin\"}"}'
```

### 3. Requête Simple

```bash
curl -X POST http://localhost:8765/query \
  -H "Content-Type: application/json" \
  -d '{"table":"users","predicates":[{"column":"city","value":"Berlin"}]}'
```

---

## 🔧 Configuration

Créer `config.yaml` :

```yaml
server:
  host: 0.0.0.0
  port: 8765
storage:
  data_dir: ./data
logging:
  level: info
```

---

## 🛠️ Dépannage

### Le Serveur ne Démarre Pas

```bash
# Vérifier les logs
./themis_server --log-level=debug
```

### Port Déjà Utilisé

```bash
# Utiliser un autre port
./themis_server --port=8766
```

---

## 📚 Voir Aussi

- **[Guide Utilisateur](USER_GUIDE.md)** - Guide complet
- **[Référence API](../apis/HTTP_API_REFERENCE.md)** - Documentation API
- **[Architecture](../architecture/ARCHITECTURE_OVERVIEW.md)** - Conception système

---

**Prêt à commencer ?** 🚀
