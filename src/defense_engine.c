#include "defense_engine.h"
#include <string.h>

#define MAX_IP_TRACK 16

typedef struct {
    char ip[16];
    uint32_t last_time_ms;
    uint32_t count;
    bool blocked;
} ip_tracker_t;

static ip_tracker_t ip_table[MAX_IP_TRACK];
static int ip_count = 0;
static uint32_t dropped_packets = 0;
static securiot_config_t local_config;

// Portable millisecond clock
#if defined(ARDUINO)
#include <Arduino.h>
uint32_t get_time_ms(void) {
    return millis();
}
#elif defined(ESP_PLATFORM)
#include "esp_timer.h"
uint32_t get_time_ms(void) {
    return (uint32_t)(esp_timer_get_time() / 1000);
}
#else
// POSIX fallback for host testing
#include <time.h>
uint32_t get_time_ms(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint32_t)(ts.tv_sec * 1000 + ts.tv_nsec / 1000000);
}
#endif

// Portable GPIO Pin Alarm Triggering
#if defined(ARDUINO)
#include <Arduino.h>
void set_alarm_pin(uint32_t pin, bool active) {
    pinMode(pin, OUTPUT);
    digitalWrite(pin, active ? HIGH : LOW);
}
#elif defined(ESP_PLATFORM)
#include "driver/gpio.h"
void set_alarm_pin(uint32_t pin, bool active) {
    // Reset/configure GPIO
    gpio_reset_pin(pin);
    gpio_set_direction(pin, GPIO_MODE_OUTPUT);
    gpio_set_level(pin, active ? 1 : 0);
}
#else
// Linux console logging simulation
#include <stdio.h>
void set_alarm_pin(uint32_t pin, bool active) {
    static bool prev_state = false;
    if (active != prev_state) {
        printf("[ACTIVE DEFENSE] Alarm Pin %u toggled to %s\n", pin, active ? "HIGH (ON)" : "LOW (OFF)");
        prev_state = active;
    }
}
#endif

// Portable Timing Noise Injection (vTaskDelay vs Sleep)
#if defined(ESP_PLATFORM) || defined(ARDUINO_ARCH_ESP32)
#if defined(ARDUINO)
#include <Arduino.h>
#else
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#endif
#include "esp_random.h"
void inject_delay(void) {
    // Random timing delay between 5ms and 25ms to thwart side-channel analysis
    uint32_t delay_ms = 5 + (esp_random() % 21);
#if defined(ARDUINO)
    delay(delay_ms);
#else
    vTaskDelay(pdMS_TO_TICKS(delay_ms));
#endif
}
#else
// POSIX microseconds sleep fallback
#include <unistd.h>
#include <stdlib.h>
void inject_delay(void) {
    uint32_t delay_ms = 5 + (rand() % 21);
    usleep(delay_ms * 1000);
}
#endif

void defense_engine_init(const securiot_config_t *config) {
    if (config) {
        local_config = *config;
    } else {
        // Load secure defaults
        local_config.alarm_pin = 2;              // Default to onboard LED pin
        local_config.enable_noise_injection = true;
        local_config.enable_alarm = true;
        local_config.rate_limit_threshold = 10;   // 10 packets per window max
        local_config.rate_limit_window_ms = 1000; // 1 second window
    }
    
    memset(ip_table, 0, sizeof(ip_table));
    ip_count = 0;
    dropped_packets = 0;
    
    if (local_config.enable_alarm) {
        set_alarm_pin(local_config.alarm_pin, false);
    }
}

bool defense_engine_evaluate(securiot_risk_t risk, const char *src_ip) {
    uint32_t now = get_time_ms();
    
    // 1. Drop immediately under High Risk and switch on alarm
    if (risk == RISK_HIGH) {
        dropped_packets++;
        if (local_config.enable_alarm) {
            set_alarm_pin(local_config.alarm_pin, true);
        }
        return true;  // DROP
    }
    
    // 2. Track packet rate and apply IP-specific rate limiting
    if (src_ip != NULL && strlen(src_ip) > 0) {
        int found_idx = -1;
        for (int i = 0; i < ip_count; i++) {
            if (strcmp(ip_table[i].ip, src_ip) == 0) {
                found_idx = i;
                break;
            }
        }
        
        if (found_idx == -1) {
            // Store new IP record
            if (ip_count < MAX_IP_TRACK) {
                found_idx = ip_count;
                ip_count++;
            } else {
                // Evict slot 0 when full (simple FIFO)
                found_idx = 0;
            }
            strncpy(ip_table[found_idx].ip, src_ip, 15);
            ip_table[found_idx].ip[15] = '\0';
            ip_table[found_idx].last_time_ms = now;
            ip_table[found_idx].count = 1;
            ip_table[found_idx].blocked = false;
        } else {
            // Check rate limiting time-window
            if (now - ip_table[found_idx].last_time_ms < local_config.rate_limit_window_ms) {
                ip_table[found_idx].count++;
                if (ip_table[found_idx].count > local_config.rate_limit_threshold) {
                    ip_table[found_idx].blocked = true;
                }
            } else {
                // Reset rate limit window
                ip_table[found_idx].last_time_ms = now;
                ip_table[found_idx].count = 1;
                ip_table[found_idx].blocked = false;
            }
        }
        
        if (ip_table[found_idx].blocked) {
            dropped_packets++;
            return true;  // DROP
        }
    }
    
    // 3. Inject timing noise under Medium Risk
    if (risk == RISK_MEDIUM) {
        if (local_config.enable_noise_injection) {
            inject_delay();
        }
    } else {
        // Clear alarm under Low Risk
        if (local_config.enable_alarm) {
            set_alarm_pin(local_config.alarm_pin, false);
        }
    }
    
    return false;  // ALLOW
}

uint32_t defense_engine_get_dropped(void) {
    return dropped_packets;
}
