// Minimal libgcrypt compatibility layer backed by OpenSSL EVP
#pragma once

#include <cstddef>
#include <cstdint>
// #include <openssl/evp.h>

// minimal subset of gcrypt types used in zsync2
typedef void* gcry_md_hd_t;
typedef int gcry_error_t;
typedef int gcry_md_algos;

// algorithm identifiers used in the code (map a few common ones)
#define GCRY_MD_MD5 1
#define GCRY_MD_SHA1 2
#define GCRY_MD_SHA256 3

#ifdef __cplusplus
extern "C" {
#endif

// function prototypes
gcry_error_t gcry_md_open(gcry_md_hd_t* handle, gcry_md_algos algo, unsigned int flags);
void gcry_md_write(gcry_md_hd_t handle, const void* buffer, size_t len);
const unsigned char* gcry_md_read(gcry_md_hd_t handle, gcry_md_algos algo);
void gcry_md_close(gcry_md_hd_t handle);
size_t gcry_md_get_algo_dlen(gcry_md_algos algo);

#ifdef __cplusplus
}
#endif
