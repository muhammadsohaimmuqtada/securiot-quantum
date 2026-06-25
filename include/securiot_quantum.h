#ifndef SECURIOT_QUANTUM_H
#define SECURIOT_QUANTUM_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// Risk levels returned by the threat detection system
typedef enum {
    RISK_LOW = 0,
    RISK_MEDIUM = 1,
    RISK_HIGH = 2
} securiot_risk_t;

// SecurIoT configuration parameters
typedef struct {
    uint32_t alarm_pin;                // GPIO pin to trigger under High Risk (DDoS/Flood)
    bool enable_noise_injection;       // Enable random timing delays on Medium Risk to stop side channels
    bool enable_alarm;                 // Enable physical alarm triggering
    uint32_t rate_limit_threshold;     // Max packets allowed per client in rate limit window
    uint32_t rate_limit_window_ms;     // Rate limiting window duration in ms
} securiot_config_t;

/**
 * @brief Initializes the SecurIoT-Quantum shield library.
 * @param config Pointer to the configuration structure. If NULL, default parameters are used.
 */
void securiot_init(const securiot_config_t *config);

/**
 * @brief Processes an incoming packet metadata and telemetry to assess threat level.
 * @param packet Pointer to the raw packet payload.
 * @param len Length of the packet.
 * @param src_ip Source IP address string of the sender.
 * @return securiot_risk_t Calculated risk level. If RISK_HIGH, packet should be dropped.
 */
securiot_risk_t securiot_process_packet(const uint8_t *packet, uint32_t len, const char *src_ip);

/**
 * @brief Returns the current risk level of the node.
 * @return securiot_risk_t Current risk category.
 */
securiot_risk_t securiot_get_current_risk(void);

/**
 * @brief Returns the total number of dropped packets since initialization.
 * @return uint32_t Total dropped packets.
 */
uint32_t securiot_get_dropped_count(void);

#ifdef __cplusplus
}
#endif

#endif // SECURIOT_QUANTUM_H
