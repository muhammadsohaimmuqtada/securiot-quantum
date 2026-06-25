#ifndef PQC_ENGINE_H
#define PQC_ENGINE_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// Define Kyber-768 parameter sizes (FIPS 203 ML-KEM-768 standard values)
#define PQC_PUBKEY_BYTES     1184  // 1184 bytes
#define PQC_SECRETKEY_BYTES   2400  // 2400 bytes
#define PQC_CIPHERTEXT_BYTES  1088  // 1088 bytes
#define PQC_SHAREDKEY_BYTES   32    // 32 bytes (256-bit symmetric key)

/**
 * @brief Generates a post-quantum Kyber-768 keypair.
 * @param pk Pointer to buffer of size PQC_PUBKEY_BYTES where the public key will be stored.
 * @param sk Pointer to buffer of size PQC_SECRETKEY_BYTES where the secret key will be stored.
 * @return 0 on success, or a non-zero error code.
 */
int pqc_generate_keys(uint8_t *pk, uint8_t *sk);

/**
 * @brief Encapsulates a shared secret under a Kyber-768 public key.
 * @param ct Pointer to buffer of size PQC_CIPHERTEXT_BYTES where the ciphertext will be stored.
 * @param ss Pointer to buffer of size PQC_SHAREDKEY_BYTES where the shared secret will be stored.
 * @param pk Pointer to the recipient's public key.
 * @return 0 on success, or a non-zero error code.
 */
int pqc_encapsulate(uint8_t *ct, uint8_t *ss, const uint8_t *pk);

/**
 * @brief Decapsulates a shared secret from a Kyber-768 ciphertext.
 * @param ss Pointer to buffer of size PQC_SHAREDKEY_BYTES where the shared secret will be stored.
 * @param ct Pointer to the incoming ciphertext.
 * @param sk Pointer to the recipient's secret key.
 * @return 0 on success, or a non-zero error code.
 */
int pqc_decapsulate(uint8_t *ss, const uint8_t *ct, const uint8_t *sk);

#ifdef __cplusplus
}
#endif

#endif // PQC_ENGINE_H
