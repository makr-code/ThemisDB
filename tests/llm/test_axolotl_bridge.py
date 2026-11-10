import json
import tempfile
import unittest
from pathlib import Path
from unittest.mock import patch

from src.llm.lora_framework.axolotl_bridge import (
    AdapterConverter,
    AxolotlBridge,
    AxolotlTrainer,
    DataExporter,
    LoRAHyperparameters,
    QLoRAConfig,
    ThemisDBTrainingConfig,
)


def make_config(output_dir: str) -> ThemisDBTrainingConfig:
    return ThemisDBTrainingConfig(
        adapter_id="test_adapter",
        base_model_path="tinyllama",
        output_dir=output_dir,
        hyperparameters=LoRAHyperparameters(),
        qlora=QLoRAConfig(),
    )


class AxolotlBridgeTest(unittest.TestCase):
    def test_normalizes_documentation_chunks_to_alpaca_samples(self) -> None:
        artifact = {
            "artifact_documents": [{"doc_id": "doc-1", "title": "Storage Guide"}],
            "chunks": [
                {"doc_id": "doc-1", "chunk_id": "chunk-1", "text": "RocksDB stores records."},
                {"doc_id": "doc-1", "chunk_id": "empty", "text": "  "},
            ],
        }

        samples = DataExporter.normalize_training_data(artifact)

        self.assertEqual(len(samples), 1)
        self.assertEqual(samples[0]["input"], "Documentation source: Storage Guide")
        self.assertEqual(samples[0]["output"], "RocksDB stores records.")

    def test_streams_docs_artifact_with_sample_limit(self) -> None:
        artifact = {
            "artifact_documents": [{"doc_id": "doc-1", "title": "Storage Guide"}],
            "chunks": [
                {"doc_id": "doc-1", "chunk_id": f"chunk-{index}", "text": f"Paragraph {index}."}
                for index in range(3)
            ],
        }
        with tempfile.TemporaryDirectory() as temp_dir:
            artifact_path = Path(temp_dir) / "docs.json"
            artifact_path.write_text(json.dumps(artifact), encoding="utf-8")
            samples = DataExporter.load_from_file(str(artifact_path), max_samples=2)

        self.assertEqual(len(samples), 2)
        self.assertEqual(samples[0]["input"], "Documentation source: Storage Guide")

    def test_streaming_reader_skips_cache_sources(self) -> None:
        artifact = {
            "artifact_documents": [
                {"doc_id": "cache", "file_path": ".pytest_cache/README.md"},
                {"doc_id": "docs", "file_path": "docs/guide.md", "title": "Guide"},
            ],
            "chunks": [
                {"doc_id": "cache", "text": "Generated cache data."},
                {"doc_id": "docs", "text": "ThemisDB documentation."},
            ],
        }
        with tempfile.TemporaryDirectory() as temp_dir:
            artifact_path = Path(temp_dir) / "docs.json"
            artifact_path.write_text(json.dumps(artifact), encoding="utf-8")
            samples = DataExporter.load_from_file(str(artifact_path), max_samples=10)

        self.assertEqual(len(samples), 1)
        self.assertEqual(samples[0]["output"], "ThemisDB documentation.")

    def test_export_preserves_instruction_and_context(self) -> None:
        sample = {
            "instruction": "Continue the documentation.",
            "input": "Documentation source: Storage Guide",
            "output": "RocksDB stores records.",
        }

        with tempfile.TemporaryDirectory() as temp_dir:
            output_path = Path(temp_dir) / "samples.jsonl"
            self.assertEqual(DataExporter.export_to_alpaca([sample], str(output_path)), 1)
            exported = json.loads(output_path.read_text(encoding="utf-8"))

        self.assertEqual(exported["instruction"], sample["instruction"])
        self.assertEqual(exported["input"], sample["input"])
        self.assertEqual(exported["output"], sample["output"])

    def test_empty_training_data_fails_before_axolotl_check(self) -> None:
        with tempfile.TemporaryDirectory() as temp_dir:
            result = AxolotlTrainer(make_config(temp_dir)).train({"chunks": []})

        self.assertFalse(result["success"])
        self.assertEqual(result["num_samples"], 0)
        self.assertIn("No usable", result["error"])

    @patch("src.llm.lora_framework.axolotl_bridge.subprocess.run")
    def test_converter_uses_configured_script_and_base_model_cli(self, run_process) -> None:
        def create_output(command, **_kwargs):
            Path(command[command.index("--outfile") + 1]).write_bytes(b"gguf")
            return type("ProcessResult", (), {"stdout": "converted"})()

        run_process.side_effect = create_output
        with tempfile.TemporaryDirectory() as temp_dir:
            converter_path = Path(temp_dir) / "llama.cpp" / "convert_lora_to_gguf.py"
            converter_path.parent.mkdir()
            converter_path.touch()
            base_model_dir = Path(temp_dir) / "base-model"
            base_model_dir.mkdir()

            for index, (base_model, base_flag, base_value) in enumerate((
                ("tinyllama", "--base-model-id", "tinyllama"),
                (str(base_model_dir), "--base", str(base_model_dir)),
            )):
                output_path = Path(temp_dir) / f"adapter-{index}.gguf"
                self.assertTrue(AdapterConverter.peft_to_gguf(
                    "peft-adapter",
                    str(output_path),
                    base_model,
                    convert_script=str(converter_path),
                ))

                command = run_process.call_args.args[0]
                self.assertEqual(Path(command[1]), converter_path)
                self.assertEqual(command[2], "peft-adapter")
                self.assertEqual(command[command.index("--outfile") + 1], str(output_path))
                self.assertEqual(command[command.index(base_flag) + 1], base_value)

    def test_selects_peft_when_axolotl_is_unavailable(self) -> None:
        sample = [{"instruction": "Continue.", "output": "Documentation text."}]
        with tempfile.TemporaryDirectory() as temp_dir:
            trainer = AxolotlTrainer(make_config(temp_dir))
            with patch.object(trainer, "_check_axolotl_available", return_value=False):
                with patch.object(trainer, "_run_peft_training", return_value={"success": True}) as train_peft:
                    result = trainer.train(sample)

        self.assertTrue(result["success"])
        train_peft.assert_called_once()

    def test_config_from_dict_builds_nested_hyperparameters(self) -> None:
        config = ThemisDBTrainingConfig.from_dict({
            "adapter_id": "test_adapter",
            "base_model_path": "tinyllama",
            "output_dir": "output",
            "hyperparameters": {"rank": 4, "target_modules": ["q_proj"]},
            "qlora": {"enabled": False},
        })

        self.assertEqual(config.hyperparameters.rank, 4)
        self.assertEqual(config.hyperparameters.target_modules, ["q_proj"])
        self.assertFalse(config.qlora.enabled)

    def test_conversion_failure_fails_the_training_pipeline(self) -> None:
        with tempfile.TemporaryDirectory() as temp_dir:
            bridge = AxolotlBridge(make_config(temp_dir))
            sample = [{"instruction": "Continue.", "output": "Documentation text."}]
            with patch.object(bridge.trainer, "train", return_value={"success": True, "adapter_path": temp_dir}):
                with patch.object(AdapterConverter, "peft_to_gguf", return_value=False):
                    result = bridge.train_and_convert(sample)

        self.assertFalse(result["success"])
        self.assertEqual(result["error"], "GGUF adapter conversion failed")


if __name__ == "__main__":
    unittest.main()