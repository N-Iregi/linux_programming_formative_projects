# baseline.py
import math
from typing import Dict, List, Sequence, Tuple, Any

def analyze_and_normalize_py(
    data: Sequence[float], 
    threshold_low: float, 
    threshold_high: float
) -> Dict[str, Any]:
    """
    Pure Python baseline for dataset analysis and normalization.
    
    Operations:
    1. Calculate mean and population variance.
    2. Find minimum and maximum values.
    3. Count values falling outside [threshold_low, threshold_high].
    4. Apply min-max normalization: y_i = (x_i - min) / (max - min).
    """
    n = len(data)
    if n == 0:
        raise ValueError("Dataset cannot be empty")

    min_val = float('inf')
    max_val = float('-inf')
    total_sum = 0.0
    outliers_count = 0

    # Pass 1: Sum, Min, Max, and Outliers
    for x in data:
        val = float(x)
        total_sum += val
        if val < min_val:
            min_val = val
        if val > max_val:
            max_val = val
        if val < threshold_low or val > threshold_high:
            outliers_count += 1

    mean_val = total_sum / n

    # Pass 2: Population Variance
    variance_sum = 0.0
    for x in data:
        diff = float(x) - mean_val
        variance_sum += diff * diff
    variance_val = variance_sum / n

    # Pass 3: Min-Max Normalization
    range_val = max_val - min_val
    normalized: List[float] = []
    
    if range_val == 0.0:
        normalized = [0.0] * n
    else:
        normalized = [(float(x) - min_val) / range_val for x in data]

    return {
        "mean": mean_val,
        "variance": variance_val,
        "min": min_val,
        "max": max_val,
        "outliers_count": outliers_count,
        "normalized": normalized
    }
