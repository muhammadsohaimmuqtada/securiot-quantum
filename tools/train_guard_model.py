import os
import numpy as np
from sklearn.tree import DecisionTreeClassifier

def generate_synthetic_data(num_samples=1500):
    # Features: packet_rate, fragment_mismatch_ratio, avg_inter_packet_gap_ms, cpu_temp_delta, heap_fragmentation
    np.random.seed(42)
    
    # Low Risk (Class 0) - Normal operation
    n_low = num_samples // 3
    pr_low = np.random.uniform(0.1, 8.0, n_low)
    fmr_low = np.random.uniform(0.0, 0.05, n_low)
    ipg_low = np.random.uniform(100.0, 1000.0, n_low)
    ctd_low = np.random.uniform(0.0, 1.5, n_low)
    hf_low = np.random.uniform(0.0, 50.0, n_low)
    y_low = np.zeros(n_low, dtype=int)
    
    # Medium Risk (Class 1) - suspicious scanning or probe
    n_med = num_samples // 3
    pr_med = np.random.uniform(10.0, 45.0, n_med)
    fmr_med = np.random.uniform(0.0, 0.12, n_med)
    ipg_med = np.random.uniform(15.0, 90.0, n_med)
    ctd_med = np.random.uniform(0.5, 3.0, n_med)
    hf_med = np.random.uniform(20.0, 200.0, n_med)
    y_med = np.ones(n_med, dtype=int)
    
    # High Risk (Class 2) - DDoS/Fragmentation flood
    n_high = num_samples // 3
    # Group A: High packet rate DDoS
    # Group B: Low rate fragmentation flood (high mismatch ratio, low gap)
    n_high_a = n_high // 2
    n_high_b = n_high - n_high_a
    
    pr_high_a = np.random.uniform(80.0, 500.0, n_high_a)
    fmr_high_a = np.random.uniform(0.0, 0.15, n_high_a)
    ipg_high_a = np.random.uniform(0.1, 8.0, n_high_a)
    ctd_high_a = np.random.uniform(4.0, 15.0, n_high_a)
    hf_high_a = np.random.uniform(500.0, 5000.0, n_high_a)
    
    pr_high_b = np.random.uniform(5.0, 30.0, n_high_b)
    fmr_high_b = np.random.uniform(0.25, 0.85, n_high_b)
    ipg_high_b = np.random.uniform(0.1, 10.0, n_high_b)
    ctd_high_b = np.random.uniform(3.0, 10.0, n_high_b)
    hf_high_b = np.random.uniform(800.0, 10000.0, n_high_b)
    
    pr_high = np.concatenate([pr_high_a, pr_high_b])
    fmr_high = np.concatenate([fmr_high_a, fmr_high_b])
    ipg_high = np.concatenate([ipg_high_a, ipg_high_b])
    ctd_high = np.concatenate([ctd_high_a, ctd_high_b])
    hf_high = np.concatenate([hf_high_a, hf_high_b])
    y_high = np.ones(n_high, dtype=int) * 2
    
    X = np.column_stack([
        np.concatenate([pr_low, pr_med, pr_high]),
        np.concatenate([fmr_low, fmr_med, fmr_high]),
        np.concatenate([ipg_low, ipg_med, ipg_high]),
        np.concatenate([ctd_low, ctd_med, ctd_high]),
        np.concatenate([hf_low, hf_med, hf_high])
    ])
    y = np.concatenate([y_low, y_med, y_high])
    return X, y

def generate_cpp_code(clf, feature_names, cpp_path, h_path):
    # Generate Header file content
    h_content = """#ifndef ML_GUARD_H
#define ML_GUARD_H

#include "securiot_quantum.h"

#ifdef __cplusplus
extern "C" {
#endif

// Features analyzed by the TinyML Guard
typedef struct {
    float packet_rate;                 // Packets per second
    float fragment_mismatch_ratio;     // Ratio of missing/out-of-order fragments
    float avg_inter_packet_gap_ms;     // Average delay between packets (ms)
    float cpu_temp_delta;              // Spikes in CPU temperature (Celsius)
    float heap_fragmentation;          // SRAM decrease rate (bytes per second)
} ml_features_t;

/**
 * @brief Classifies the network and CPU features into risk categories.
 * @param features Pointer to current system and network telemetry features.
 * @return securiot_risk_t Classified risk level (RISK_LOW, RISK_MEDIUM, RISK_HIGH).
 */
securiot_risk_t ml_guard_classify(const ml_features_t *features);

#ifdef __cplusplus
}
#endif

#endif // ML_GUARD_H
"""

    with open(h_path, "w") as f:
        f.write(h_content)
        
    print(f"Header file written to {h_path}")

    # Generate source file content
    tree = clf.tree_
    
    def recurse(node_id, depth):
        indent = "    " * depth
        if tree.children_left[node_id] == tree.children_right[node_id]: # Leaf node
            val = tree.value[node_id][0]
            class_idx = val.argmax()
            risk_label = "RISK_LOW" if class_idx == 0 else "RISK_MEDIUM" if class_idx == 1 else "RISK_HIGH"
            return f"{indent}return {risk_label};\n"
        
        feature_idx = tree.feature[node_id]
        threshold = tree.threshold[node_id]
        feature_name = f"features->{feature_names[feature_idx]}"
        
        left_child = tree.children_left[node_id]
        right_child = tree.children_right[node_id]
        
        left_code = recurse(left_child, depth + 1)
        right_code = recurse(right_child, depth + 1)
        
        code = f"{indent}if ({feature_name} <= {threshold:.6f}f) {{\n"
        code += left_code
        code += f"{indent}}} else {{\n"
        code += right_code
        code += f"{indent}}}\n"
        return code

    body_code = recurse(0, 1)

    cpp_content = f"""#include "ml_guard.h"
#include <stddef.h>

extern "C" securiot_risk_t ml_guard_classify(const ml_features_t *features) {{
    if (features == NULL) {{
        return RISK_LOW;
    }}
    
{body_code}
}}
"""

    with open(cpp_path, "w") as f:
        f.write(cpp_content)
        
    print(f"Source file written to {cpp_path}")

def main():
    X, y = generate_synthetic_data()
    feature_names = [
        "packet_rate",
        "fragment_mismatch_ratio",
        "avg_inter_packet_gap_ms",
        "cpu_temp_delta",
        "heap_fragmentation"
    ]
    
    clf = DecisionTreeClassifier(max_depth=4, random_state=42)
    clf.fit(X, y)
    
    # Ensure folders exist
    os.makedirs("../include", exist_ok=True)
    os.makedirs("../src", exist_ok=True)
    
    h_path = "../include/ml_guard.h"
    cpp_path = "../src/ml_guard.cpp"
    
    generate_cpp_code(clf, feature_names, cpp_path, h_path)
    print("SUCCESS: Decision tree generated successfully!")

if __name__ == "__main__":
    main()
