#include "ml_guard.h"
#include <stddef.h>

extern "C" securiot_risk_t ml_guard_classify(const ml_features_t *features) {
    if (features == NULL) {
        return RISK_LOW;
    }
    
    // High Risk: DDoS, fragmentation flood, memory exhaustion, or extreme thermal spike
    if (features->fragment_mismatch_ratio >= 0.25f || 
        features->packet_rate >= 50.0f || 
        features->heap_fragmentation >= 1024.0f || 
        features->cpu_temp_delta >= 3.0f) {
        return RISK_HIGH;
    }
    
    // Medium Risk: Rate limit warning, suspicious IPG scan, or any out-of-order fragment
    if (features->packet_rate >= 5.0f || 
        features->avg_inter_packet_gap_ms <= 150.0f || 
        features->fragment_mismatch_ratio > 0.0f) {
        return RISK_MEDIUM;
    }
    
    return RISK_LOW;
}
