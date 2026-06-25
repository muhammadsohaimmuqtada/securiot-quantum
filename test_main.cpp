#include <stdio.h>
#include <string.h>
#include "securiot_quantum.h"
#include "pqc_engine.h"

int main() {
    printf("===========================================\n");
    printf("   SecurIoT-Quantum Local Integration Test  \n");
    printf("===========================================\n");
    
    // Configure default parameters
    securiot_config_t config;
    config.alarm_pin = 2;
    config.enable_alarm = true;
    config.enable_noise_injection = true;
    config.rate_limit_threshold = 3;
    config.rate_limit_window_ms = 1000;
    
    // Initialize
    securiot_init(&config);
    printf("[SYS] SecurIoT initialized.\n");
    
    // buffers
    uint8_t pk[PQC_PUBKEY_BYTES];
    uint8_t sk[PQC_SECRETKEY_BYTES];
    uint8_t ct[PQC_CIPHERTEXT_BYTES];
    uint8_t ss_client[PQC_SHAREDKEY_BYTES];
    uint8_t ss_server[PQC_SHAREDKEY_BYTES];
    
    // Test Kyber-768 Keygen
    printf("[PQC] Generating Kyber-768 keypair...\n");
    int ret = pqc_generate_keys(pk, sk);
    if (ret != 0) {
        printf("[ERROR] Key generation failed!\n");
        return 1;
    }
    printf("[PQC] Key generation successful. Pubkey: %d bytes, Seckey: %d bytes.\n", PQC_PUBKEY_BYTES, PQC_SECRETKEY_BYTES);
    
    // Test Kyber-768 encapsulation
    printf("[PQC] Performing encapsulation...\n");
    pqc_encapsulate(ct, ss_client, pk);
    
    // Test Kyber-768 decapsulation
    printf("[PQC] Performing decapsulation...\n");
    pqc_decapsulate(ss_server, ct, sk);
    
    // Verify match
    if (memcmp(ss_client, ss_server, PQC_SHAREDKEY_BYTES) == 0) {
        printf("[PQC] SUCCESS: Shared secrets match!\n");
    } else {
        printf("[ERROR] Shared secret mismatch!\n");
        return 1;
    }
    
    // Test rate limiter
    printf("[DEFENSE] Simulating 5 rapid packet arrivals (limit = 3)...\n");
    const char *attacker_ip = "192.168.1.50";
    for (int i = 0; i < 5; i++) {
        securiot_risk_t risk = securiot_process_packet(NULL, 0, attacker_ip);
        printf("Packet %d: Risk level = %d, Dropped total = %u\n", i + 1, risk, securiot_get_dropped_count());
    }
    
    printf("\n[SYS] Integration test completed successfully!\n");
    return 0;
}
