// Minimal libgcrypt compatibility layer backed by Windows BCrypt APIs.
#include "../include/gcrypt_compat.h"
#include <windows.h>
#include <bcrypt.h>
#include <vector>
#include <memory>

// Link with bcrypt.lib on MSVC
#ifndef BCRYPT_SUCCESS
#define BCRYPT_SUCCESS(Status) (((NTSTATUS)(Status)) >= 0)
#endif

struct BCryptCtx {
    BCRYPT_HASH_HANDLE h = nullptr;
    BCRYPT_ALG_HANDLE alg = nullptr;
    unsigned char* digest_buf = nullptr; /* allocated to hold final digest */
    ULONG digest_len = 0;
    PUCHAR hash_object = nullptr; /* object buffer required by BCryptCreateHash */
    ULONG hash_object_len = 0;
    bool finalized = false;
};

static LPCWSTR map_algo(gcry_md_algos algo) {
    switch(algo) {
        case GCRY_MD_MD5: return BCRYPT_MD5_ALGORITHM;
        case GCRY_MD_SHA1: return BCRYPT_SHA1_ALGORITHM;
        case GCRY_MD_SHA256: return BCRYPT_SHA256_ALGORITHM;
        default: return nullptr;
    }
}

extern "C" gcry_error_t gcry_md_open(gcry_md_hd_t* handle, gcry_md_algos algo, unsigned int /*flags*/) {
    LPCWSTR algId = map_algo(algo);
    if(!algId) return -1;

    BCryptCtx* ctx = new BCryptCtx();
    NTSTATUS st = BCryptOpenAlgorithmProvider(&ctx->alg, algId, nullptr, 0);
    if(!BCRYPT_SUCCESS(st)) {
        delete ctx;
        return -1;
    }

    /* Query required object length for hash handle and allocate object buffer */
    ULONG result = 0;
    st = BCryptGetProperty(ctx->alg, BCRYPT_OBJECT_LENGTH, (PUCHAR)&ctx->hash_object_len, sizeof(ctx->hash_object_len), &result, 0);
    if (!BCRYPT_SUCCESS(st) || ctx->hash_object_len == 0) {
        BCryptCloseAlgorithmProvider(ctx->alg, 0);
        delete ctx;
        return -1;
    }

    ctx->hash_object = (PUCHAR)HeapAlloc(GetProcessHeap(), 0, ctx->hash_object_len);
    if (!ctx->hash_object) {
        BCryptCloseAlgorithmProvider(ctx->alg, 0);
        delete ctx;
        return -1;
    }

    st = BCryptCreateHash(ctx->alg, &ctx->h, ctx->hash_object, ctx->hash_object_len, nullptr, 0, 0);
    if(!BCRYPT_SUCCESS(st)) {
        HeapFree(GetProcessHeap(), 0, ctx->hash_object);
        BCryptCloseAlgorithmProvider(ctx->alg,0);
        delete ctx;
        return -1;
    }

    /* allocate digest buffer to hold final digest; caller will be returned a pointer
     * to this internal buffer by gcry_md_read (matching libgcrypt semantics where
     * the returned pointer is valid until the handle is closed). */
    ctx->digest_len = (ULONG)gcry_md_get_algo_dlen(algo);
    if (ctx->digest_len > 0) {
        ctx->digest_buf = (unsigned char*)HeapAlloc(GetProcessHeap(), 0, ctx->digest_len);
        if (!ctx->digest_buf) {
            BCryptDestroyHash(ctx->h);
            BCryptCloseAlgorithmProvider(ctx->alg,0);
            delete ctx;
            return -1;
        }
    }

    *handle = ctx;
    return 0;
}

extern "C" void gcry_md_write(gcry_md_hd_t handle, const void* buffer, size_t len) {
    if(!handle || !buffer) return;
    BCryptCtx* ctx = static_cast<BCryptCtx*>(handle);
    if (ctx->finalized) {
        /* Already finalized (digest produced). Further writes are ignored by this
         * compatibility shim. A more complete implementation would reset or
         * recreate the hash context. */
        return;
    }

    NTSTATUS st = BCryptHashData(ctx->h, (PUCHAR)buffer, (ULONG)len, 0);
    (void)st; /* if it fails, we have no way to report via this void API; gcry_md_read will fail */
}

extern "C" const unsigned char* gcry_md_read(gcry_md_hd_t handle, gcry_md_algos algo) {
    if(!handle) return nullptr;
    BCryptCtx* ctx = static_cast<BCryptCtx*>(handle);

    if (!ctx->digest_buf || ctx->digest_len == 0) return nullptr;
    /* If we've already finalized, return cached digest. Otherwise finish the
     * hash and cache the result so repeated reads succeed like libgcrypt. */
    if (ctx->finalized) return ctx->digest_buf;

    NTSTATUS st = BCryptFinishHash(ctx->h, ctx->digest_buf, ctx->digest_len, 0);
    if(!BCRYPT_SUCCESS(st)) {
        return nullptr;
    }

    ctx->finalized = true;
    return ctx->digest_buf;
}

extern "C" void gcry_md_close(gcry_md_hd_t handle) {
    if(!handle) return;
    BCryptCtx* ctx = static_cast<BCryptCtx*>(handle);
    if(ctx->h) BCryptDestroyHash(ctx->h);
    if(ctx->alg) BCryptCloseAlgorithmProvider(ctx->alg,0);
    if (ctx->hash_object) {
        HeapFree(GetProcessHeap(), 0, ctx->hash_object);
        ctx->hash_object = nullptr;
    }
    if (ctx->digest_buf) {
        HeapFree(GetProcessHeap(), 0, ctx->digest_buf);
        ctx->digest_buf = nullptr;
    }
    delete ctx;
}

extern "C" size_t gcry_md_get_algo_dlen(gcry_md_algos algo) {
    switch(algo) {
        case GCRY_MD_MD5: return 16;
        case GCRY_MD_SHA1: return 20;
        case GCRY_MD_SHA256: return 32;
        default: return 0;
    }
}
