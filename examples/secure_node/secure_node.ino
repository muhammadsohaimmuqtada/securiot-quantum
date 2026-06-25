#include <securiot_quantum.h>
#include <pqc_engine.h>
#include <WiFi.h>
#include <WiFiUdp.h>

// Array of common programmable LED pins across different ESP32 development boards
const int alarm_pins[] = {2, 4, 5, 12, 13, 14, 15, 21, 22, 23};
const int num_alarm_pins = sizeof(alarm_pins) / sizeof(alarm_pins[0]);

// Function to drive all common LED pins
void set_all_alarms(bool active) {
    for (int i = 0; i < num_alarm_pins; i++) {
        pinMode(alarm_pins[i], OUTPUT);
        digitalWrite(alarm_pins[i], active ? HIGH : LOW);
    }
}

// Configuration
#define ALARM_PIN             2
#define RATE_LIMIT_MAX        5      // Maximum 5 packets/second before rate-limit
#define WINDOW_DURATION_MS    1000

// Wi-Fi credentials
const char* ssid = "SHAHID-JUTT-4G";
const char* password = "wanbhachran";

// UDP listener port
const unsigned int localPort = 8080;
WiFiUDP Udp;
uint8_t packetBuffer[1500]; // buffer to hold incoming packet

// Cryptographic buffers for Kyber-768
uint8_t public_key[PQC_PUBKEY_BYTES];
uint8_t secret_key[PQC_SECRETKEY_BYTES];
uint8_t ciphertext[PQC_CIPHERTEXT_BYTES];
uint8_t client_shared_secret[PQC_SHAREDKEY_BYTES];
uint8_t server_shared_secret[PQC_SHAREDKEY_BYTES];

void printRiskLevel(securiot_risk_t risk);

