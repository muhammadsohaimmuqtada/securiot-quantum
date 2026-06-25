#ifndef KYBER_K
#define KYBER_K 3  // Force Kyber-768 configuration
#endif

#include "pqc_engine.h"
#include "pqc/kem.h"
#include "pqc/randombytes.h"
#include <stddef.h>
#include <stdint.h>

// Implement portable randombytes for the Kyber library
#if defined(ESP_PLATFORM) || defined(ARDUINO_ARCH_ESP32)
#include "esp_random.h"
void randombytes(uint8_t *out, size_t outlen) {
    esp_fill_random(out, outlen);
}
#else
// POSIX /dev/urandom fallback for offline testing on Kali Linux
#include <stdio.h>
#include <stdlib.h>
void randombytes(uint8_t *out, size_t outlen) {
    static FILE *urandom = NULL;
    if (urandom == NULL) {
        urandom = fopen("/dev/urandom", "rb");
        if (urandom == NULL) {
            perror("SecurIoT PQC Engine: Failed to open /dev/urandom");
            exit(1);
        }
    }
    size_t read_bytes = fread(out, 1, outlen, urandom);
    (void)read_bytes;
}
#endif

int pqc_generate_keys(uint8_t *pk, uint8_t *sk) {
    return crypto_kem_keypair(pk, sk);
}

int pqc_encapsulate(uint8_t *ct, uint8_t *ss, const uint8_t *pk) {
    return crypto_kem_enc(ct, ss, pk);
}

int pqc_decapsulate(uint8_t *ss, const uint8_t *ct, const uint8_t *sk) {
    return crypto_kem_dec(ss, ct, sk);
}
