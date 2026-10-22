import json
import sys
import tempfile
import unittest
from pathlib import Path

# Allow direct import from benchmarks/ like sibling tests in this directory.
_BENCHMARKS = Path(__file__).resolve().parent.parent
if str(_BENCHMARKS) not in sys.path:
    sys.path.insert(0, str(_BENCHMARKS))

from hybrid_break_even_report import generate_report


class HybridBreakEvenReportTest(unittest.TestCase):
    def _write_json(self, payload: dict) -> Path:
        fd, raw_path = tempfile.mkstemp(suffix=".json")
        path = Path(raw_path)
        path.write_text(json.dumps(payload), encoding="utf-8")
        return path

    def test_reports_gpu_thresholds_for_ann_and_tensor(self):
        ann = self._write_json(
            {
                "benchmarks": [
                    {
                        "name": "BM_ANN_CPUSIMDDispatchBatchKnn/4000/128/10/1_mean",
                        "run_name": "BM_ANN_CPUSIMDDispatchBatchKnn/4000/128/10/1",
                        "aggregate_name": "mean",
                        "real_time": 1200,
                        "time_unit": "us",
                    },
                    {
                        "name": "BM_ANN_MixedPathGpuQueryAccelerator/4000/128/10/1_mean",
                        "run_name": "BM_ANN_MixedPathGpuQueryAccelerator/4000/128/10/1",
                        "aggregate_name": "mean",
                        "real_time": 1400,
                        "time_unit": "us",
                    },
                    {
                        "name": "BM_ANN_CPUSIMDDispatchBatchKnn/16000/128/10/32_mean",
                        "run_name": "BM_ANN_CPUSIMDDispatchBatchKnn/16000/128/10/32",
                        "aggregate_name": "mean",
                        "real_time": 3200,
                        "time_unit": "us",
                    },
                    {
                        "name": "BM_ANN_MixedPathGpuQueryAccelerator/16000/128/10/32_mean",
                        "run_name": "BM_ANN_MixedPathGpuQueryAccelerator/16000/128/10/32",
                        "aggregate_name": "mean",
                        "real_time": 1400,
                        "time_unit": "us",
                    },
                ]
            }
        )
        tensor = self._write_json(
            {
                "benchmarks": [
                    {"name": "BM_TEN_S4_BatchSimilarityCPU/256/64/1", "real_time": 400, "time_unit": "us"},
                    {"name": "BM_TEN_S4_BatchSimilarityGPU/256/64/1", "real_time": 500, "time_unit": "us"},
                    {"name": "BM_TEN_S4_BatchSimilarityCPU/1024/64/32", "real_time": 2200, "time_unit": "us"},
                    {"name": "BM_TEN_S4_BatchSimilarityGPU/1024/64/32", "real_time": 1200, "time_unit": "us"},
                ]
            }
        )

        report = generate_report(ann, tensor, None)

        self.assertEqual(report["ann"]["gpu_wins_from"], {
            "num_vectors": 16000,
            "dim": 128,
            "top_k": 10,
            "batch_size": 32,
        })
        self.assertEqual(report["tensor"]["gpu_wins_from"], {
            "num_vectors": 1024,
            "dim": 64,
            "batch_size": 32,
        })
        self.assertEqual(report["graph"]["status"], "missing_or_no_data")


if __name__ == "__main__":
    unittest.main()
