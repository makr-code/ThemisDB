---
category: "🛠️ Developer/Technical"
version: "v1.3.0"
status: "✅"
date: "23.12.2025"
---

# 🛠️ Guide de Dépannage - ThemisDB

Aide rapide pour les problèmes courants de build et d'exécution.

## 📋 Table des Matières

- [🔧 Problèmes de Build](#-problèmes-de-build)
- [🐳 Problèmes Docker](#-problèmes-docker)
- [⚡ Problèmes d'Exécution](#-problèmes-dexécution)
- [🔍 Outils de Débogage](#-outils-de-débogage)
- [🆘 Support](#-support)

## 🔧 Problèmes de Build

### Windows MSVC: "unrecognized file format"

**Symptôme :**
```
error MSB3073: cmake.exe -E __create_def ... a quitté avec le code 1
```

**Solution :**
```powershell
# Option 1: Utilisez un build statique (recommandé)
cmake -DTHEMIS_CORE_SHARED=OFF ...

# Option 2: Utilisez le script de build
.\scripts\build.ps1 -Target windows
```

---

### CMake Configuration Failed

**Symptôme :** "Required package not found"

**Solution :**
```bash
# Clean Build
rm -rf build-msvc  # Windows
rm -rf build-wsl   # Linux

# Reconfigurer complètement
cmake -S . -B build-msvc \
  -DCMAKE_TOOLCHAIN_FILE=./vcpkg/scripts/buildsystems/vcpkg.cmake
```

---

## 🐳 Problèmes Docker

### Le Conteneur ne Démarre Pas

**Diagnostic :**
```bash
# Vérifier les logs
docker logs <container-id>

# Mode debug interactif
docker run -it --entrypoint /bin/bash themisdb/themisdb:latest
```

**Causes fréquentes :**
- Port déjà utilisé (8080/18765)
- Permissions de volume
- Mémoire insuffisante

---

## ⚡ Problèmes d'Exécution

### Le Serveur ne Démarre Pas

**Diagnostic :**
```bash
# Test avec log détaillé
./themis_server --log-level=debug

# Vérifier la config
cat config/themis.yaml
```

---

### "Failed to open RocksDB"

**Solution :**
```bash
# Vérifier les permissions
ls -la data/rocksdb

# Créer le répertoire
mkdir -p data/rocksdb
chmod 755 data/rocksdb
```

---

## 🔍 Outils de Débogage

### Vérifier les Dépendances

**Windows :**
```powershell
# Vérifier les DLLs
.\scripts\check-dll-dependencies.ps1
```

**Linux :**
```bash
# Bibliothèques partagées
ldd ./themis_server
```

---

## 🆘 Support

Si les problèmes persistent :

1. **Issues GitHub** : [github.com/makr-code/ThemisDB/issues](https://github.com/makr-code/ThemisDB/issues)
2. **Documentation** : [makr-code.github.io/ThemisDB/](https://makr-code.github.io/ThemisDB/)
3. **Wiki** : [github.com/makr-code/ThemisDB/wiki](https://github.com/makr-code/ThemisDB/wiki)

### Informations pour les Rapports de Bugs

Veuillez toujours inclure :
- **Plateforme** : Windows / Linux / Docker
- **Type de Build** : Release / Debug
- **Version CMake** : `cmake --version`
- **Compilateur** : MSVC / GCC / Clang + version
- **Messages d'Erreur** : Messages d'erreur complets
