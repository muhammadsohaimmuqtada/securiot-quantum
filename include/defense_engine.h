#ifndef DEFENSE_ENGINE_H
#define DEFENSE_ENGINE_H

#include "securiot_quantum.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Initializes the active defense engine.
 * @param config Pointer to configuration parameters.
 */
void defense_engine_init(const securiot_config_t *config);

/**
 * @brief Evaluates an incoming connection IP and risk profile to decide if it should be dropped.
 * @param risk Current classified risk category.
 * @param src_ip Source IP address of the incoming packet.
 * @return true if the packet must be dropped (mitigation active), false to allow processing.
 */
bool defense_engine_evaluate(securiot_risk_t risk, const char *src_ip);

/**
 * @brief Retrieves total packets dropped by the active defense engine.
 * @return uint32_t Dropped packet counter.
 */
uint32_t defense_engine_get_dropped(void);

#ifdef __cplusplus
}
#endif

#endif // DEFENSE_ENGINE_H
