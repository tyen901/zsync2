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

    st = BCryptCreateHash(ctx->alg, &ctx->h, nullptr, 0, nullptr, 0, 0);
    if(!BCRYPT_SUCCESS(st)) {
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
    BCryptHashData(ctx->h, (PUCHAR)buffer, (ULONG)len, 0);
}

extern "C" const unsigned char* gcry_md_read(gcry_md_hd_t handle, gcry_md_algos algo) {
    if(!handle) return nullptr;
    BCryptCtx* ctx = static_cast<BCryptCtx*>(handle);

    if (!ctx->digest_buf || ctx->digest_len == 0) return nullptr;
    /* FinishHash writes the digest into our internal buffer; copy into ctx->digest_buf */
    NTSTATUS st = BCryptFinishHash(ctx->h, ctx->digest_buf, ctx->digest_len, 0);
    if(!BCRYPT_SUCCESS(st)) {
        return nullptr;
    }
    return ctx->digest_buf;
}

extern "C" void gcry_md_close(gcry_md_hd_t handle) {
    if(!handle) return;
    BCryptCtx* ctx = static_cast<BCryptCtx*>(handle);
    if(ctx->h) BCryptDestroyHash(ctx->h);
    if(ctx->alg) BCryptCloseAlgorithmProvider(ctx->alg,0);
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
