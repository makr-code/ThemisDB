# Tesseract OCR Language Packs — `config/ai_ml/tesseract_lang/`

This directory is the **canonical tessdata root** for ThemisDB's OCR processor.
When `OcrProcessor::Config::data_dir` is left empty (the default), the processor
automatically resolves this path via `ConfigPathResolver` and passes it to
`TessBaseAPI::Init()` as the tessdata directory.  If the directory does not exist
at startup, Tesseract falls back to its built-in auto-detection mechanism
(e.g. `TESSDATA_PREFIX` environment variable or the system tessdata location).

---

## Directory Layout

```
config/ai_ml/tesseract_lang/
├── README.md              ← this file
├── eng.traineddata        ← English (required baseline; installed by default)
├── deu.traineddata        ← German (optional)
├── fra.traineddata        ← French (optional)
└── ...                    ← additional language packs
```

Each language is represented by a single `<lang>.traineddata` file.
The file names follow Tesseract's ISO 639-2 three-letter codes
(e.g. `eng`, `deu`, `fra`, `spa`, `ita`, `por`, `chi_sim`, `jpn`, …).

---

## Installing the Baseline English Pack

```bash
# Ubuntu / Debian
sudo apt-get install tesseract-ocr-eng
cp /usr/share/tesseract-ocr/5/tessdata/eng.traineddata \
   config/ai_ml/tesseract_lang/

# macOS (Homebrew)
brew install tesseract
cp "$(brew --prefix tesseract)/share/tessdata/eng.traineddata" \
   config/ai_ml/tesseract_lang/
```

---

## Installing Additional Language Packs

### Option A — Copy from the system tessdata directory

```bash
# List all installed packs (Ubuntu/Debian)
ls /usr/share/tesseract-ocr/5/tessdata/

# Install a pack, e.g. German
sudo apt-get install tesseract-ocr-deu
cp /usr/share/tesseract-ocr/5/tessdata/deu.traineddata \
   config/ai_ml/tesseract_lang/
```

### Option B — Download from the official Tesseract repository

Best-accuracy models (~10–30 MB each):
```bash
BASE_URL="https://github.com/tesseract-ocr/tessdata_best/raw/main"
DEST="config/ai_ml/tesseract_lang"

# English
curl -L "$BASE_URL/eng.traineddata" -o "$DEST/eng.traineddata"

# German
curl -L "$BASE_URL/deu.traineddata" -o "$DEST/deu.traineddata"

# French
curl -L "$BASE_URL/fra.traineddata" -o "$DEST/fra.traineddata"
```

Fast (smaller) models from `tessdata_fast`:
```bash
BASE_URL="https://github.com/tesseract-ocr/tessdata_fast/raw/main"
```

---

## Per-Collection Override

Operators can override the tessdata directory on a per-collection basis by
setting `OcrProcessor::Config::data_dir` to an absolute path before creating
the processor:

```cpp
OcrProcessor::Config cfg;
cfg.language = "deu";
cfg.data_dir = "/opt/custom_tessdata";          // overrides the default
auto processor = createOcrProcessor(std::move(cfg));
```

---

## Supported Language Codes (examples)

| Code       | Language              |
|------------|-----------------------|
| `eng`      | English               |
| `deu`      | German                |
| `fra`      | French                |
| `spa`      | Spanish               |
| `ita`      | Italian               |
| `por`      | Portuguese            |
| `nld`      | Dutch                 |
| `rus`      | Russian               |
| `chi_sim`  | Chinese Simplified    |
| `chi_tra`  | Chinese Traditional   |
| `jpn`      | Japanese              |
| `kor`      | Korean                |
| `ara`      | Arabic                |

For the full list see: <https://tesseract-ocr.github.io/tessdoc/Data-Files.html>

---

## Legacy Path

The legacy path `config/tesseract_lang/` is mapped to this directory by
`ConfigPathResolver` for backward compatibility.  Operators using the old path
will receive a deprecation warning; please migrate to
`config/ai_ml/tesseract_lang/` before 2027-06-30.