void securiot_task(void *pvParameters) {
    Serial.println("===========================================");
    Serial.println("   SecurIoT-Quantum Shield Initializing    ");
    Serial.println("===========================================");

    // Define configuration
    securiot_config_t config;
    config.alarm_pin = ALARM_PIN;
    config.enable_noise_injection = true;
    config.enable_alarm = true;
    config.rate_limit_threshold = RATE_LIMIT_MAX;
    config.rate_limit_window_ms = WINDOW_DURATION_MS;

    // Start shield
    securiot_init(&config);
    
    // Set up all alternate alarm pins as well
    set_all_alarms(false);
    
    Serial.println("SecurIoT-Quantum Shield Active.");

    // Connect to Wi-Fi
    Serial.print("Connecting to Wi-Fi network: ");
    Serial.println(ssid);
    WiFi.begin(ssid, password);
    
    int retries = 0;
    while (WiFi.status() != WL_CONNECTED && retries < 20) {
        delay(500);
        Serial.print(".");
        retries++;
    }
    
    if (WiFi.status() == WL_CONNECTED) {
        Serial.println("\nWi-Fi connected successfully!");
        Serial.print("IP Address: ");
        Serial.println(WiFi.localIP());
        Udp.begin(localPort);
        Serial.print("Listening for UDP packets on port ");
        Serial.println(localPort);
    } else {
        Serial.println("\nWi-Fi connection failed! Continuing in Serial-only mode.");
    }

    Serial.println("\n[SYSTEM] Ready for interactive manual/network commands!");
    Serial.println("Type '1' to execute Post-Quantum Key Exchange (Kyber-768)");
    Serial.println("Type '2' to process a Normal Telemetry Packet");
    Serial.println("Type '3' to trigger IP Rate-Limiting Attack Simulation");
    Serial.println("Type '4' to trigger DDoS Fragmentation Flood Simulation");
    Serial.println("Type '5' to reset alarm state and drop counters");

    while (true) {
        // 1. Handle Serial Commands
        if (Serial.available()) {
            char cmd = Serial.read();
            while (Serial.available() && (Serial.peek() == '\n' || Serial.peek() == '\r')) {
                Serial.read();
            }

            if (cmd == '1') {
                Serial.println("\n[COMMAND] Executing Kyber-768 Key Exchange...");
                Serial.println("Generating ephemeral Kyber-768 keypair...");
                unsigned long start_time = millis();
                int ret = pqc_generate_keys(public_key, secret_key);
                unsigned long duration = millis() - start_time;
                
                if (ret == 0) {
                    Serial.print("SUCCESS: Kyber-768 keys generated in ");
                    Serial.print(duration);
                    Serial.println(" ms.");
                    
                    Serial.println("Executing post-quantum KEM encapsulation...");
                    pqc_encapsulate(ciphertext, client_shared_secret, public_key);
                    Serial.println("Executing post-quantum KEM decapsulation...");
                    pqc_decapsulate(server_shared_secret, ciphertext, secret_key);
                    
                    if (memcmp(client_shared_secret, server_shared_secret, PQC_SHAREDKEY_BYTES) == 0) {
                        Serial.println("SUCCESS: Post-Quantum shared secret established!");
                    } else {
                        Serial.println("ERROR: Cryptographic key desync!");
                    }
                } else {
                    Serial.println("ERROR: Kyber-768 key generation failed!");
                }
            }
            else if (cmd == '2') {
                Serial.println("\n[COMMAND] Processing normal telemetry packet...");
                uint8_t packet[32] = "NormalTelemetryDataPacket";
                securiot_risk_t risk = securiot_process_packet(packet, sizeof(packet), "192.168.1.100");
                printRiskLevel(risk);
                if (risk == RISK_HIGH) {
                    Serial.println("PACKET BLOCKED: High risk active dropping engaged.");
                    set_all_alarms(true);
                } else {
                    set_all_alarms(false);
                }
            }
            else if (cmd == '3') {
                Serial.println("\n[COMMAND] Simulating client rate-limit violation...");
                uint8_t packet[32] = "RateLimitTestPacket";
                const char *attacker_ip = "192.168.1.200";
                for (int i = 0; i < 8; i++) {
                    Serial.print("Sending packet ");
                    Serial.print(i + 1);
                    Serial.print("... ");
                    
                    securiot_risk_t rate_risk = securiot_process_packet(packet, sizeof(packet), attacker_ip);
                    
                    if (rate_risk == RISK_HIGH) {
                        Serial.println("BLOCKED (IP Rate-limited!)");
                    } else if (rate_risk == RISK_MEDIUM) {
                        Serial.println("Suspicious! (Timing noise delay injected)");
                    } else {
                        Serial.println("Allowed");
                    }
                    vTaskDelay(pdMS_TO_TICKS(100)); // Fast burst rate
                }
            }
            else if (cmd == '4') {
                Serial.println("\n[COMMAND] Simulating DDoS Fragmentation Flood...");
                const char *ddos_ip = "192.168.1.250";
                uint8_t fragment1[4] = {'F', 4, 0}; 
                uint8_t fragment2[4] = {'F', 4, 3}; 
                
                for (int i = 0; i < 15; i++) {
                    uint8_t *active_frag = (i % 2 == 0) ? fragment1 : fragment2;
                    securiot_risk_t flood_risk = securiot_process_packet(active_frag, sizeof(active_frag), ddos_ip);
                    
                    if (flood_risk == RISK_HIGH) {
                        Serial.print("Packet ");
                        Serial.print(i + 1);
                        Serial.println(": DROPPED immediately (ML Guard Shield High Risk!). Alarm pin GPIO 2 HIGH!");
                        set_all_alarms(true);
                    } else {
                        Serial.print("Packet ");
                        Serial.print(i + 1);
                        Serial.println(": Allowed");
                    }
                    vTaskDelay(pdMS_TO_TICKS(50));
                }
                Serial.print("Total dropped packets: ");
                Serial.println(securiot_get_dropped_count());
            }
            else if (cmd == '5') {
                Serial.println("\n[COMMAND] Resetting defense engine alarm and counters...");
                set_all_alarms(false);
                Serial.println("Alarm pins set to LOW. Ready for new tests.");
            }
        }

        // 2. Handle UDP packets (Real-time Network Testing)
        if (WiFi.status() == WL_CONNECTED) {
            int packetSize = Udp.parsePacket();
            if (packetSize > 0 && packetSize < sizeof(packetBuffer)) {
                // Read packet
                Udp.read(packetBuffer, packetSize);
                IPAddress remoteIP = Udp.remoteIP();
                String remoteIPStr = remoteIP.toString();

                // Process packet through securiot shield
                securiot_risk_t risk = securiot_process_packet(packetBuffer, packetSize, remoteIPStr.c_str());
                
                // If high risk, turn on all alarm pins
                if (risk == RISK_HIGH) {
                    set_all_alarms(true);
                } else if (risk == RISK_LOW) {
                    set_all_alarms(false);
                }

                if (risk != RISK_HIGH) {
                    // Send response indicating it was processed and allowed
                    Udp.beginPacket(remoteIP, Udp.remotePort());
                    Udp.write((const uint8_t*)"ALLOWED", 7);
                    Udp.endPacket();
                }
            }
        }

        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

void setup() {
    Serial.begin(115200);
    delay(1000);
    
    // Create dedicated FreeRTOS task with 24 KB stack size
    xTaskCreatePinnedToCore(
        securiot_task,
        "SecurIoTTask",
        24576,
        NULL,
        1,
        NULL,
        1
    );
}

void loop() {
    // Nothing to do here, keep loopTask idle
    vTaskDelay(portMAX_DELAY);
}

void printRiskLevel(securiot_risk_t risk) {
    Serial.print("Current Threat Level: ");
    switch (risk) {
        case RISK_LOW:
            Serial.println("LOW (Operational Safe Mode)");
            break;
        case RISK_MEDIUM:
            Serial.println("MEDIUM (Suspicious IP - Mitigation Active)");
            break;
        case RISK_HIGH:
            Serial.println("HIGH (Threat Detected - Packet Dropped!)");
            break;
        default:
            Serial.println("UNKNOWN");
            break;
    }
}
