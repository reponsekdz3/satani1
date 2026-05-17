// hex_utils.h - Hexadecimal conversion and cryptographic utility functions header
// Provides declarations for hex encoding/decoding, bit manipulation, and crypto helpers

#ifndef HEX_UTILS_H
#define HEX_UTILS_H

#include <stdint.h>

// Hex string to binary conversion
// Returns number of bytes written, or -1 on error
int hex_to_bytes(const char* hex, uint8_t* out, size_t out_size);

// Binary to hex string conversion
// Returns number of characters written (excluding null terminator), or -1 on error
int bytes_to_hex(const uint8_t* in, size_t in_size, char* out, size_t out_size);

// XOR two buffers together (in-place)
void xor_buf(uint8_t* buf1, const uint8_t* buf2, size_t len);

// 32-bit rotate left
uint32_t rotl32(uint32_t value, unsigned int count);

// 64-bit rotate left
uint64_t rotl64(uint64_t value, unsigned int count);

// 32-bit rotate right
uint32_t rotr32(uint32_t value, unsigned int count);

// 64-bit rotate right
uint64_t rotr64(uint64_t value, unsigned int count);

// Constant-time buffer comparison (to prevent timing attacks)
// Returns 1 if equal, 0 if not equal
int buf_eq(const uint8_t* buf1, const uint8_t* buf2, size_t len);

// Safe buffer allocation with error handling
// Returns allocated buffer or NULL on failure
uint8_t* alloc_buf(size_t size);

// AES-128 ECB encryption (simplified - real implementation would use AES-NI or BCrypt)
// This is a placeholder for demonstration - in production use proper AES implementation
int encrypt_aes_ecb(const uint8_t* plaintext, size_t plaintext_len,
                   const uint8_t* key, size_t key_len,
                   uint8_t* ciphertext, size_t ciphertext_size);

// Derive TLS master secret from client_random, server_random, and premaster_secret
// Uses real HMAC-SHA256 via BCrypt
int derive_tls_master_secret(const char* client_random, const char* server_random,
                           const char* premaster_secret, char* master_secret, size_t master_secret_size);

// Generate GNSS key stream from entropy sources
// Uses real BCryptGenRandom for cryptographic quality randomness
int generate_gnss_key_stream(const char* gnss_system, uint8_t* key_stream, size_t key_stream_size);

#endif // HEX_UTILS_H