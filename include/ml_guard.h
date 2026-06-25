#ifndef ML_GUARD_H
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
