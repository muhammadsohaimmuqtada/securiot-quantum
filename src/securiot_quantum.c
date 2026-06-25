#include "securiot_quantum.h"
#include "ml_guard.h"
#include "defense_engine.h"
#include <stddef.h>
#include <stdio.h>

static securiot_risk_t current_risk = RISK_LOW;
static uint32_t window_start_ms = 0;
static uint32_t window_packets = 0;
static uint32_t last_packet_ms = 0;
static float cpu_temp_baseline = 38.0f;
static uint32_t total_ipg_sum = 0;
static uint32_t ipg_count = 0;
static uint32_t prev_heap = 0;
static uint32_t window_fragments = 0;
static uint32_t window_mismatched_fragments = 0;
static uint8_t expected_next_frag = 0;

// Internal declarations for portable time and heap APIs (implemented in defense_engine.c)
extern uint32_t get_time_ms(void);

#if defined(ARDUINO)
#include <Arduino.h>
#include "esp_system.h"
#elif defined(ESP_PLATFORM)
#include "esp_system.h"
#endif

// Portable CPU Temperature Reading
#if defined(ARDUINO) && defined(ESP32)
#ifdef __cplusplus
extern "C" {
#endif
uint8_t temprature_sens_read();
#ifdef __cplusplus
}
#endif
float read_cpu_temp(void) {
    return (float)temprature_sens_read();
}
#else
// Standard mockup baseline for Linux tests or devices without direct ROM access
float read_cpu_temp(void) {
    return 38.5f;
}
#endif

// Portable Heap Delta Tracker
static float get_heap_diff_rate(void) {
    uint32_t current_heap = 0;
#if defined(ESP_PLATFORM) || defined(ARDUINO_ARCH_ESP32) || defined(ARDUINO)
    current_heap = esp_get_free_heap_size();
#else
    // Mock heap space on Linux
    current_heap = 100000;
#endif

    if (prev_heap == 0) {
        prev_heap = current_heap;
        return 0.0f;
    }
    
    float diff = 0.0f;
    if (current_heap < prev_heap) {
        diff = (float)(prev_heap - current_heap);
    }
    prev_heap = current_heap;
    return diff; // Returns heap decrease in bytes
}

void securiot_init(const securiot_config_t *config) {
    current_risk = RISK_LOW;
    window_start_ms = 0;
    window_packets = 0;
    last_packet_ms = 0;
    total_ipg_sum = 0;
    ipg_count = 0;
    prev_heap = 0;
    window_fragments = 0;
    window_mismatched_fragments = 0;
    expected_next_frag = 0;
    cpu_temp_baseline = read_cpu_temp();
    
    // Initialize defense engine
    defense_engine_init(config);
}

securiot_risk_t securiot_process_packet(const uint8_t *packet, uint32_t len, const char *src_ip) {
    uint32_t now = get_time_ms();
    
    if (window_start_ms == 0) {
        window_start_ms = now;
        cpu_temp_baseline = read_cpu_temp();
        prev_heap = 0;
    }
    
    window_packets++;
    
    // Track fragment mismatch ratio over the window
    if (packet != NULL && len >= 3 && packet[0] == 'F') {
        window_fragments++;
        uint8_t total = packet[1];
        uint8_t current = packet[2];
        if (current != expected_next_frag) {
            window_mismatched_fragments++;
        }
        expected_next_frag = (current + 1) % total;
    }
    
    // Calculate Inter-Packet Gap (IPG)
    if (last_packet_ms != 0) {
        uint32_t gap = now - last_packet_ms;
        total_ipg_sum += gap;
        ipg_count++;
    }
    last_packet_ms = now;
    
    uint32_t elapsed = now - window_start_ms;
    
    // Analyze and classify threats in sliding 1-second windows
    if (elapsed >= 1000) {
        ml_features_t features;
        
        // Calculate packets per second
        features.packet_rate = (float)window_packets / ((float)elapsed / 1000.0f);
        
        // Calculate average IPG
        if (ipg_count > 0) {
            features.avg_inter_packet_gap_ms = (float)total_ipg_sum / (float)ipg_count;
        } else {
            features.avg_inter_packet_gap_ms = 1000.0f;
        }
        
        // Calculate CPU Temperature Delta
        float current_temp = read_cpu_temp();
        features.cpu_temp_delta = current_temp - cpu_temp_baseline;
        if (features.cpu_temp_delta < 0.0f) {
            features.cpu_temp_delta = 0.0f;
        }
        
        // Calculate Heap decrease rate (bytes per second)
        features.heap_fragmentation = get_heap_diff_rate() / ((float)elapsed / 1000.0f);
        
        // Calculate fragment mismatch ratio over the sliding window
        if (window_fragments > 0) {
            features.fragment_mismatch_ratio = (float)window_mismatched_fragments / (float)window_fragments;
        } else {
            features.fragment_mismatch_ratio = 0.0f;
        }
        
        // Run classification decision forest
        current_risk = ml_guard_classify(&features);
        
        // Print window evaluation details
        printf("\n[SecurIoT Window Evaluation (elapsed %u ms)]:\n", (unsigned int)elapsed);
        printf("  -> Packets in Window: %u (Rate: %.2f pps)\n", (unsigned int)window_packets, features.packet_rate);
        printf("  -> Avg Inter-packet Gap: %.2f ms\n", features.avg_inter_packet_gap_ms);
        printf("  -> Mismatch Ratio: %.2f (Total Frag Packets: %u, Mismatches: %u)\n", 
               features.fragment_mismatch_ratio, (unsigned int)window_fragments, (unsigned int)window_mismatched_fragments);
        printf("  -> CPU Temperature Delta: %.2f C\n", features.cpu_temp_delta);
        printf("  -> Heap Loss Rate: %.2f bytes/sec\n", features.heap_fragmentation);
        printf("  -> CLASSIFIED RISK LEVEL: %d\n", current_risk);
        
        // Reset window stats
        window_start_ms = now;
        window_packets = 0;
        total_ipg_sum = 0;
        ipg_count = 0;
        window_fragments = 0;
        window_mismatched_fragments = 0;
    }
    
    // Active defense evaluation (packet filtering/rate-limiting/timing noise)
    bool drop = defense_engine_evaluate(current_risk, src_ip);
    
    printf("[SecurIoT Packet] Src: %s | Len: %u | Risk: %d | Decision: %s\n", 
           src_ip ? src_ip : "UNKNOWN", (unsigned int)len, current_risk, drop ? "DROP" : "ALLOW");
    
    if (drop) {
        return RISK_HIGH; // Return RISK_HIGH to tell the application layer to drop the packet
    }
    
    return current_risk;
}

securiot_risk_t securiot_get_current_risk(void) {
    return current_risk;
}

uint32_t securiot_get_dropped_count(void) {
    return defense_engine_get_dropped();
}
