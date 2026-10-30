import math
import random
import time
from typing import List
from py_baseline import analyze_and_normalize_py
import c_python_pybaseline

def verify_correctness():
    """Verify that Python and C implementations yield identical results."""
    print("=== Running Correctness & Edge Case Tests ===")
    
    test_cases = {
        "Standard Dataset": ([1.0, 2.0, 3.0, 4.0, 5.0, 100.0], 1.5, 10.0),
        "Single Element": ([42.0], 0.0, 50.0),
        "Identical Values": ([5.0, 5.0, 5.0, 5.0], 2.0, 8.0),
        "Negative & Floats": ([-10.5, -2.0, 0.0, 3.5, 12.1], -5.0, 5.0)
    }

    for name, (data, low, high) in test_cases.items():
        res_py = analyze_and_normalize_py(data, low, high)
        res_c = c_python_pybaseline.analyze_and_normalize(data, low, high)

        assert math.isclose(res_py["mean"], res_c["mean"], rel_tol=1e-9)
        assert math.isclose(res_py["variance"], res_c["variance"], rel_tol=1e-9)
        assert math.isclose(res_py["min"], res_c["min"], rel_tol=1e-9)
        assert math.isclose(res_py["max"], res_c["max"], rel_tol=1e-9)
        assert res_py["outliers_count"] == res_c["outliers_count"]
        assert len(res_py["normalized"]) == len(res_c["normalized"])
        for p_norm, c_norm in zip(res_py["normalized"], res_c["normalized"]):
            assert math.isclose(p_norm, c_norm, rel_tol=1e-9)

        print(f"  [PASS] {name}")

    # Edge Case: Empty Dataset
    try:
        c_python_pybaseline.analyze_and_normalize([], 0.0, 1.0)
        print("  [FAIL] Empty dataset did not raise ValueError")
    except ValueError:
        print("  [PASS] Empty dataset properly raised ValueError")

    print("All correctness tests passed successfully.\n")

def run_benchmarks():
    """Run performance benchmarks across varying dataset sizes."""
    sizes = [1_000, 100_000, 1_000_000]
    iterations = 5
    threshold_low, threshold_high = -1.5, 1.5

    print(f"{'Dataset Size':<15} | {'Python Time (s)':<17} | {'C Extension (s)':<17} | {'Speedup':<10}")
    print("-" * 70)

    for size in sizes:
        random.seed(42)
        dataset = [random.gauss(0, 1) for _ in range(size)]

        # Benchmark Pure Python
        start = time.perf_counter()
        for _ in range(iterations):
            _ = analyze_and_normalize_py(dataset, threshold_low, threshold_high)
        py_time = (time.perf_counter() - start) / iterations

        # Benchmark C Extension
        start = time.perf_counter()
        for _ in range(iterations):
            _ = c_python_pybaseline.analyze_and_normalize(dataset, threshold_low, threshold_high)
        c_time = (time.perf_counter() - start) / iterations

        speedup = py_time / c_time if c_time > 0 else 0.0
        print(f"{size:<15,d} | {py_time:<17.6f} | {c_time:<17.6f} | {speedup:<10.2f}x")

if __name__ == "__main__":
    verify_correctness()
    run_benchmarks()
